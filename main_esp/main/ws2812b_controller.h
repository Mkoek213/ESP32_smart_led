#ifndef WS2812B_CONTROLLER_H
#define WS2812B_CONTROLLER_H

#include "driver/gpio.h"
#include "led_strip.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

class WS2812BController {
public:
    WS2812BController(gpio_num_t pin, uint32_t num_leds);
    ~WS2812BController();
    
    bool init();
    void set_pixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue);
    void set_pixel_brightness(uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint8_t brightness);
    void clear();
    void refresh();
    
    // Power up animation - cycles through colors and brightness levels
    void power_up_animation(uint32_t duration_ms = 2000);
    void stop_animation();
    void restart_animation();
    
    // Helper methods for common patterns
    void set_all_pixels(uint32_t red, uint32_t green, uint32_t blue);
    void set_all_pixels_brightness(uint32_t red, uint32_t green, uint32_t blue, uint8_t brightness);
    void pulse_animation(uint32_t red, uint32_t green, uint32_t blue, uint32_t duration_ms);
    
    // Color cycling animation with different brightness levels
    void color_brightness_cycle();
    
private:
    gpio_num_t pin_;
    uint32_t num_leds_;
    led_strip_handle_t led_strip_;
    TaskHandle_t animation_task_;
    volatile bool stop_animation_;
    
    static void animation_task_entry(void* arg);
    void run_animation();
};

#endif // WS2812B_CONTROLLER_H
