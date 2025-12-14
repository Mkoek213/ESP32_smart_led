#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
// Domyślne ustawienia WiFi
#define EXAMPLE_ESP_WIFI_SSID ""
#define EXAMPLE_ESP_WIFI_PASS ""

// Konfiguracja LED
#define LED_GPIO GPIO_NUM_2
#define LED_BLINK_PERIOD_MS 500

// Konfiguracja WS2812B (NeoPixel)
#define WS2812B_GPIO GPIO_NUM_4  // D4 pin
#define WS2812B_NUM_LEDS 6

// Konfiguracja HC-SR04 Ultrasonic Distance Sensor
#define HC_SR04_TRIG_GPIO GPIO_NUM_5  // D5 pin (Trigger)
#define HC_SR04_ECHO_GPIO GPIO_NUM_18 // GPIO18 (Echo)

// Konfiguracja przycisku do wejścia w tryb provisioning, BOOT button
#define CONFIG_BUTTON_GPIO GPIO_NUM_0
#define CONFIG_BUTTON_LONG_PRESS_MS 3000  // 3 sekundy naciśnięcia

// MQTT Configuration
#define MQTT_CUSTOMER_ID "1"
#define MQTT_LOCATION_ID "1"
#define MQTT_DEVICE_ID "1"

// Topic Base: customer/{custId}/location/{locId}/device/{deviceId}
#define MQTT_TOPIC_BASE "customer/" MQTT_CUSTOMER_ID "/location/" MQTT_LOCATION_ID "/device/" MQTT_DEVICE_ID

// Topic Suffixes
#define MQTT_TOPIC_SUFFIX_TELEMETRY "/telemetry"
#define MQTT_TOPIC_SUFFIX_STATUS "/status"
#define MQTT_TOPIC_SUFFIX_CMD "/cmd"
#define MQTT_TOPIC_SUFFIX_RESP "/resp"
#define MQTT_TOPIC_SUFFIX_ATTRIBUTES "/attributes"


#endif
