#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class LEDController {
public:
    LEDController(gpio_num_t pin, int period_ms);
    void start_blinking();
    void stop_blinking();

private:
    gpio_num_t pin_;
    int period_ms_;
    TaskHandle_t task_;
    volatile bool stop_requested_;

    static void task_entry(void* arg);
    void run();
};

#endif

