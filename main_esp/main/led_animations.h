#ifndef LED_ANIMATIONS_H
#define LED_ANIMATIONS_H

#include "ws2812b_controller.h"

/**
 * Collection of example animations and patterns for WS2812B LED strip
 */

// Example usage functions - call these from your main logic

/**
 * Show a success indicator - green pulse
 */
static inline void led_show_success(WS2812BController& leds) {
    leds.pulse_animation(0, 255, 0, 1000);  // Green, 1 second
}

/**
 * Show an error indicator - red pulse
 */
static inline void led_show_error(WS2812BController& leds) {
    leds.pulse_animation(255, 0, 0, 1000);  // Red, 1 second
}

/**
 * Show a warning indicator - yellow pulse
 */
static inline void led_show_warning(WS2812BController& leds) {
    leds.pulse_animation(255, 255, 0, 1000);  // Yellow, 1 second
}

/**
 * Show WiFi connection status - cyan steady
 */
static inline void led_show_wifi_connected(WS2812BController& leds) {
    leds.set_all_pixels_brightness(0, 255, 255, 200);  // Cyan, ~78% brightness
}

/**
 * Show WiFi disconnected - dim red
 */
static inline void led_show_wifi_disconnected(WS2812BController& leds) {
    leds.set_all_pixels_brightness(255, 0, 0, 100);  // Red, ~39% brightness
}

/**
 * Show MQTT connected - blue steady
 */
static inline void led_show_mqtt_connected(WS2812BController& leds) {
    leds.set_all_pixels_brightness(0, 0, 255, 200);  // Blue, ~78% brightness
}

/**
 * Turn off all LEDs
 */
static inline void led_turn_off(WS2812BController& leds) {
    leds.clear();
}

#endif // LED_ANIMATIONS_H
