#ifndef WIFI_STATION_H
#define WIFI_STATION_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "led_controller.h"

/**
 * @brief WiFi Station Class
 * Manages WiFi station connection and reconnection logic
 */
class WiFiStation {
private:
    static const int WIFI_CONNECTED_BIT = BIT0;
    static const int WIFI_FAIL_BIT = BIT1;
    
    EventGroupHandle_t wifi_event_group;
    int retry_num;
    bool connected;
    LEDController* led;
    
    static WiFiStation* instance;
    
    static void event_handler_wrapper(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data);
    void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data);
    
    void log_connection(ip_event_got_ip_t* event);
    void log_disconnection(wifi_event_sta_disconnected_t* disconnected);
    
public:
    WiFiStation(LEDController* led_controller);
    ~WiFiStation();
    
    bool init(bool enable_ap = false);
    bool is_connected() const;
    
    static WiFiStation* get_instance();
};

#endif // WIFI_STATION_H

