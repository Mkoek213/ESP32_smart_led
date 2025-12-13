#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_pm.h"

#include "ble_provisioning.h"
#include "sensor_task.h"
#include "config.h"
#include "led_controller.h"
#include "wifi_config.h"
#include "wifi_station.h"

extern "C" {
#include "app_common.h"
#include "app_mqtt.h"
#include "app_sntp.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
}

static const char *TAG = "main";

// NimBLE callbacks
static void on_reset(int reason) {
    ESP_LOGE(TAG, "BLE stack resetting; reason=%d", reason);
}

static void on_sync(void) {
    ESP_LOGI(TAG, "BLE stack synced");
    xEventGroupSetBits(s_app_event_group, BLE_STACK_READY_BIT);
}

static void host_task(void *param) {
    ESP_LOGI(TAG, "BLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Tracking MQTT/SNTP state - accessible from button monitor task
static volatile bool mqtt_started = false;
static volatile bool sntp_started = false;

// Task monitorujący przycisk konfiguracyjny
static void button_monitor_task(void *arg) {
  LEDController *led = static_cast<LEDController *>(arg);

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

        // Stop MQTT and SNTP
        if (mqtt_started) {
          ESP_LOGI(TAG, "Stopping MQTT...");
          app_mqtt_stop();
          mqtt_started = false;
        }
        if (sntp_started) {
          ESP_LOGI(TAG, "Stopping SNTP...");
          app_sntp_stop();
          sntp_started = false;
        }

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

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "=== WiFi LED Project with BLE Provisioning ===");

  // Initialize global event group for MQTT/WiFi coordination
  s_app_event_group = xEventGroupCreate();
  if (s_app_event_group == NULL) {
    ESP_LOGE(TAG, "Failed to create event group!");
    return;
  }

  // Inicjalizacja NVS
  if (!wifi_config_nvs_init()) {
    ESP_LOGE(TAG, "Failed to init NVS!");
    return;
  }

  // Inicjalizacja MQTT client
  ESP_LOGI(TAG, "Initializing MQTT...");
  app_mqtt_init();

  // Inicjalizacja BLE Stack (NimBLE)
  ESP_LOGI(TAG, "Initializing BLE Stack...");
  ESP_ERROR_CHECK(nimble_port_init());
  ble_hs_cfg.reset_cb = on_reset;
  ble_hs_cfg.sync_cb = on_sync;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_svc_gap_device_name_set("ESP32_LED_Controller"); // Default name, overridden by provisioning or app

  // Zarejestruj serwisy provisioning (muszą być dostępne zawsze)
  ble_provisioning_init_services();
  
  // Start BLE host task
  nimble_port_freertos_init(host_task);

  // Wait for BLE stack to be ready
  ESP_LOGI(TAG, "Waiting for BLE stack sync...");
  xEventGroupWaitBits(s_app_event_group, BLE_STACK_READY_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "BLE Stack ready");

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
  
  // Start sensor reading task (independent of WiFi connection)
  sensor_reading_task_start();
  
  ESP_LOGI(
      TAG,
      "System initialized. Hold button for %dms to enter provisioning mode.",
      CONFIG_BUTTON_LONG_PRESS_MS);

  while (true) {
    if (wifi_station_is_connected()) {
      // Start SNTP if not already started
      if (!sntp_started) {
        ESP_LOGI(TAG, "WiFi connected - starting SNTP...");
        app_sntp_init();
        sntp_started = true;
      }

      // Start MQTT if not already started
      if (!mqtt_started) {
        // Give network stack a moment to stabilize (ARP, etc.) to avoid initial
        // connection timeout
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "WiFi connected - starting MQTT...");
        app_mqtt_start();
        app_mqtt_start_publishing_task();
        mqtt_started = true;
      }
    } else {
      // WiFi disconnected - reset state
      if (mqtt_started || sntp_started) {
        ESP_LOGI(TAG, "WiFi disconnected - resetting MQTT/SNTP state");
        mqtt_started = false;
        sntp_started = false;
      }
    }

    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}
