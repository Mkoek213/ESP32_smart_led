#include "access_point.h"
#include "config.h"
#include "esp_log.h"
#include "esp_mac.h"

static const char* TAG = "ACCESS_POINT";

AccessPoint* AccessPoint::instance = nullptr;

AccessPoint::AccessPoint() : web_server(nullptr) {
    instance = this;
}

AccessPoint::~AccessPoint() {
    if (web_server) {
        web_server->stop();
        delete web_server;
    }
}

void AccessPoint::event_handler_wrapper(void* arg, esp_event_base_t event_base,
                                       int32_t event_id, void* event_data) {
    if (instance != nullptr) {
        instance->event_handler(event_base, event_id, event_data);
    }
}

void AccessPoint::event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STACONNECTED) {
        wifi_event_ap_staconnected_t* event = 
            static_cast<wifi_event_ap_staconnected_t*>(event_data);
        ESP_LOGI(TAG, "Client connected to AP, MAC: " MACSTR, MAC2STR(event->mac));
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_AP_STADISCONNECTED) {
        wifi_event_ap_stadisconnected_t* event = 
            static_cast<wifi_event_ap_stadisconnected_t*>(event_data);
        ESP_LOGI(TAG, "Client disconnected from AP, MAC: " MACSTR, MAC2STR(event->mac));
    }
}

bool AccessPoint::init() {
    ESP_LOGI(TAG, "============================================");
    ESP_LOGI(TAG, "Initializing Access Point Mode");
    ESP_LOGI(TAG, "============================================");
    
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STACONNECTED,
                                                        &event_handler_wrapper,
                                                        nullptr,
                                                        nullptr));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        WIFI_EVENT_AP_STADISCONNECTED,
                                                        &event_handler_wrapper,
                                                        nullptr,
                                                        nullptr));
    
    ESP_LOGI(TAG, "Access Point configured (in WiFiStation::init)");
    ESP_LOGI(TAG, "   SSID: %s", AP_SSID);
    ESP_LOGI(TAG, "   Password: %s", AP_PASSWORD);
    ESP_LOGI(TAG, "   Channel: %d", AP_CHANNEL);
    ESP_LOGI(TAG, "   Max Connections: %d", AP_MAX_CONNECTIONS);
    ESP_LOGI(TAG, "   IP Address: 192.168.4.1");
    ESP_LOGI(TAG, "============================================");
    
    web_server = new WebServer();
    if (web_server->start(WEB_SERVER_PORT)) {
        ESP_LOGI(TAG, "Web server running at http://192.168.4.1");
    }
    
    return true;
}

