#include "led_controller.h"

LEDController::LEDController(gpio_num_t pin, int period_ms)
    : pin_(pin), period_ms_(period_ms), task_(nullptr) {
    gpio_reset_pin(pin_);
    gpio_set_direction(pin_, GPIO_MODE_OUTPUT);
    gpio_set_level(pin_, 0);
}

void LEDController::start_blinking() {
    if (task_ == nullptr) {
        xTaskCreate(task_entry, "led_blink", 1024, this, 1, &task_);
    }
}

void LEDController::stop_blinking() {
    if (task_ != nullptr) {
        vTaskDelete(task_);
        task_ = nullptr;
        gpio_set_level(pin_, 0);
    }
}

void LEDController::task_entry(void* arg) {
    static_cast<LEDController*>(arg)->run();
}

void LEDController::run() {
    while (true) {
        gpio_set_level(pin_, 1);
        vTaskDelay(pdMS_TO_TICKS(period_ms_));
        gpio_set_level(pin_, 0);
        vTaskDelay(pdMS_TO_TICKS(period_ms_));
    }
}

