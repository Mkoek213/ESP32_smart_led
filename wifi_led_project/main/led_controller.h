#ifndef LED_CONTROLLER_H
#define LED_CONTROLLER_H

#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

/**
 * @brief LED Controller Class
 * Manages LED blinking for WiFi connection status indication
 */
class LEDController {
private:
    gpio_num_t pin;
    int blink_period_ms;
    TaskHandle_t task_handle;
    
    static void blink_task_wrapper(void* arg);
    void blink_task();
    
public:
    LEDController(gpio_num_t gpio_pin, int period_ms);
    ~LEDController();
    
    void start_blinking();
    void stop_blinking();
};

#endif // LED_CONTROLLER_H

