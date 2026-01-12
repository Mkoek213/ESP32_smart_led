#ifndef HTTP_SERVER_H
#define HTTP_SERVER_H

#include "esp_err.h"
#include "esp_http_server.h"

#ifdef __cplusplus
extern "C" {
#endif

// Start HTTP REST API server on port 80
esp_err_t http_server_start(void);

// Stop HTTP server
void http_server_stop(void);

// Register device control callbacks
typedef struct {
    void (*on_led_control)(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness);
    void (*on_config_update)(const char* json_config);
    const char* (*get_status)(void);  // Returns JSON string with device status
} http_server_callbacks_t;

void http_server_register_callbacks(const http_server_callbacks_t* callbacks);

#ifdef __cplusplus
}
#endif

#endif // HTTP_SERVER_H
