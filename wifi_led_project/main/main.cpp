#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "esp_timer.h"

#include "config.h"
#include "led_controller.h"
#include "wifi_station.h"
#include "wifi_config.h"
#include "ble_provisioning.h"

static const char* TAG = "main";

// Task monitorujący przycisk konfiguracyjny
static void button_monitor_task(void* arg)
{
    LEDController* led = static_cast<LEDController*>(arg);
    
    // Konfiguracja GPIO przycisku
    gpio_config_t btn_cfg = {};
    btn_cfg.pin_bit_mask = (1ULL << CONFIG_BUTTON_GPIO);
    btn_cfg.mode = GPIO_MODE_INPUT;
    btn_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
    btn_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
    btn_cfg.intr_type = GPIO_INTR_DISABLE;
    gpio_config(&btn_cfg);
    
    int64_t press_start_time = 0;
    bool was_pressed = false;
    
    ESP_LOGI(TAG, "Button monitor task started on GPIO%d", CONFIG_BUTTON_GPIO);
    
    while (true) {
        int level = gpio_get_level(CONFIG_BUTTON_GPIO);
        bool is_pressed = (level == 0); // Active LOW (pullup)
        
        int64_t now = esp_timer_get_time() / 1000; // ms
        
        if (is_pressed && !was_pressed) {
            // Przycisk został naciśnięty
            press_start_time = now;
            ESP_LOGI(TAG, "Button pressed");
        }
        
        if (!is_pressed && was_pressed) {
            // Przycisk został zwolniony
            press_start_time = 0;
            ESP_LOGI(TAG, "Button released");
        }
        
        if (is_pressed && press_start_time > 0) {
            int64_t press_duration = now - press_start_time;
            
            if (press_duration >= CONFIG_BUTTON_LONG_PRESS_MS) {
                ESP_LOGI(TAG, "Long press detected! Entering BLE provisioning mode...");
                
                // Zatrzymaj WiFi
                wifi_station_stop();
                
                // Uruchom BLE provisioning
                ble_provisioning_start(led);
                
                // Czekaj aż provisioning się zakończy
                while (ble_provisioning_is_active()) {
                    vTaskDelay(pdMS_TO_TICKS(1000));
                }
                
                // Po zakończeniu provisioning, wczytaj nową konfigurację i uruchom WiFi
                WifiConfig cfg;
                if (wifi_config_load(cfg)) {
                    ESP_LOGI(TAG, "Restarting WiFi with new config...");
                    wifi_station_start(led, &cfg);
                } else {
                    ESP_LOGW(TAG, "No config saved, starting with default");
                    wifi_station_start(led, nullptr);
                }
                
                press_start_time = 0;
            }
        }
        
        was_pressed = is_pressed;
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}

extern "C" void app_main(void)
{
    ESP_LOGI(TAG, "=== WiFi LED Project with BLE Provisioning ===");
    
    // Inicjalizacja NVS
    if (!wifi_config_nvs_init()) {
        ESP_LOGE(TAG, "Failed to init NVS!");
        return;
    }
    
    // Inicjalizacja LED controllera
    LEDController led(LED_GPIO, LED_BLINK_PERIOD_MS);
    
    // Sprawdź czy istnieje zapisana konfiguracja WiFi
    WifiConfig cfg;
    bool has_config = wifi_config_load(cfg);
    
    if (!has_config) {
        ESP_LOGI(TAG, "No saved WiFi config found. Starting BLE provisioning...");
        
        // Brak konfiguracji - uruchom BLE provisioning
        ble_provisioning_start(&led);
        
        // Czekaj na zakończenie provisioning
        while (ble_provisioning_is_active()) {
            vTaskDelay(pdMS_TO_TICKS(1000));
        }
        
        // Spróbuj ponownie wczytać konfigurację
        has_config = wifi_config_load(cfg);
        
        if (!has_config) {
            ESP_LOGW(TAG, "Still no config after provisioning. Using defaults.");
        }
    }
    
    // Uruchom WiFi Station
    if (has_config) {
        ESP_LOGI(TAG, "Starting WiFi with saved config: SSID='%s'", cfg.ssid);
        wifi_station_start(&led, &cfg);
    } else {
        ESP_LOGI(TAG, "Starting WiFi with default config");
        wifi_station_start(&led, nullptr);
    }
    
    // Uruchom task monitorujący przycisk (pozwala na zmianę konfiguracji)
    xTaskCreate(button_monitor_task, "btn_monitor", 4096, &led, 5, NULL);
    
    ESP_LOGI(TAG, "System initialized. Hold button for %dms to enter provisioning mode.",
             CONFIG_BUTTON_LONG_PRESS_MS);
    
    while (true) {
        if (wifi_station_is_connected()) {
        }
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}


