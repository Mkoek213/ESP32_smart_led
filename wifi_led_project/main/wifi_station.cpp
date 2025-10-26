#include "wifi_station.h"
#include "config.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_netif_defaults.h"
#include <cstring>

static const char* TAG = "wifi station";

WiFiStation* WiFiStation::instance = nullptr;

WiFiStation::WiFiStation(LEDController* led_controller) 
    : retry_num(0), connected(false), led(led_controller) {
    wifi_event_group = xEventGroupCreate();
    instance = this;
}

WiFiStation::~WiFiStation() {
    if (wifi_event_group) {
        vEventGroupDelete(wifi_event_group);
    }
}

void WiFiStation::event_handler_wrapper(void* arg, esp_event_base_t event_base,
                                       int32_t event_id, void* event_data) {
    if (instance != nullptr) {
        instance->event_handler(event_base, event_id, event_data);
    }
}

void WiFiStation::event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started, attempting initial connection...");
        esp_wifi_connect();
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconnected = 
            static_cast<wifi_event_sta_disconnected_t*>(event_data);
        
        log_disconnection(disconnected);
        
        connected = false;
        if (led != nullptr) {
            led->start_blinking();
        }
        
        retry_num++;
        ESP_LOGI(TAG, "Reconnection attempt #%d - Trying to reconnect to AP '%s'...", 
                 retry_num, EXAMPLE_ESP_WIFI_SSID);
        esp_wifi_connect();
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = static_cast<ip_event_got_ip_t*>(event_data);
        
        log_connection(event);
        
        connected = true;
        if (led != nullptr) {
            led->stop_blinking();
        }
        
        retry_num = 0;
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void WiFiStation::log_disconnection(wifi_event_sta_disconnected_t* disconnected) {
    ESP_LOGW(TAG, "============================================");
    ESP_LOGW(TAG, "WiFi DISCONNECTED!");
    ESP_LOGW(TAG, "SSID: %s", disconnected->ssid);
    ESP_LOGW(TAG, "Reason code: %d", disconnected->reason);
    
    switch(disconnected->reason) {
        case WIFI_REASON_UNSPECIFIED:
            ESP_LOGW(TAG, "Reason: Unspecified");
            break;
        case WIFI_REASON_AUTH_EXPIRE:
            ESP_LOGW(TAG, "Reason: Authentication expired");
            break;
        case WIFI_REASON_AUTH_LEAVE:
            ESP_LOGW(TAG, "Reason: Deauthenticated (left network)");
            break;
        case WIFI_REASON_ASSOC_EXPIRE:
            ESP_LOGW(TAG, "Reason: Association expired");
            break;
        case WIFI_REASON_ASSOC_TOOMANY:
            ESP_LOGW(TAG, "Reason: Too many associated stations");
            break;
        case WIFI_REASON_NOT_AUTHED:
            ESP_LOGW(TAG, "Reason: Not authenticated");
            break;
        case WIFI_REASON_NOT_ASSOCED:
            ESP_LOGW(TAG, "Reason: Not associated");
            break;
        case WIFI_REASON_ASSOC_LEAVE:
            ESP_LOGW(TAG, "Reason: Disassociated");
            break;
        case WIFI_REASON_ASSOC_NOT_AUTHED:
            ESP_LOGW(TAG, "Reason: Not authenticated");
            break;
        case WIFI_REASON_BEACON_TIMEOUT:
            ESP_LOGW(TAG, "Reason: Beacon timeout (out of range?)");
            break;
        case WIFI_REASON_NO_AP_FOUND:
            ESP_LOGW(TAG, "Reason: AP not found");
            break;
        case WIFI_REASON_AUTH_FAIL:
            ESP_LOGW(TAG, "Reason: Authentication failed (wrong password?)");
            break;
        case WIFI_REASON_HANDSHAKE_TIMEOUT:
            ESP_LOGW(TAG, "Reason: Handshake timeout");
            break;
        case WIFI_REASON_CONNECTION_FAIL:
            ESP_LOGW(TAG, "Reason: Connection failed");
            break;
        default:
            ESP_LOGW(TAG, "Reason: Unknown (%d)", disconnected->reason);
            break;
    }
    ESP_LOGW(TAG, "============================================");
}

void WiFiStation::log_connection(ip_event_got_ip_t* event) {
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "WiFi CONNECTED!");
    ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
    ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
    ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
    
    if (retry_num > 0) {
        ESP_LOGI(TAG, "Reconnected successfully after %d attempts", retry_num);
    } else {
        ESP_LOGI(TAG, "Connected successfully on first attempt");
    }
    ESP_LOGI(TAG, "============================================");
}

bool WiFiStation::init(bool enable_ap) {
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    
    if (enable_ap) {
        esp_netif_config_t cfg_ap = ESP_NETIF_DEFAULT_WIFI_AP();
        esp_netif_create_wifi(WIFI_IF_AP, cfg_ap.base);
    }
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler_wrapper,
                                                        nullptr,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler_wrapper,
                                                        nullptr,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {};
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.ssid), 
                 EXAMPLE_ESP_WIFI_SSID, sizeof(wifi_config.sta.ssid));
    std::strncpy(reinterpret_cast<char*>(wifi_config.sta.password), 
                 EXAMPLE_ESP_WIFI_PASS, sizeof(wifi_config.sta.password));
    wifi_config.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    
    wifi_mode_t mode = enable_ap ? WIFI_MODE_APSTA : WIFI_MODE_STA;
    ESP_ERROR_CHECK(esp_wifi_set_mode(mode));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    
    if (enable_ap) {
        wifi_config_t ap_config = {};
        std::strncpy(reinterpret_cast<char*>(ap_config.ap.ssid), 
                     AP_SSID, sizeof(ap_config.ap.ssid) - 1);
        std::strncpy(reinterpret_cast<char*>(ap_config.ap.password), 
                     AP_PASSWORD, sizeof(ap_config.ap.password) - 1);
        ap_config.ap.ssid_len = strlen(AP_SSID);
        ap_config.ap.channel = AP_CHANNEL;
        ap_config.ap.max_connection = AP_MAX_CONNECTIONS;
        
        int pwd_len = strlen(AP_PASSWORD);
        if (pwd_len >= 8 && pwd_len < 64) {
            ap_config.ap.authmode = WIFI_AUTH_WPA_WPA2_PSK;
            ap_config.ap.pmf_cfg.required = false;
            ESP_LOGI(TAG, "AP: WPA/WPA2-PSK mode, password length: %d", pwd_len);
        } else {
            ap_config.ap.authmode = WIFI_AUTH_OPEN;
            ESP_LOGW(TAG, "AP: OPEN mode (password invalid length)");
        }
        
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap_config));
        ESP_LOGI(TAG, "AP configured - SSID: %s, Auth: %d", AP_SSID, ap_config.ap.authmode);
    }
    
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "WiFi initialization finished");
    
    if (enable_ap) {
        ESP_LOGI(TAG, "AP mode enabled - returning immediately (station will connect in background)");
        return true;
    }

    ESP_LOGI(TAG, "Waiting for station connection...");
    EventBits_t bits = xEventGroupWaitBits(wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "Connected to AP SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        return true;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
        return false;
    }
    
    ESP_LOGE(TAG, "UNEXPECTED EVENT");
    return false;
}

bool WiFiStation::is_connected() const {
    return connected;
}

WiFiStation* WiFiStation::get_instance() {
    return instance;
}

