#include "app_mqtt.h"
#include "app_common.h"
#include "config.h"
#include "wifi_config.h"
#include "esp_mac.h"
#include "led_config.h"
#include "sensor_manager.h"

#include <inttypes.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <string>

#include "cJSON.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "mqtt_client.h"

#define MQTT_USERNAME ""
#define MQTT_PASSWORD ""

#define SEND_INTERVAL_MS                                                       \
  (30 * 1000) // Check and send every 30 seconds (matches telemetry generation)
#define BATCH_SIZE 50

static const char *TAG = "app_mqtt";

static esp_mqtt_client_handle_t mqtt_client = NULL;

// Static variables to hold the dynamic topic strings
static std::string topic_telemetry;
static std::string topic_status;
static std::string topic_cmd;
static std::string topic_config;

// Embed certificates
extern const uint8_t root_ca_pem_start[] asm("_binary_AmazonRootCA1_pem_start");
extern const uint8_t root_ca_pem_end[] asm("_binary_AmazonRootCA1_pem_end");
extern const uint8_t client_cert_pem_start[] asm("_binary_certificate_pem_crt_start");
extern const uint8_t client_cert_pem_end[] asm("_binary_certificate_pem_crt_end");
extern const uint8_t private_key_pem_start[] asm("_binary_private_pem_key_start");
extern const uint8_t private_key_pem_end[] asm("_binary_private_pem_key_end");

extern "C" {

static void mqtt_event_handler(void *handler_args, esp_event_base_t base,
                               int32_t event_id, void *event_data) {
  ESP_LOGD(TAG,
           "Event dispatched from event loop base=%s, event_id=%" PRIi32 "",
           base, event_id);
  esp_mqtt_event_handle_t event = (esp_mqtt_event_handle_t)event_data;
  esp_mqtt_client_handle_t client = event->client;

  switch ((esp_mqtt_event_id_t)event_id) {
  case MQTT_EVENT_CONNECTED:
    ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
    // Publish online status (Retained)
    esp_mqtt_client_publish(client, topic_status.c_str(),
                            "{\"state\":\"online\"}", 0, 1, 1);

    // Subscribe to commands
    esp_mqtt_client_subscribe(client, topic_cmd.c_str(), 1);
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
    if (event->topic_len > 0 &&
        strncmp(event->topic, topic_cmd.c_str(), event->topic_len) == 0) {
      ESP_LOGI(TAG, "Received command on CMD topic");

      // Parse JSON command
      char *json_str = (char *)malloc(event->data_len + 1);
      if (json_str) {
        memcpy(json_str, event->data, event->data_len);
        json_str[event->data_len] = '\0';

        cJSON *root = cJSON_Parse(json_str);
        if (root) {
          cJSON *type = cJSON_GetObjectItem(root, "type");

          if (type && cJSON_IsString(type)) {
            if (strcmp(type->valuestring, "SET_CONFIG") == 0) {
              // Handle SET_CONFIG command
              cJSON *payload = cJSON_GetObjectItem(root, "payload");
              if (payload) {
                LEDConfig new_config =
                    LEDConfigManager::getInstance().getConfig();

                // Parse humidity thresholds
                cJSON *thresholds =
                    cJSON_GetObjectItem(payload, "humidityThresholds");
                if (thresholds && cJSON_IsArray(thresholds) &&
                    cJSON_GetArraySize(thresholds) == 4) {
                  for (int i = 0; i < 4; i++) {
                    cJSON *item = cJSON_GetArrayItem(thresholds, i);
                    if (cJSON_IsNumber(item)) {
                      new_config.humidity_thresholds[i] =
                          (float)item->valuedouble;
                    }
                  }
                }

                // Parse colors (hex strings like "FF0000")
                cJSON *colors = cJSON_GetObjectItem(payload, "colors");
                if (colors && cJSON_IsArray(colors) &&
                    cJSON_GetArraySize(colors) == 3) {
                  for (int i = 0; i < 3; i++) {
                    cJSON *color_str = cJSON_GetArrayItem(colors, i);
                    if (cJSON_IsString(color_str)) {
                      unsigned int r, g, b;
                      if (sscanf(color_str->valuestring, "%02X%02X%02X", &r, &g,
                                 &b) == 3) {
                        new_config.colors[i].r = (uint8_t)r;
                        new_config.colors[i].g = (uint8_t)g;
                        new_config.colors[i].b = (uint8_t)b;
                      }
                    }
                  }
                }

                // Parse brightness settings
                cJSON *brightness_pct =
                    cJSON_GetObjectItem(payload, "brightnessPct");
                if (brightness_pct && cJSON_IsNumber(brightness_pct)) {
                  new_config.manual_brightness_pct =
                      (uint8_t)brightness_pct->valueint;
                }

                cJSON *auto_brightness =
                    cJSON_GetObjectItem(payload, "autobrightness");
                if (auto_brightness && cJSON_IsBool(auto_brightness)) {
                  new_config.auto_brightness = cJSON_IsTrue(auto_brightness);
                }

                // Parse number of LEDs to activate
                cJSON *num_leds = cJSON_GetObjectItem(payload, "numLeds");
                if (num_leds && cJSON_IsNumber(num_leds)) {
                  uint8_t leds = (uint8_t)num_leds->valueint;
                  // Clamp to valid range 1-WS2812B_NUM_LEDS
                  if (leds >= 1 && leds <= WS2812B_NUM_LEDS) {
                    new_config.num_leds_active = leds;
                  }
                }

                // Parse LED timeout settings (in seconds, convert to ms)
                cJSON *no_motion_timeout =
                    cJSON_GetObjectItem(payload, "noMotionTimeoutSec");
                if (no_motion_timeout && cJSON_IsNumber(no_motion_timeout)) {
                  uint32_t timeout_sec = (uint32_t)no_motion_timeout->valueint;
                  if (timeout_sec > 0 && timeout_sec <= 300) { // Max 5 minutes
                    new_config.no_motion_timeout_ms = timeout_sec * 1000;
                  }
                }

                cJSON *max_on_duration =
                    cJSON_GetObjectItem(payload, "maxOnDurationSec");
                if (max_on_duration && cJSON_IsNumber(max_on_duration)) {
                  uint32_t duration_sec = (uint32_t)max_on_duration->valueint;
                  if (duration_sec > 0 &&
                      duration_sec <= 600) { // Max 10 minutes
                    new_config.max_on_duration_ms = duration_sec * 1000;
                  }
                }

                // Parse distance threshold (in cm)
                cJSON *distance_threshold =
                    cJSON_GetObjectItem(payload, "distanceThresholdCm");
                if (distance_threshold && cJSON_IsNumber(distance_threshold)) {
                  float threshold = (float)distance_threshold->valuedouble;
                  if (threshold > 0.0f && threshold <= 400.0f) { // Max 4 meters
                    new_config.distance_threshold_cm = threshold;
                  }
                }

                // Apply configuration
                LEDConfigManager::getInstance().setConfig(new_config);
                ESP_LOGI(TAG, "LED configuration updated via MQTT");
              }
            } else if (strcmp(type->valuestring, "GET_CONFIG") == 0) {
              // Handle GET_CONFIG command - publish current config
              const LEDConfig &cfg =
                  LEDConfigManager::getInstance().getConfig();

              char response[512];
              snprintf(
                  response, sizeof(response),
                  "{\"humidityThresholds\":[%.1f,%.1f,%.1f,%.1f],"
                  "\"colors\":[\"%02X%02X%02X\",\"%02X%02X%02X\",\"%02X%02X%"
                  "02X\"],"
                  "\"brightnessPct\":%d,\"autobrightness\":%s,\"numLeds\":%d,"
                  "\"noMotionTimeoutSec\":%lu,\"maxOnDurationSec\":%lu,"
                  "\"distanceThresholdCm\":%.1f}",
                  cfg.humidity_thresholds[0], cfg.humidity_thresholds[1],
                  cfg.humidity_thresholds[2], cfg.humidity_thresholds[3],
                  cfg.colors[0].r, cfg.colors[0].g, cfg.colors[0].b,
                  cfg.colors[1].r, cfg.colors[1].g, cfg.colors[1].b,
                  cfg.colors[2].r, cfg.colors[2].g, cfg.colors[2].b,
                  cfg.manual_brightness_pct,
                  cfg.auto_brightness ? "true" : "false", cfg.num_leds_active,
                  (unsigned long)(cfg.no_motion_timeout_ms / 1000),
                  (unsigned long)(cfg.max_on_duration_ms / 1000),
                  cfg.distance_threshold_cm);

              esp_mqtt_client_publish(client, topic_config.c_str(), response, 0,
                                      1, 0);
              ESP_LOGI(TAG, "Published current configuration");
            }
          }

          cJSON_Delete(root);
        }
        free(json_str);
      }
    }
    break;

  case MQTT_EVENT_ERROR:
    ESP_LOGE(TAG, "MQTT_EVENT_ERROR");
    if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
      ESP_LOGE(TAG, "TCP transport error connecting to the MQTT broker");
    }
    break;
  default:
    break;
  }
}

static void mqtt_publishing_task(void *pvParameters) {
  while (1) {
    // Check connection (Time sync is optional)
    EventBits_t bits = xEventGroupWaitBits(
        s_app_event_group, WIFI_CONNECTED_BIT, pdFALSE,
        pdTRUE, pdMS_TO_TICKS(1000));

    if (bits & WIFI_CONNECTED_BIT) {
      // Connected, drain the queue
      while (!SensorManager::getInstance().empty()) {
        std::vector<Telemetry> batch =
            SensorManager::getInstance().popBatch(BATCH_SIZE);
        if (batch.empty())
          break;

        // Create JSON array manually
        std::string payload = "[";
        // Reserve memory approx 180 bytes per item
        payload.reserve(batch.size() * 180);

        char buf[256];
        for (size_t i = 0; i < batch.size(); ++i) {
          const auto &item = batch[i];
          snprintf(buf, sizeof(buf),
                   "{\"timestamp\":%" PRIi64
                   ",\"temperature\":%.1f,\"humidity\":%.1f,\"pressure\":%.1f,"
                   "\"personCount\":%d}",
                   item.timestamp, item.temperature, item.humidity,
                   item.pressure, item.person_count);

          payload += buf;
          if (i < batch.size() - 1) {
            payload += ",";
          }
        }
        payload += "]";

        ESP_LOGI(TAG, "Publishing batch of %d items, length: %d",
                 (int)batch.size(), (int)payload.length());

        int msg_id = esp_mqtt_client_publish(
            mqtt_client, topic_telemetry.c_str(), payload.c_str(), 0, 1, 0);
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

void app_mqtt_init(void) {
  ESP_LOGI(TAG, "Initialising MQTT...");

  // Get device MAC address for topic construction
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  static char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  ESP_LOGI(TAG, "Device MAC: %s", mac_str);

  // Construct topics: smart-led/device/{MAC}/{suffix}
  std::string base_topic = std::string(MQTT_TOPIC_BASE) + std::string(mac_str);

  topic_telemetry = base_topic + MQTT_TOPIC_SUFFIX_TELEMETRY;
  topic_status = base_topic + MQTT_TOPIC_SUFFIX_STATUS;
  topic_cmd = base_topic + MQTT_TOPIC_SUFFIX_CMD;
  topic_config = base_topic + "/config";

  ESP_LOGI(TAG, "Telemetry Topic: %s", topic_telemetry.c_str());
  ESP_LOGI(TAG, "Status Topic: %s", topic_status.c_str());
  ESP_LOGI(TAG, "Command Topic: %s", topic_cmd.c_str());
  ESP_LOGI(TAG, "Config Topic: %s", topic_config.c_str());

  esp_mqtt_client_config_t mqtt_cfg = {};

  // Load configuration from NVS
  WifiConfig wifi_cfg;
  // FORCE HARDCODED because NVS might have old URL
  // if (wifi_config_load(wifi_cfg) && strlen(wifi_cfg.brokerUrl) > 0) {
  //     ESP_LOGI(TAG, "Using configured MQTT Broker: %s", wifi_cfg.brokerUrl);
  //     mqtt_cfg.broker.address.uri = wifi_cfg.brokerUrl;
  // } else {
      ESP_LOGW(TAG, "Using hardcoded AWS Broker: %s", MQTT_BROKER_URL);
      mqtt_cfg.broker.address.uri = MQTT_BROKER_URL;
  // }
  
  // AWS IoT Core Authentication (Certificate-based)
  mqtt_cfg.broker.verification.certificate = (const char *)root_ca_pem_start;
  mqtt_cfg.credentials.authentication.certificate = (const char *)client_cert_pem_start;
  mqtt_cfg.credentials.authentication.key = (const char *)private_key_pem_start;

  // LWT Setup
  mqtt_cfg.session.last_will.topic = topic_status.c_str();
  mqtt_cfg.session.last_will.msg = "{\"state\":\"offline\"}";
  mqtt_cfg.session.last_will.qos = 1;
  // Fix: Set Client ID (Static string)
  mqtt_cfg.credentials.client_id = mac_str;

  // Debug: Validate Certs
  ESP_LOGI(TAG, "Root CA len: %d", strlen((const char *)root_ca_pem_start));
  ESP_LOGI(TAG, "Client Cert len: %d", strlen((const char *)client_cert_pem_start));
  ESP_LOGI(TAG, "Private Key len: %d", strlen((const char *)private_key_pem_start));

  mqtt_client = esp_mqtt_client_init(&mqtt_cfg);
  esp_mqtt_client_register_event(mqtt_client,
                                 (esp_mqtt_event_id_t)ESP_EVENT_ANY_ID,
                                 mqtt_event_handler, NULL);
}

void app_mqtt_start(void) {
  if (mqtt_client) {
    // Wait for Time Sync before connecting (crucial for AWS TLS)
    ESP_LOGI(TAG, "Waiting for Time Sync...");
    xEventGroupWaitBits(s_app_event_group, TIME_SYNCED_BIT, pdFALSE, pdTRUE, pdMS_TO_TICKS(20000));
    ESP_LOGI(TAG, "Time Synced (or timeout), starting MQTT...");

    esp_mqtt_client_start(mqtt_client);
  }
}

void app_mqtt_stop(void) {
  if (mqtt_client) {
    esp_mqtt_client_stop(mqtt_client);
  }
}

void app_mqtt_start_publishing_task(void) {
  xTaskCreate(mqtt_publishing_task, "mqtt_pub", 4096, NULL, 5, NULL);
}

} // extern "C"
