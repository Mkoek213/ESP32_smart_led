#ifndef HC_SR04_H
#define HC_SR04_H

#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_attr.h"

/**
 * @brief HC-SR04 Ultrasonic Distance Sensor Controller
 * 
 * Measures distance using ultrasonic pulses (polling-based, no ISR).
 * Distance (cm) = (Echo Time in µs) / 58
 */
class HCSR04 {
public:
    /**
     * @brief Constructor for HC-SR04 sensor
     * @param trig_pin GPIO pin for TRIG (trigger) signal
     * @param echo_pin GPIO pin for ECHO (echo) signal
     */
    HCSR04(gpio_num_t trig_pin, gpio_num_t echo_pin);
    
    /**
     * @brief Initialize the sensor
     * @return true if initialization successful, false otherwise
     */
    bool init();
    
    /**
     * @brief Deinitialize the sensor and cleanup resources
     */
    void deinit();
    
    /**
     * @brief Measure distance in centimeters
     * @return Distance in cm, or -1 if measurement failed
     */
    float measure_distance_cm() IRAM_ATTR;
    
    /**
     * @brief Measure distance in millimeters
     * @return Distance in mm, or -1 if measurement failed
     */
    float measure_distance_mm();
    
    /**
     * @brief Get the last measured distance in cm
     * @return Last distance value in cm
     */
    float get_last_distance_cm() const { return last_distance_cm; }
    
    /**
     * @brief Get the last measured distance in mm
     * @return Last distance value in mm
     */
    float get_last_distance_mm() const { return last_distance_mm; }

private:
    gpio_num_t trig_pin;
    gpio_num_t echo_pin;
    gptimer_handle_t timer_handle;
    
    float last_distance_cm;
    float last_distance_mm;
    
    // Timer value at echo start (kept for future ISR support)
    uint64_t echo_start_time;
    // Flag to indicate echo pulse is high (kept for future ISR support)
    volatile bool echo_pin_high;
    
    /**
     * @brief Send trigger pulse to sensor
     */
    void send_trigger_pulse() IRAM_ATTR;
    
    /**
     * @brief Wait for echo pulse with timeout (polling-based)
     * @return Echo time in microseconds, or 0 if timeout
     */
    uint32_t wait_for_echo() IRAM_ATTR;
    
    /**
     * @brief Convert echo time to distance in cm
     * @param echo_time_us Echo time in microseconds
     * @return Distance in centimeters
     */
    static float echo_time_to_distance_cm(uint32_t echo_time_us);
};

#endif // HC_SR04_H
