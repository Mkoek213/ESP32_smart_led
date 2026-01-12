#include "http_server.h"
#include "esp_log.h"
#include "esp_http_server.h"
#include "esp_mac.h"
#include "cJSON.h"
#include "ota_update.h"
#include "ble_provisioning.h"
#include <string.h>

static const char *TAG = "http_server";
static httpd_handle_t server = NULL;
static http_server_callbacks_t callbacks = {0};

void http_server_register_callbacks(const http_server_callbacks_t* cbs)
{
    if (cbs) {
        memcpy(&callbacks, cbs, sizeof(http_server_callbacks_t));
        ESP_LOGI(TAG, "Callbacks registered");
    }
}

// CORS headers for web app
static esp_err_t set_cors_headers(httpd_req_t *req)
{
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Methods", "GET, POST, PUT, DELETE, OPTIONS");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Headers", "Content-Type, Authorization");
    return ESP_OK;
}

// GET /api/device/info - Device information
static esp_err_t device_info_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    // Get MAC address
    uint8_t mac[6];
    ble_provisioning_get_mac_address(mac);
    char mac_str[18];
    snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
             mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
    
    // Get PoP
    uint8_t pop[32];
    char pop_hex[65] = {0};
    if (ble_provisioning_get_pop(pop)) {
        for (int i = 0; i < 32; i++) {
            sprintf(&pop_hex[i*2], "%02x", pop[i]);
        }
    }
    
    // Build JSON response
    cJSON *root = cJSON_CreateObject();
    cJSON_AddStringToObject(root, "mac", mac_str);
    cJSON_AddStringToObject(root, "pop", pop_hex);
    cJSON_AddStringToObject(root, "firmware_version", ota_get_current_version());
    cJSON_AddStringToObject(root, "device_type", "smart_led");
    
    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, json_str);
    
    free(json_str);
    return ESP_OK;
}

// GET /api/device/status - Device status (telemetry)
static esp_err_t device_status_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    const char* status_json = "{}";
    if (callbacks.get_status) {
        status_json = callbacks.get_status();
    }
    
    httpd_resp_set_type(req, "application/json");
    httpd_resp_sendstr(req, status_json);
    return ESP_OK;
}

// POST /api/device/led - Control LEDs
static esp_err_t led_control_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    ESP_LOGI(TAG, "LED control request: %s", content);
    
    // Parse JSON
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *r = cJSON_GetObjectItem(root, "red");
    cJSON *g = cJSON_GetObjectItem(root, "green");
    cJSON *b = cJSON_GetObjectItem(root, "blue");
    cJSON *brightness = cJSON_GetObjectItem(root, "brightness");
    
    if (!r || !g || !b) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing RGB values");
        return ESP_FAIL;
    }
    
    uint8_t red = (uint8_t)r->valueint;
    uint8_t green = (uint8_t)g->valueint;
    uint8_t blue = (uint8_t)b->valueint;
    uint8_t bright = brightness ? (uint8_t)brightness->valueint : 255;
    
    cJSON_Delete(root);
    
    // Call callback
    if (callbacks.on_led_control) {
        callbacks.on_led_control(red, green, blue, bright);
    }
    
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// POST /api/device/config - Update device configuration
static esp_err_t config_update_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    char *content = malloc(req->content_len + 1);
    if (!content) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }
    
    int ret = httpd_req_recv(req, content, req->content_len);
    if (ret <= 0) {
        free(content);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    ESP_LOGI(TAG, "Config update request: %s", content);
    
    // Validate JSON
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        free(content);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    cJSON_Delete(root);
    
    // Call callback
    if (callbacks.on_config_update) {
        callbacks.on_config_update(content);
    }
    
    free(content);
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

// POST /api/device/ota - Trigger OTA update
static esp_err_t ota_trigger_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    char content[256];
    int ret = httpd_req_recv(req, content, sizeof(content) - 1);
    if (ret <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request");
        return ESP_FAIL;
    }
    content[ret] = '\0';
    
    // Parse JSON to get URL
    cJSON *root = cJSON_Parse(content);
    if (!root) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON");
        return ESP_FAIL;
    }
    
    cJSON *url_item = cJSON_GetObjectItem(root, "url");
    if (!url_item || !cJSON_IsString(url_item)) {
        cJSON_Delete(root);
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Missing URL");
        return ESP_FAIL;
    }
    
    const char* url = url_item->valuestring;
    ESP_LOGI(TAG, "OTA update triggered from URL: %s", url);
    
    httpd_resp_sendstr(req, "{\"status\":\"updating\"}");
    
    // Start OTA in background (will restart device on success)
    char* url_copy = strdup(url);
    cJSON_Delete(root);
    
    // Create task to perform OTA (so we can respond first)
    xTaskCreate([](void* param) {
        vTaskDelay(pdMS_TO_TICKS(1000)); // Give time for response to be sent
        ota_update_from_url((const char*)param);
        free(param);
        vTaskDelete(NULL);
    }, "ota_trigger", 8192, url_copy, 5, NULL);
    
    return ESP_OK;
}

// POST /api/device/factory-reset - Factory reset device
static esp_err_t factory_reset_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    
    ESP_LOGW(TAG, "Factory reset triggered via HTTP API");
    
    httpd_resp_sendstr(req, "{\"status\":\"resetting\"}");
    
    // Perform factory reset in background
    xTaskCreate([](void* param) {
        vTaskDelay(pdMS_TO_TICKS(1000));
        ble_provisioning_factory_reset();
        vTaskDelete(NULL);
    }, "factory_reset", 2048, NULL, 5, NULL);
    
    return ESP_OK;
}

// OPTIONS handler for CORS preflight
static esp_err_t options_handler(httpd_req_t *req)
{
    set_cors_headers(req);
    httpd_resp_set_status(req, "204 No Content");
    httpd_resp_send(req, NULL, 0);
    return ESP_OK;
}

esp_err_t http_server_start(void)
{
    if (server != NULL) {
        ESP_LOGW(TAG, "HTTP server already running");
        return ESP_OK;
    }
    
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.lru_purge_enable = true;
    config.max_uri_handlers = 16;
    
    ESP_LOGI(TAG, "Starting HTTP server on port %d", config.server_port);
    
    if (httpd_start(&server, &config) != ESP_OK) {
        ESP_LOGE(TAG, "Failed to start HTTP server");
        return ESP_FAIL;
    }
    
    // Register URI handlers
    httpd_uri_t device_info = {
        .uri = "/api/device/info",
        .method = HTTP_GET,
        .handler = device_info_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &device_info);
    
    httpd_uri_t device_status = {
        .uri = "/api/device/status",
        .method = HTTP_GET,
        .handler = device_status_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &device_status);
    
    httpd_uri_t led_control = {
        .uri = "/api/device/led",
        .method = HTTP_POST,
        .handler = led_control_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &led_control);
    
    httpd_uri_t config_update = {
        .uri = "/api/device/config",
        .method = HTTP_POST,
        .handler = config_update_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &config_update);
    
    httpd_uri_t ota_trigger = {
        .uri = "/api/device/ota",
        .method = HTTP_POST,
        .handler = ota_trigger_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &ota_trigger);
    
    httpd_uri_t factory_reset = {
        .uri = "/api/device/factory-reset",
        .method = HTTP_POST,
        .handler = factory_reset_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &factory_reset);
    
    // OPTIONS handler for CORS
    httpd_uri_t options = {
        .uri = "*",
        .method = HTTP_OPTIONS,
        .handler = options_handler,
        .user_ctx = NULL
    };
    httpd_register_uri_handler(server, &options);
    
    ESP_LOGI(TAG, "HTTP server started successfully");
    return ESP_OK;
}

void http_server_stop(void)
{
    if (server) {
        httpd_stop(server);
        server = NULL;
        ESP_LOGI(TAG, "HTTP server stopped");
    }
}
