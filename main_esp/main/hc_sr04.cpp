#include "hc_sr04.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_rom_sys.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/gptimer.h"

static const char *TAG = "HC_SR04";

// Ultrasonic speed: 343 m/s = 0.0343 cm/µs = 58 µs per cm (roundtrip)
#define DISTANCE_CONVERSION_FACTOR 58.0f

// Maximum measurement distance (400 cm for HC-SR04)
#define MAX_DISTANCE_CM 400.0f

// Timeout for echo pulse (in milliseconds)
#define ECHO_TIMEOUT_MS 30

HCSR04::HCSR04(gpio_num_t trig_pin, gpio_num_t echo_pin) 
    : trig_pin(trig_pin), 
      echo_pin(echo_pin), 
      timer_handle(nullptr),
      last_distance_cm(-1),
      last_distance_mm(-1),
      echo_start_time(0),
      echo_pin_high(false) {
}

bool HCSR04::init() {
    ESP_LOGI(TAG, "Initializing HC-SR04 sensor on TRIG=%d, ECHO=%d", trig_pin, echo_pin);
    
    // Configure TRIG pin as output
    gpio_config_t trig_cfg = {
        .pin_bit_mask = 1ULL << trig_pin,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (gpio_config(&trig_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure TRIG pin");
        return false;
    }
    
    // Configure ECHO pin as input (no interrupt - we'll poll it)
    gpio_config_t echo_cfg = {
        .pin_bit_mask = 1ULL << echo_pin,
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE
    };
    
    if (gpio_config(&echo_cfg) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to configure ECHO pin");
        return false;
    }
    
    // Initialize general-purpose timer for measuring echo pulse duration
    gptimer_config_t timer_cfg = {
        .clk_src = GPTIMER_CLK_SRC_DEFAULT,
        .direction = GPTIMER_COUNT_UP,
        .resolution_hz = 1000000,  // 1 MHz resolution = 1 microsecond per count
        .intr_priority = 0,
        .flags = {0}
    };
    
    if (gptimer_new_timer(&timer_cfg, &timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create timer");
        return false;
    }
    
    if (gptimer_enable(timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to enable timer");
        return false;
    }
    
    if (gptimer_start(timer_handle) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start timer");
        return false;
    }
    
    // Set initial trigger pin state to low
    gpio_set_level(trig_pin, 0);
    
    ESP_LOGI(TAG, "HC-SR04 sensor initialized successfully");
    return true;
}

void HCSR04::deinit() {
    ESP_LOGI(TAG, "Deinitializing HC-SR04 sensor");
    
    if (timer_handle != nullptr) {
        gptimer_stop(timer_handle);
        gptimer_disable(timer_handle);
        gptimer_del_timer(timer_handle);
        timer_handle = nullptr;
    }
}

void IRAM_ATTR HCSR04::send_trigger_pulse() {
    // HC-SR04 requires a 10 microsecond pulse on TRIG pin
    gpio_set_level(trig_pin, 1);
    esp_rom_delay_us(10);  // 10 microsecond pulse
    gpio_set_level(trig_pin, 0);
}

uint32_t IRAM_ATTR HCSR04::wait_for_echo() {
    // Wait for rising edge (timeout in milliseconds)
    int64_t timeout_start = esp_timer_get_time() / 1000;  // Convert to ms
    
    while ((esp_timer_get_time() / 1000 - timeout_start) < ECHO_TIMEOUT_MS) {
        if (gpio_get_level(echo_pin) == 1) {
            // Rising edge detected - start measuring
            break;
        }
    }
    
    // Start timer measurement at rising edge
    uint64_t pulse_start;
    gptimer_get_raw_count(timer_handle, &pulse_start);
    
    // Wait for falling edge
    timeout_start = esp_timer_get_time() / 1000;
    while ((esp_timer_get_time() / 1000 - timeout_start) < ECHO_TIMEOUT_MS) {
        if (gpio_get_level(echo_pin) == 0) {
            // Falling edge detected
            uint64_t pulse_end;
            gptimer_get_raw_count(timer_handle, &pulse_end);
            
            uint32_t duration_us = (uint32_t)(pulse_end - pulse_start);
            return duration_us;
        }
    }
    
    ESP_LOGW(TAG, "Echo timeout - no response from sensor");
    return 0;
}

float HCSR04::echo_time_to_distance_cm(uint32_t echo_time_us) {
    if (echo_time_us == 0) {
        return -1.0f;
    }
    
    // Distance = (Speed of Sound * Time) / 2
    // Distance (cm) = echo_time_us / 58
    float distance = (float)echo_time_us / DISTANCE_CONVERSION_FACTOR;
    
    // Validate measurement
    if (distance < 2.0f || distance > MAX_DISTANCE_CM) {
        ESP_LOGW(TAG, "Measurement out of range: %.1f cm (echo_time: %u µs)", distance, echo_time_us);
        return -1.0f;
    }
    
    return distance;
}

float IRAM_ATTR HCSR04::measure_distance_cm() {
    // Send trigger pulse
    send_trigger_pulse();
    
    // Wait a bit for sensor to respond
    vTaskDelay(pdMS_TO_TICKS(1));
    
    // Measure echo pulse
    uint32_t echo_time_us = wait_for_echo();
    
    if (echo_time_us == 0) {
        last_distance_cm = -1.0f;
        last_distance_mm = -1.0f;
        return -1.0f;
    }
    
    last_distance_cm = echo_time_to_distance_cm(echo_time_us);
    if (last_distance_cm > 0) {
        last_distance_mm = last_distance_cm * 10.0f;
    } else {
        last_distance_mm = -1.0f;
    }
    
    return last_distance_cm;
}

float HCSR04::measure_distance_mm() {
    float distance_cm = measure_distance_cm();
    return (distance_cm > 0) ? distance_cm * 10.0f : -1.0f;
}
