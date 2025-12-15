#include "person_counter.h"
#include "freertos/FreeRTOS.h"
#include "freertos/semphr.h"
#include "esp_log.h"

static const char* TAG = "PersonCounter";

// Static member initialization
void* PersonCounter::mutex = nullptr;
int PersonCounter::count = 0;

void PersonCounter::init() {
    if (mutex == nullptr) {
        mutex = xSemaphoreCreateMutex();
        if (mutex == nullptr) {
            ESP_LOGE(TAG, "Failed to create mutex!");
        } else {
            ESP_LOGI(TAG, "PersonCounter initialized");
        }
    }
}

void PersonCounter::increment() {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "PersonCounter not initialized!");
        return;
    }
    
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        count++;
        xSemaphoreGive(sem);
    }
}

int PersonCounter::get_and_reset() {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "PersonCounter not initialized!");
        return 0;
    }
    
    int result = 0;
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        result = count;
        count = 0;
        xSemaphoreGive(sem);
    }
    return result;
}

int PersonCounter::get() {
    if (mutex == nullptr) {
        ESP_LOGE(TAG, "PersonCounter not initialized!");
        return 0;
    }
    
    int result = 0;
    SemaphoreHandle_t sem = static_cast<SemaphoreHandle_t>(mutex);
    if (xSemaphoreTake(sem, portMAX_DELAY) == pdTRUE) {
        result = count;
        xSemaphoreGive(sem);
    }
    return result;
}
