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
#define WS2812B_GPIO GPIO_NUM_4 // D4 pin
#define WS2812B_NUM_LEDS 5

// Konfiguracja HC-SR04 Ultrasonic Distance Sensor
#define HC_SR04_TRIG_GPIO GPIO_NUM_5  // D5 pin (Trigger)
#define HC_SR04_ECHO_GPIO GPIO_NUM_18 // GPIO18 (Echo)

// Konfiguracja przycisku do wejścia w tryb provisioning, BOOT button
#define CONFIG_BUTTON_GPIO GPIO_NUM_0
#define CONFIG_BUTTON_LONG_PRESS_MS 3000 // 3 sekundy naciśnięcia

// MQTT Configuration
// IDs are no longer hardcoded; MAC address is used dynamically

// Topic Base: smart-led/device/{mac_address} which we append suffixes to
#define MQTT_TOPIC_BASE "smart-led/device/"

// Topic Suffixes
#define MQTT_TOPIC_SUFFIX_TELEMETRY "/telemetry"
#define MQTT_TOPIC_SUFFIX_STATUS "/status"
#define MQTT_TOPIC_SUFFIX_CMD "/cmd"
#define MQTT_TOPIC_SUFFIX_RESP "/resp"
#define MQTT_TOPIC_SUFFIX_ATTRIBUTES "/attributes"

// Sensor Configuration
#define USE_REAL_PHOTORESISTOR 1         // Use real photoresistor on GPIO34
#define TELEMETRY_SEND_INTERVAL_MS 30000 // Send telemetry every 30 seconds

// LED Auto-off Configuration
#define LED_NO_MOTION_TIMEOUT_MS 15000 // Turn off after 15 seconds of no motion
#define LED_MAX_DURATION_MS 300000     // Maximum 5 minutes on time

// Photoresistor (Light Sensor) Configuration
#define PHOTORESISTOR_GPIO                                                     \
  GPIO_NUM_34 // ADC1_CHANNEL_6, input-only, WiFi-compatible
#define PHOTORESISTOR_ADC_CHANNEL ADC_CHANNEL_6
#define PHOTORESISTOR_ADC_ATTEN                                                \
  ADC_ATTEN_DB_12 // 0-3.3V range (updated from deprecated DB_11)
#define PHOTORESISTOR_ADC_BITWIDTH ADC_BITWIDTH_12 // 12-bit: 0-4095

// Photoresistor calibration values (based on actual measurements)
#define PHOTORESISTOR_MIN_ADC 1700 // Dark/covered (measured ~1711)
#define PHOTORESISTOR_MAX_ADC 4095 // Normal daylight (saturated max)

// BMP280 I2C Configuration
#define I2C_MASTER_SCL_IO GPIO_NUM_22
#define I2C_MASTER_SDA_IO GPIO_NUM_21
#define I2C_MASTER_FREQ_HZ 100000
#define I2C_MASTER_NUM I2C_NUM_0

// Backend Configuration (UPDATE THIS WITH YOUR BACKEND URL)
#define BACKEND_URL                                                            \
  "http://192.168.0.186:8080" // Change this to your actual backend URL
#define MQTT_BROKER_URL                                                        \
  "mqtt://192.168.0.186:1883" // Default MQTT broker URL (Fallback)
#define DEVICE_CONFIG_URL BACKEND_URL "/api/devices/mac/%s/config"

#endif
