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
    
    ESP_LOGI(TAG_TASK, "Waiting for WiFi connection before HTTP request...");
    while (true) {
        WiFiStation* wifi = WiFiStation::get_instance();
        if (wifi && wifi->is_connected()) {
            ESP_LOGI(TAG_TASK, "WiFi connected, starting HTTP request");
            break;
        }
        ESP_LOGD(TAG_TASK, "WiFi not connected, waiting...");
        vTaskDelay(pdMS_TO_TICKS(1000)); 
    }
    
    // Try HTTP request with retries
    const int max_retries = 5;
    const int retry_delay_ms = 5000;
    bool success = false;
    
    for (int attempt = 1; attempt <= max_retries; attempt++) {
        ESP_LOGI(TAG_TASK, "HTTP request attempt %d/%d", attempt, max_retries);
        
        HTTPClient client(HTTP_SERVER, HTTP_PORT, HTTP_PATH, HTTP_RECV_BUF_SIZE);
        success = client.perform_get_request();
        
        if (success) {
            ESP_LOGI(TAG_TASK, "HTTP request successful!");
            break;
        }
        
        if (attempt < max_retries) {
            ESP_LOGW(TAG_TASK, "HTTP request failed, retrying in %d seconds...", retry_delay_ms/1000);
            vTaskDelay(pdMS_TO_TICKS(retry_delay_ms));
        }
    }
    
    if (!success) {
        ESP_LOGE(TAG_TASK, "HTTP request failed after %d attempts", max_retries);
    }
    
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

