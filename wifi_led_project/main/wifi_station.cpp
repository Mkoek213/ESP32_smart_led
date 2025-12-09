#include "wifi_station.h"
#include "wifi_config.h"
#include "config.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_wifi.h"
#include <cstring>
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

static LEDController* led_controller = nullptr;
static EventGroupHandle_t s_wifi_event_group = NULL;
static const int WIFI_CONNECTED_BIT = BIT0;
static const char* TAG = "wifi_station";
static esp_netif_t* s_sta_netif = nullptr;

static void handle_event(void*,
                         esp_event_base_t event_base,
                         int32_t event_id,
                         void*) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        if (s_wifi_event_group) {
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        if (led_controller != nullptr) {
            led_controller->start_blinking();
        }
        esp_wifi_connect();
    }

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        if (s_wifi_event_group) {
            xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        if (led_controller != nullptr) {
            led_controller->start_blinking();
        }
        ESP_LOGI(TAG, "disconnected");
        esp_wifi_connect();
    }

    if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        if (s_wifi_event_group) {
            xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
        }
        if (led_controller != nullptr) {
            led_controller->stop_blinking();
        }
        ESP_LOGI(TAG, "connected");
    }
}

void wifi_station_start(LEDController* led, const WifiConfig* cfg) {
    led_controller = led;

    if (s_wifi_event_group == NULL) {
        s_wifi_event_group = xEventGroupCreate();
    }

    ESP_ERROR_CHECK(esp_netif_init());
    {
        esp_err_t err = esp_event_loop_create_default();
        if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
            ESP_ERROR_CHECK(err);
        }
    }

    if (s_sta_netif == nullptr) {
        s_sta_netif = esp_netif_create_default_wifi_sta();
    }

    wifi_init_config_t wifi_cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&wifi_cfg));

    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &handle_event, nullptr));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_event, nullptr));

    wifi_config_t wifi_config = {};
    
    if (cfg != nullptr && strlen(cfg->ssid) > 0) {
        // Użyj konfiguracji z parametru
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                     cfg->ssid,
                     sizeof(wifi_config.sta.ssid));
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                     cfg->password,
                     sizeof(wifi_config.sta.password));
        ESP_LOGI(TAG, "Starting WiFi with saved config: SSID='%s'", cfg->ssid);
    } else {
        // Użyj domyślnej konfiguracji z config.h
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid),
                     EXAMPLE_ESP_WIFI_SSID,
                     sizeof(wifi_config.sta.ssid));
        std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password),
                     EXAMPLE_ESP_WIFI_PASS,
                     sizeof(wifi_config.sta.password));
        ESP_LOGI(TAG, "Starting WiFi with default config");
    }

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());
}

bool wifi_station_is_connected() {
    if (s_wifi_event_group != NULL) {
        EventBits_t bits = xEventGroupGetBits(s_wifi_event_group);
        return (bits & WIFI_CONNECTED_BIT) != 0;
    }
    return false;
}

void wifi_station_stop() {
    if (s_wifi_event_group != NULL) {
        xEventGroupClearBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    
    esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &handle_event);
    esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &handle_event);
    
    esp_wifi_stop();
    esp_wifi_deinit();
    
    if (s_sta_netif != nullptr) {
        esp_netif_destroy(s_sta_netif);
        s_sta_netif = nullptr;
    }
    
    ESP_LOGI(TAG, "WiFi station stopped");
}