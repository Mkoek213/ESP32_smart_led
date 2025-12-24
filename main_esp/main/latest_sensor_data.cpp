#include "latest_sensor_data.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char* TAG = "LatestSensorData";

// Static member initialization
void* LatestSensorData::mutex = nullptr;
float LatestSensorData::temperature = 22.0f;
float LatestSensorData::humidity = 50.0f;
float LatestSensorData::pressure = 1013.25f;
bool LatestSensorData::data_available = false;

void LatestSensorData::init() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateMutex();
        if (mutex == nullptr) {
            ESP_LOGE(TAG, "Failed to create mutex!");
        } else {
            ESP_LOGI(TAG, "LatestSensorData initialized");
        }
    }
}

void LatestSensorData::update(float temp, float humid, float press) {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "LatestSensorData not initialized!");
        return;
    }
    
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        temperature = temp;
        humidity = humid;
        pressure = press;
        data_available = true;
        xSemaphoreGive(sem);
        ESP_LOGI(TAG, "Updated: T=%.2f°C H=%.2f%% P=%.2fhPa", temp, humid, press);
    }
}

float LatestSensorData::get_temperature() {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "LatestSensorData not initialized!");
        return 22.0f;
    }
    
    float result = 22.0f;
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        result = temperature;
        xSemaphoreGive(sem);
    }
    return result;
}

float LatestSensorData::get_humidity() {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "LatestSensorData not initialized!");
        return 50.0f;
    }
    
    float result = 50.0f;
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        result = humidity;
        xSemaphoreGive(sem);
    }
    return result;
}

float LatestSensorData::get_pressure() {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "LatestSensorData not initialized!");
        return 1013.25f;
    }
    
    float result = 1013.25f;
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        result = pressure;
        xSemaphoreGive(sem);
    }
    return result;
}

bool LatestSensorData::has_data() {
    if (mutex == nullptr) {
        return false;
    }
    
    bool result = false;
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        result = data_available;
        xSemaphoreGive(sem);
    }
    return result;
}
