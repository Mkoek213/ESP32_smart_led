#include "app_mqtt.h" // For app_mqtt_send_motion_event
#include "sensor_manager.h"
#include "sensor_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_common.h"
#include <ctime>
#include <cstdlib>

static const char* TAG = "sensor_task";
static const int SENSOR_READ_INTERVAL_MS = 2000; // Read every 2 seconds
static const int MOTION_CHECK_INTERVAL_MS = 5000; // Check for motion every 5s

static void sensor_reading_task(void* arg) {
    ESP_LOGI(TAG, "Sensor reading task started");
    
    // Wait for time sync before starting to log timestamps
    xEventGroupWaitBits(
        s_app_event_group,
        TIME_SYNCED_BIT,
        pdFALSE,
        pdTRUE,
        portMAX_DELAY
    );

    ESP_LOGI(TAG, "Time synced, starting data generation");

    while (true) {
        time_t now;
        time(&now);

        Telemetry data;
        data.timestamp = (int64_t)now;
        data.humidity = 30.0 + (rand() % 50);
        data.temperature = 15.0 + (rand() % 20); // 15-35 C
        data.pressure = 980.0 + (rand() % 40);   // 980-1020 hPa
        data.motion_detected = (rand() % 10) == 0; // 10% chance

        SensorManager::getInstance().enqueue(data);
        
        // Optional: log occasionally or verbose
        ESP_LOGD(TAG, "Enqueued sensor data: ts=%" PRIi64 ", motion=%d", data.timestamp, data.motion_detected);

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

void sensor_reading_task_start(void) {
    xTaskCreate(sensor_reading_task, "sensor_read", 4096, NULL, 5, NULL);
}

