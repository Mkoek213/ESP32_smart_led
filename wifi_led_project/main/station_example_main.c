/* WiFi station Example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"
#include "esp_system.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "driver/gpio.h"

#include "lwip/err.h"
#include "lwip/sys.h"

/* The examples use WiFi configuration that you can set via project configuration menu

   If you'd rather not, just change the below entries to strings with
   the config you want - ie #define EXAMPLE_WIFI_SSID "mywifissid"
*/
#define EXAMPLE_ESP_WIFI_SSID      "iPhone"
#define EXAMPLE_ESP_WIFI_PASS      "..."

/* LED Configuration */
#define LED_GPIO                   GPIO_NUM_2  // Built-in LED on most ESP32 dev boards
#define LED_BLINK_PERIOD_MS        500         // LED blink period in milliseconds

#if CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HUNT_AND_PECK
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HUNT_AND_PECK
#define EXAMPLE_H2E_IDENTIFIER ""
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_HASH_TO_ELEMENT
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_HASH_TO_ELEMENT
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#elif CONFIG_ESP_STATION_EXAMPLE_WPA3_SAE_PWE_BOTH
#define ESP_WIFI_SAE_MODE WPA3_SAE_PWE_BOTH
#define EXAMPLE_H2E_IDENTIFIER CONFIG_ESP_WIFI_PW_ID
#endif
#if CONFIG_ESP_WIFI_AUTH_OPEN
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_OPEN
#elif CONFIG_ESP_WIFI_AUTH_WEP
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WEP
#elif CONFIG_ESP_WIFI_AUTH_WPA_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA_WPA2_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA_WPA2_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WPA2_WPA3_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WPA2_WPA3_PSK
#elif CONFIG_ESP_WIFI_AUTH_WAPI_PSK
#define ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD WIFI_AUTH_WAPI_PSK
#endif

/* FreeRTOS event group to signal when we are connected*/
static EventGroupHandle_t s_wifi_event_group;

/* The event group allows multiple bits for each event, but we only care about two events:
 * - we are connected to the AP with an IP
 * - we failed to connect after the maximum amount of retries */
#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1

static const char *TAG = "wifi station";

static int s_retry_num = 0;

/* Global variable to track WiFi connection status */
static volatile bool wifi_connected = false;

/* Task handle for LED blinking task */
static TaskHandle_t led_blink_task_handle = NULL;

/**
 * @brief LED blinking task - blinks LED when WiFi is disconnected
 */
static void led_blink_task(void* arg)
{
    ESP_LOGI(TAG, "LED blink task started - WiFi disconnected");
    
    while(1) {
        gpio_set_level(LED_GPIO, 1);  // LED ON
        vTaskDelay(LED_BLINK_PERIOD_MS / portTICK_PERIOD_MS);
        gpio_set_level(LED_GPIO, 0);  // LED OFF
        vTaskDelay(LED_BLINK_PERIOD_MS / portTICK_PERIOD_MS);
    }
}

/**
 * @brief Initialize LED GPIO
 */
static void led_init(void)
{
    gpio_reset_pin(LED_GPIO);
    gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
    gpio_set_level(LED_GPIO, 0);  // LED OFF initially
    ESP_LOGI(TAG, "LED initialized on GPIO %d", LED_GPIO);
}

/**
 * @brief Start LED blinking task
 */
static void start_led_blink(void)
{
    if (led_blink_task_handle == NULL) {
        xTaskCreate(led_blink_task, "led_blink_task", 2048, NULL, 5, &led_blink_task_handle);
    }
}

/**
 * @brief Stop LED blinking task and turn off LED
 */
static void stop_led_blink(void)
{
    if (led_blink_task_handle != NULL) {
        vTaskDelete(led_blink_task_handle);
        led_blink_task_handle = NULL;
        gpio_set_level(LED_GPIO, 0);  // Turn off LED
        ESP_LOGI(TAG, "LED blink task stopped - WiFi connected");
    }
}

static void event_handler(void* arg, esp_event_base_t event_base,
                                int32_t event_id, void* event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        ESP_LOGI(TAG, "WiFi station started, attempting initial connection...");
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* disconnected = (wifi_event_sta_disconnected_t*) event_data;
        
        /* Log detailed disconnection information */
        ESP_LOGW(TAG, "============================================");
        ESP_LOGW(TAG, "WiFi DISCONNECTED!");
        ESP_LOGW(TAG, "SSID: %s", disconnected->ssid);
        ESP_LOGW(TAG, "Reason code: %d", disconnected->reason);
        
        /* Provide human-readable reason */
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
        
        /* Set connection status to false */
        wifi_connected = false;
        
        /* Start LED blinking to indicate disconnection */
        start_led_blink();
        
        /* Keep trying to reconnect indefinitely (removed retry limit) */
        s_retry_num++;
        ESP_LOGI(TAG, "🔄 Reconnection attempt #%d - Trying to reconnect to AP '%s'...", 
                 s_retry_num, EXAMPLE_ESP_WIFI_SSID);
        esp_wifi_connect();
        
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        
        ESP_LOGI(TAG, "============================================");
        ESP_LOGI(TAG, "✅ WiFi CONNECTED!");
        ESP_LOGI(TAG, "IP Address: " IPSTR, IP2STR(&event->ip_info.ip));
        ESP_LOGI(TAG, "Netmask: " IPSTR, IP2STR(&event->ip_info.netmask));
        ESP_LOGI(TAG, "Gateway: " IPSTR, IP2STR(&event->ip_info.gw));
        
        if (s_retry_num > 0) {
            ESP_LOGI(TAG, "Reconnected successfully after %d attempts", s_retry_num);
        } else {
            ESP_LOGI(TAG, "Connected successfully on first attempt");
        }
        ESP_LOGI(TAG, "============================================");
        
        /* Set connection status to true */
        wifi_connected = true;
        
        /* Stop LED blinking */
        stop_led_blink();
        
        s_retry_num = 0;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

void wifi_init_sta(void)
{
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init());

    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    wifi_config_t wifi_config = {
        .sta = {
            .ssid = EXAMPLE_ESP_WIFI_SSID,
            .password = EXAMPLE_ESP_WIFI_PASS,
            /* Authmode threshold resets to WPA2 as default if password matches WPA2 standards (password len => 8).
             * If you want to connect the device to deprecated WEP/WPA networks, Please set the threshold value
             * to WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK and set the password with length and format matching to
             * WIFI_AUTH_WEP/WIFI_AUTH_WPA_PSK standards.
             */
            .threshold.authmode = ESP_WIFI_SCAN_AUTH_MODE_THRESHOLD,
            .sae_pwe_h2e = ESP_WIFI_SAE_MODE,
            .sae_h2e_identifier = EXAMPLE_H2E_IDENTIFIER,
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA) );
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config) );
    ESP_ERROR_CHECK(esp_wifi_start() );

    ESP_LOGI(TAG, "wifi_init_sta finished.");

    /* Waiting until either the connection is established (WIFI_CONNECTED_BIT) or connection failed for the maximum
     * number of re-tries (WIFI_FAIL_BIT). The bits are set by event_handler() (see above) */
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    /* xEventGroupWaitBits() returns the bits before the call returned, hence we can test which event actually
     * happened. */
    if (bits & WIFI_CONNECTED_BIT) {
        ESP_LOGI(TAG, "connected to ap SSID:%s password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGI(TAG, "Failed to connect to SSID:%s, password:%s",
                 EXAMPLE_ESP_WIFI_SSID, EXAMPLE_ESP_WIFI_PASS);
    } else {
        ESP_LOGE(TAG, "UNEXPECTED EVENT");
    }
}

void app_main(void)
{
    //Initialize NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
      ESP_ERROR_CHECK(nvs_flash_erase());
      ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    /* Initialize LED GPIO */
    led_init();

    if (CONFIG_LOG_MAXIMUM_LEVEL > CONFIG_LOG_DEFAULT_LEVEL) {
        /* If you only want to open more logs in the wifi module, you need to make the max level greater than the default level,
         * and call esp_log_level_set() before esp_wifi_init() to improve the log level of the wifi module. */
        esp_log_level_set("wifi", CONFIG_LOG_MAXIMUM_LEVEL);
    }

    ESP_LOGI(TAG, "ESP_WIFI_MODE_STA");
    wifi_init_sta();
}
