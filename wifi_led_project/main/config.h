#ifndef CONFIG_H
#define CONFIG_H

#include "driver/gpio.h"
// Domyślne ustawienia WiFi
#define EXAMPLE_ESP_WIFI_SSID ""
#define EXAMPLE_ESP_WIFI_PASS ""

// Konfiguracja LED
#define LED_GPIO GPIO_NUM_2
#define LED_BLINK_PERIOD_MS 500

// Konfiguracja przycisku do wejścia w tryb provisioning, BOOT button
#define CONFIG_BUTTON_GPIO GPIO_NUM_0
#define CONFIG_BUTTON_LONG_PRESS_MS 3000  // 3 sekundy naciśnięcia

#endif
