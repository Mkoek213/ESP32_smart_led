#include "led_controller.h"
#include "esp_log.h"

static const char* TAG = "LED_CONTROLLER";

LEDController::LEDController(gpio_num_t gpio_pin, int period_ms) 
    : pin(gpio_pin), blink_period_ms(period_ms), task_handle(nullptr) {
    gpio_reset_pin(pin);
    gpio_set_direction(pin, GPIO_MODE_OUTPUT);
    gpio_set_level(pin, 0);
    ESP_LOGI(TAG, "LED initialized on GPIO %d", pin);
}

LEDController::~LEDController() {
    stop_blinking();
}

void LEDController::blink_task_wrapper(void* arg) {
    LEDController* controller = static_cast<LEDController*>(arg);
    controller->blink_task();
}

void LEDController::blink_task() {
    ESP_LOGI(TAG, "LED blink task started - waiting for WiFi connection");
    while (true) {
        gpio_set_level(pin, 1);
        vTaskDelay(pdMS_TO_TICKS(blink_period_ms));
        gpio_set_level(pin, 0);
        vTaskDelay(pdMS_TO_TICKS(blink_period_ms));
    }
}

void LEDController::start_blinking() {
    if (task_handle == nullptr) {
        xTaskCreate(blink_task_wrapper, "led_blink_task", 2048, this, 5, &task_handle);
    }
}

void LEDController::stop_blinking() {
    if (task_handle != nullptr) {
        vTaskDelete(task_handle);
        task_handle = nullptr;
        gpio_set_level(pin, 0);
        ESP_LOGI(TAG, "LED blink task stopped - WiFi connected");
    }
}

