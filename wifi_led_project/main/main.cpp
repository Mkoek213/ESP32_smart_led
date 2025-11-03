#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_log.h"

#include "config.h"
#include "led_controller.h"
#include "http_client.h"
#include "wifi_station.h"
#include "access_point.h"

static const char* TAG = "main";

static void http_get_task(void* pvParameters) {
    const char* TAG_TASK = "HTTP_TASK";
    
    ESP_LOGI(TAG_TASK, "HTTP request task started");
    
    // Periodic HTTP requests with exponential backoff on failure
    int retry_delay_ms = 2000;  // Start with 2 seconds for retries
    const int max_delay_ms = 3600000;  // Max 1 hour (3600000ms)
    const int success_interval_ms = 60000;  // 60 seconds between successful requests
    int attempt = 1;
    
    while (true) {
        // Check WiFi connection before each attempt
        WiFiStation* wifi = WiFiStation::get_instance();
        if (!wifi || !wifi->is_connected()) {
            ESP_LOGW(TAG_TASK, "WiFi disconnected, waiting for connection...");
            retry_delay_ms = 2000;  // Reset delay when disconnected
            attempt = 1;
            vTaskDelay(pdMS_TO_TICKS(1000));  // Check every second when disconnected
            continue;
        }
        
        ESP_LOGI(TAG_TASK, "HTTP request attempt #%d", attempt);
        
        HTTPClient client(HTTP_SERVER, HTTP_PORT, HTTP_PATH, HTTP_RECV_BUF_SIZE);
        bool success = client.perform_get_request();
        
        if (success) {
            ESP_LOGI(TAG_TASK, "HTTP request successful! Next request in %d seconds", 
                     success_interval_ms/1000);
            retry_delay_ms = 2000;  // Reset delay on success
            attempt = 1;
            vTaskDelay(pdMS_TO_TICKS(success_interval_ms));
        } else {
            ESP_LOGW(TAG_TASK, "HTTP request failed, retrying in %d seconds...", retry_delay_ms/1000);
            vTaskDelay(pdMS_TO_TICKS(retry_delay_ms));
            
            // Exponential backoff: double the delay, capped at 1 hour
            retry_delay_ms = retry_delay_ms * 2;
            if (retry_delay_ms > max_delay_ms) {
                retry_delay_ms = max_delay_ms;
            }
            attempt++;
        }
    }
    
    // This should never be reached, but if it is, delete the task
    vTaskDelete(nullptr);
}


extern "C" void app_main(void)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    LEDController* led = new LEDController(LED_GPIO, LED_BLINK_PERIOD_MS);

    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "ESP32 Smart LED - WiFi Station + AP");
    ESP_LOGI(TAG, "Mode: SoftAP + Station (APSTA)");
    ESP_LOGI(TAG, "============================================");
    
    bool enable_ap_mode = true;
    
    WiFiStation* wifi_station = new WiFiStation(led);
    wifi_station->init(enable_ap_mode);
    
    AccessPoint* access_point = nullptr;
    if (enable_ap_mode) {
        access_point = new AccessPoint();
        access_point->init();
        
        ESP_LOGI(TAG, "============================================");
        ESP_LOGI(TAG, "Access Point is ready!");
        ESP_LOGI(TAG, "   Connect to: %s", AP_SSID);
        ESP_LOGI(TAG, "   Password: %s", AP_PASSWORD);
        ESP_LOGI(TAG, "   Open browser: http://192.168.4.1");
        ESP_LOGI(TAG, "============================================");
    }
    
    ESP_LOGI(TAG, "Creating HTTP GET task...");
    xTaskCreate(&http_get_task, "http_get_task", 4096, nullptr, 5, nullptr);
    
    ESP_LOGI(TAG, "Initialization complete!");
}

