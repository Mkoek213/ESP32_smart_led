#ifndef CONFIG_H
#define CONFIG_H

// WiFi Station Configuration
#define EXAMPLE_ESP_WIFI_SSID      "iPhone"
#define EXAMPLE_ESP_WIFI_PASS      "haslo123"
#define EXAMPLE_ESP_MAXIMUM_RETRY  100

// Access Point Configuration
#define AP_SSID                    "ESP32-Config"
#define AP_PASSWORD                "esp32pass"
#define AP_CHANNEL                 1
#define AP_MAX_CONNECTIONS         4

// LED Configuration
#define LED_GPIO                   GPIO_NUM_2
#define LED_BLINK_PERIOD_MS        500

// HTTP Client Configuration
#define HTTP_SERVER                "172.20.10.7"
#define HTTP_PORT                  5000
#define HTTP_PATH                  "/"
#define HTTP_RECV_BUF_SIZE         512

// Web Server Configuration
#define WEB_SERVER_PORT            80

#endif // CONFIG_H

