#include "app_mqtt.h"
#include "sensor_manager.h"
#include "app_common.h"
#include "config.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <inttypes.h>
#include <string>

#include "esp_log.h"
#include "mqtt_client.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#define BROKER_URI "mqtt://172.20.10.3:1883"
#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define SEND_INTERVAL_MS (60 * 1000)
#define BATCH_SIZE 50

static const char *TAG = "app_mqtt";

static esp_mqtt_client_handle_t mqtt_client = NULL;

extern "C" {

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
    esp_mqtt_client_handle_t client = event->client;

    switch ((esp_mqtt_event_id_t)event_id)
    {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        // Publish online status (Retained)
        esp_mqtt_client_publish(client, MQTT_TOPIC_BASE MQTT_TOPIC_SUFFIX_STATUS, "{\"state\":\"online\"}", 0, 1, 1);
        
        // Subscribe to commands
        esp_mqtt_client_subscribe(client, MQTT_TOPIC_BASE MQTT_TOPIC_SUFFIX_CMD, 1);
        break;

    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;

    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;

    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        // Log basic info
        ESP_LOGI(TAG, "TOPIC=%.*s", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s", event->data_len, event->data);

        // Check if it is a command
        // Note: Simple check, in real app might need exact match logic
        if (strncmp(event->topic, MQTT_TOPIC_BASE MQTT_TOPIC_SUFFIX_CMD, event->topic_len) == 0) {
             ESP_LOGI(TAG, "Received message on CMD topic. Logic deferred.");
             // TODO: Parse JSON and execute command
        }
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

static void mqtt_publishing_task(void *pvParameters)
{
    while (1)
    {
        // Check connection
        EventBits_t bits = xEventGroupWaitBits(
            s_app_event_group,
            WIFI_CONNECTED_BIT | TIME_SYNCED_BIT,
            pdFALSE,
            pdTRUE,
            pdMS_TO_TICKS(1000));

        if ((bits & (WIFI_CONNECTED_BIT | TIME_SYNCED_BIT)) == (WIFI_CONNECTED_BIT | TIME_SYNCED_BIT))
        {
            // Connected, drain the queue
            while (!SensorManager::getInstance().empty()) {
                std::vector<Telemetry> batch = SensorManager::getInstance().popBatch(BATCH_SIZE);
                if (batch.empty()) break;

                // Create JSON array manually
                std::string payload = "[";
                // Reserve memory approx 180 bytes per item
                payload.reserve(batch.size() * 180);
                
                char buf[256];
                for (size_t i = 0; i < batch.size(); ++i) {
                    const auto& item = batch[i];
                    snprintf(buf, sizeof(buf), 
                        "{\"timestamp\":%" PRIi64 ",\"temperature\":%.1f,\"humidity\":%.1f,\"pressure\":%.1f,\"motionDetected\":%s}",
                        item.timestamp, item.temperature, item.humidity, item.pressure, item.motion_detected ? "true" : "false");
                    
                    payload += buf;
                    if (i < batch.size() - 1) {
                        payload += ",";
                    }
                }
                payload += "]";

                ESP_LOGI(TAG, "Publishing batch of %d items, length: %d", (int)batch.size(), (int)payload.length());
                
                int msg_id = esp_mqtt_client_publish(mqtt_client, MQTT_TOPIC_BASE MQTT_TOPIC_SUFFIX_TELEMETRY, payload.c_str(), 0, 1, 0);
                if (msg_id == -1) {
                    ESP_LOGE(TAG, "Failed to publish message, requeueing batch");
                    SensorManager::getInstance().requeueBatch(batch);
                    break;
                }
                
                vTaskDelay(pdMS_TO_TICKS(100));
            }
        }
        
        vTaskDelay(pdMS_TO_TICKS(SEND_INTERVAL_MS));
    }
}

void app_mqtt_init(void)
{
    esp_mqtt_client_config_t mqtt_cfg = {};
    mqtt_cfg.broker.address.uri = BROKER_URI;
    mqtt_cfg.credentials.username = MQTT_USERNAME;
    mqtt_cfg.credentials.authentication.password = MQTT_PASSWORD;
    
    // LWT Setup
    mqtt_cfg.session.last_will.topic = MQTT_TOPIC_BASE MQTT_TOPIC_SUFFIX_STATUS;
    mqtt_cfg.session.last_will.msg = "{\"state\":\"offline\"}";
    mqtt_cfg.session.last_will.qos = 1;
    mqtt_cfg.session.last_will.retain = 1;

    mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(mqtt_client, (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
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
    xTaskCreate(mqtt_publishing_task, "mqtt_pub", 4096, NULL, 5, NULL);
}

} // extern "C"
