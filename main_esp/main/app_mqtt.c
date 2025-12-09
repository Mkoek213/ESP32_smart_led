#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <time.h>
#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "app_mqtt.h"
#include "app_common.h"

#define BROKER_URI CONFIG_BROKER_URI
#define MQTT_USERNAME CONFIG_MQTT_USERNAME
#define MQTT_PASSWORD CONFIG_MQTT_PASSWORD
#define SENSOR_DATA_TOPIC CONFIG_SENSOR_TOPIC
#define SEND_INTERVAL_MS (15 * 1000)

static const char *TAG = "app_mqtt";

static esp_mqtt_client_handle_t mqtt_client = NULL;

static void send_sensor_data(void)
{
    if (!mqtt_client)
    {
        ESP_LOGW(TAG, "MQTT client not ready");
        return;
    }

    time_t now;
    struct tm timeinfo;
    time(&now);
    localtime_r(&now, &timeinfo);

    char timestamp[32];
    strftime(timestamp, sizeof(timestamp), "%Y-%m-%dT%H:%M:%S", &timeinfo);

    double humidity = 30.0 + (rand() % 50);   // 30-80%
    bool motion_detected = (rand() % 2) == 1; // Random true/false
    int light_level = 100 + (rand() % 900);   // 100-1000 lux

    char payload[256];
    snprintf(payload, sizeof(payload),
             "[{\"timestamp\":\"%s\",\"humidity\":%.1f,\"motionDetected\":%s,\"lightLevel\":%d}]",
             timestamp, humidity, motion_detected ? "true" : "false", light_level);

    esp_mqtt_client_publish(mqtt_client, SENSOR_DATA_TOPIC, payload, 0, 1, 0);
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    case MQTT_EVENT_ERROR:
        ESP_LOGE(TAG, "MQTT_EVENT_ERROR");

        if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT)
        {
            ESP_LOGE(TAG, "TCP transport error connecting to the MQTT broker");
        }

        break;
    default:
        break;
    }
}

static void sensor_data_task(void *pvParameters)
{
    while (1)
    {
        vTaskDelay(SEND_INTERVAL_MS / portTICK_PERIOD_MS);

        EventBits_t bits = xEventGroupWaitBits(
            s_app_event_group,
            WIFI_CONNECTED_BIT | TIME_SYNCED_BIT,
            pdFALSE,              // don't clear the bits on exit
            pdTRUE,               // wait for all bits to be set
            pdMS_TO_TICKS(5000)); // wait for max 5 seconds

        if ((bits & (WIFI_CONNECTED_BIT | TIME_SYNCED_BIT)) == (WIFI_CONNECTED_BIT | TIME_SYNCED_BIT))
        {
            send_sensor_data();
        }
    }
}

void app_mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = BROKER_URI,
        .credentials = {
            .username = MQTT_USERNAME,
            .authentication = {
                .password = MQTT_PASSWORD,
            },
        },
    };

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
}

void app_mqtt_start(void)
{
    if (mqtt_client)
    {
        esp_mqtt_client_start(mqtt_client);
    }
}

void app_mqtt_stop(void)
{
    if (mqtt_client)
    {
        esp_mqtt_client_stop(mqtt_client);
    }
}

void app_mqtt_start_publishing_task(void)
{
    xTaskCreate(sensor_data_task, "sensor_data_task", 4 * 1024, NULL, 5, NULL);
}
