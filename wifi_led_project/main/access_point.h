#ifndef ACCESS_POINT_H
#define ACCESS_POINT_H

#include "esp_event.h"
#include "esp_wifi.h"
#include "web_server.h"

/**
 * @brief Access Point Class
 * Manages ESP32 as WiFi Access Point with web server
 */
class AccessPoint {
private:
    WebServer* web_server;
    
    static AccessPoint* instance;
    
    static void event_handler_wrapper(void* arg, esp_event_base_t event_base,
                                     int32_t event_id, void* event_data);
    void event_handler(esp_event_base_t event_base, int32_t event_id, void* event_data);
    
public:
    AccessPoint();
    ~AccessPoint();
    
    bool init();
};

#endif // ACCESS_POINT_H

