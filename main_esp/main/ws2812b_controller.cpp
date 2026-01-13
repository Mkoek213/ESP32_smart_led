#include "ws2812b_controller.h"
#include "esp_log.h"

static const char *TAG = "WS2812B";

WS2812BController::WS2812BController(gpio_num_t pin, uint32_t num_leds)
    : pin_(pin), num_leds_(num_leds), led_strip_(nullptr), animation_task_(nullptr), stop_animation_(false) {
}

WS2812BController::~WS2812BController() {
    if (led_strip_ != nullptr) {
        led_strip_del(led_strip_);
    }
}

bool WS2812BController::init() {
    led_strip_config_t strip_config = {
        .strip_gpio_num = pin_,
        .max_leds = num_leds_,
        .led_model = LED_MODEL_WS2812,
    };

    led_strip_rmt_config_t rmt_config = {
        .clk_src = RMT_CLK_SRC_DEFAULT,
        .resolution_hz = 10 * 1000 * 1000,
        .mem_block_symbols = 128,
    };

    esp_err_t err = led_strip_new_rmt_device(&strip_config, &rmt_config, &led_strip_);
    if (err != ESP_OK || !led_strip_) {
        ESP_LOGE(TAG, "LED init failed: %d", err);
        return false;
    }

    ESP_LOGI(TAG, "LED strip initialized on GPIO%d with %lu LEDs", pin_, num_leds_);
    clear();
    led_strip_refresh(led_strip_);
    return true;
}

void WS2812BController::set_pixel(uint32_t index, uint32_t red, uint32_t green, uint32_t blue) {
    if (index < num_leds_ && led_strip_) {
        led_strip_set_pixel(led_strip_, index, red, green, blue);
    }
}

void WS2812BController::set_pixel_brightness(uint32_t index, uint32_t red, uint32_t green, uint32_t blue, uint8_t brightness) {
    if (index < num_leds_ && led_strip_) {
        uint32_t r = (red * brightness) / 255;
        uint32_t g = (green * brightness) / 255;
        uint32_t b = (blue * brightness) / 255;
        led_strip_set_pixel(led_strip_, index, r, g, b);
    }
}

void WS2812BController::clear() {
    if (led_strip_) {
        led_strip_clear(led_strip_);
    }
}

void WS2812BController::refresh() {
    if (led_strip_) {
        led_strip_refresh(led_strip_);
    }
}

void WS2812BController::power_up_animation(uint32_t duration_ms) {
    if (animation_task_ == nullptr) {
        stop_animation_ = false;
        xTaskCreatePinnedToCore(animation_task_entry, "ws2812b_anim", 2048, this, 5, &animation_task_, 1);
    }
}

void WS2812BController::stop_animation() {
    stop_animation_ = true;
    while (animation_task_ != nullptr) {
        vTaskDelay(pdMS_TO_TICKS(10));
    }
}

void WS2812BController::restart_animation() {
    if (animation_task_ == nullptr) {
        stop_animation_ = false;
        xTaskCreatePinnedToCore(animation_task_entry, "ws2812b_anim", 2048, this, 5, &animation_task_, 1);
    }
}

void WS2812BController::set_all_pixels(uint32_t red, uint32_t green, uint32_t blue) {
    if (!led_strip_) {
        ESP_LOGW(TAG, "LED strip not initialized!");
        return;
    }
    ESP_LOGI(TAG, "Setting all LEDs to R:%lu G:%lu B:%lu", red, green, blue);
    for (uint32_t i = 0; i < num_leds_; i++) {
        set_pixel(i, red, green, blue);
    }
    refresh();
}

void WS2812BController::set_all_pixels_brightness(uint32_t red, uint32_t green, uint32_t blue, uint8_t brightness) {
    for (uint32_t i = 0; i < num_leds_; i++) {
        set_pixel_brightness(i, red, green, blue, brightness);
    }
    refresh();
}

void WS2812BController::pulse_animation(uint32_t red, uint32_t green, uint32_t blue, uint32_t duration_ms) {
    int steps = 5;
    int delay = duration_ms / (2 * steps);
    for (int s = 0; s <= steps; s++) {
        uint8_t br = (s * 255) / steps;
        set_all_pixels_brightness(red, green, blue, br);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
    for (int s = steps; s > 0; s--) {
        uint8_t br = (s * 255) / steps;
        set_all_pixels_brightness(red, green, blue, br);
        vTaskDelay(pdMS_TO_TICKS(delay));
    }
    clear();
}

void WS2812BController::color_brightness_cycle() {
    // Color sequence: Red, Blue, Green
    const struct {
        uint8_t r, g, b;
    } colors[] = {
        {255, 0, 0},    // Red
        {0, 0, 255},    // Blue
        {0, 255, 0},    // Green
    };
    
    const uint8_t brightness_levels[] = {85, 170, 255};  // 3 different brightness levels (roughly 1/3, 2/3, full)
    const int num_colors = 3;
    const int num_brightness = 3;
    const int brightness_delay_ms = 3333;  // ~3.3 seconds per brightness level (10 seconds / 3 levels)
    const int check_interval_ms = 50;  // Check stop flag every 50ms for quick response
    
    while (!stop_animation_) {
        // Cycle through each color
        for (int c = 0; c < num_colors && !stop_animation_; c++) {
            // For each color, show 3 different brightness levels
            for (int b = 0; b < num_brightness && !stop_animation_; b++) {
                set_all_pixels_brightness(colors[c].r, colors[c].g, colors[c].b, brightness_levels[b]);
                
                // Break delay into smaller chunks to check stop flag frequently
                int remaining_ms = brightness_delay_ms;
                while (remaining_ms > 0 && !stop_animation_) {
                    int delay = (remaining_ms > check_interval_ms) ? check_interval_ms : remaining_ms;
                    vTaskDelay(pdMS_TO_TICKS(delay));
                    remaining_ms -= delay;
                }
            }
        }
    }
    
    clear();
    refresh();
}

void WS2812BController::animation_task_entry(void* arg) {
    static_cast<WS2812BController*>(arg)->run_animation();
}

void WS2812BController::run_animation() {
    // Use the color brightness cycle animation
    color_brightness_cycle();
    animation_task_ = nullptr;
    vTaskDelete(nullptr);
}
