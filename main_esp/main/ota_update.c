#include "ota_update.h"
#include "cJSON.h"
#include "esp_http_client.h"
#include "esp_https_ota.h"
#include "esp_log.h"
#include "esp_mac.h"
#include "esp_ota_ops.h"
#include "esp_system.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>

static const char *TAG = "ota_update";

// Server certificate for HTTPS OTA (if needed)
// extern const char server_cert_pem_start[] asm("_binary_ca_cert_pem_start");
// extern const char server_cert_pem_end[]   asm("_binary_ca_cert_pem_end");

static char s_backend_url[256] = {0};

esp_err_t ota_update_init(void) {
  ESP_LOGI(TAG, "OTA Update System initialized");
  ESP_LOGI(TAG, "Current firmware version: %s", FIRMWARE_VERSION);

  // Get running partition info
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;

  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      ESP_LOGW(TAG, "OTA image is in pending verification state");
      ESP_LOGI(TAG,
               "First boot after OTA update - diagnostics can be run here");
    }
  }

  ESP_LOGI(TAG, "Running partition: %s (type %d, subtype %d, address 0x%08x)",
           running->label, running->type, running->subtype, running->address);

  return ESP_OK;
}

const char *ota_get_current_version(void) { return FIRMWARE_VERSION; }

void ota_mark_boot_valid(void) {
  const esp_partition_t *running = esp_ota_get_running_partition();
  esp_ota_img_states_t ota_state;

  if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
    if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
      ESP_LOGI(TAG, "Marking current boot as valid");
      esp_ota_mark_app_valid_cancel_rollback();
    }
  }
}

typedef struct {
  char *buffer;
  int buffer_len;
  int data_len;
} ota_response_data_t;

static esp_err_t http_client_event_handler(esp_http_client_event_t *evt) {
  switch (evt->event_id) {
  case HTTP_EVENT_ERROR:
    ESP_LOGD(TAG, "HTTP_EVENT_ERROR");
    break;
  case HTTP_EVENT_ON_CONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_CONNECTED");
    break;
  case HTTP_EVENT_HEADER_SENT:
    ESP_LOGD(TAG, "HTTP_EVENT_HEADER_SENT");
    break;
  case HTTP_EVENT_ON_HEADER:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_HEADER, key=%s, value=%s", evt->header_key,
             evt->header_value);
    break;
  case HTTP_EVENT_ON_DATA:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_DATA, len=%d", evt->data_len);
    if (evt->user_data) {
      ota_response_data_t *resp = (ota_response_data_t *)evt->user_data;
      int copy_len = evt->data_len;
      if (resp->data_len + copy_len < resp->buffer_len) {
        memcpy(resp->buffer + resp->data_len, evt->data, copy_len);
        resp->data_len += copy_len;
        resp->buffer[resp->data_len] = '\0'; // Keep null-terminated
      }
    }
    break;
  case HTTP_EVENT_ON_FINISH:
    ESP_LOGD(TAG, "HTTP_EVENT_ON_FINISH");
    break;
  case HTTP_EVENT_DISCONNECTED:
    ESP_LOGD(TAG, "HTTP_EVENT_DISCONNECTED");
    break;
  default:
    break;
  }
  return ESP_OK;
}

esp_err_t ota_update_from_url(const char *url) {
  ESP_LOGI(TAG, "Starting OTA update from URL: %s", url);

  esp_http_client_config_t config = {
      .url = url,
      .timeout_ms = OTA_RECV_TIMEOUT_MS,
      .keep_alive_enable = true,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (client == NULL) {
    ESP_LOGE(TAG, "Failed to initialise HTTP connection");
    return ESP_FAIL;
  }

  esp_err_t err = esp_http_client_open(client, 0);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open HTTP connection: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return err;
  }

  esp_http_client_fetch_headers(client);

  int content_length = esp_http_client_get_content_length(client);
  ESP_LOGI(TAG, "OTA Content Length: %d", content_length);

  const esp_partition_t *update_partition =
      esp_ota_get_next_update_partition(NULL);
  if (update_partition == NULL) {
    ESP_LOGE(TAG, "Passive OTA partition not found");
    esp_http_client_cleanup(client);
    return ESP_FAIL;
  }
  ESP_LOGI(TAG, "Writing to partition: %s at offset 0x%x",
           update_partition->label, (unsigned int)update_partition->address);

  esp_ota_handle_t update_handle = 0;
  err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    return err;
  }

  char *upgrade_data_buf = (char *)malloc(1024);
  if (!upgrade_data_buf) {
    ESP_LOGE(TAG, "Failed to allocate buffer");
    esp_ota_end(update_handle);
    esp_http_client_cleanup(client);
    return ESP_ERR_NO_MEM;
  }

  int binary_file_len = 0;
  while (1) {
    int data_read = esp_http_client_read(client, upgrade_data_buf, 1024);
    if (data_read < 0) {
      ESP_LOGE(TAG, "Error: SSL data read error");
      break;
    } else if (data_read == 0) {
      // Connection closed
      break;
    }

    err = esp_ota_write(update_handle, upgrade_data_buf, data_read);
    if (err != ESP_OK) {
      ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
      break;
    }
    binary_file_len += data_read;
    ESP_LOGD(TAG, "Written image length %d", binary_file_len);
  }

  free(upgrade_data_buf);
  esp_http_client_cleanup(client);

  if (err != ESP_OK) {
    esp_ota_end(update_handle);
    return err;
  }

  err = esp_ota_end(update_handle);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_end failed (%s)", esp_err_to_name(err));
    return err;
  }

  err = esp_ota_set_boot_partition(update_partition);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)",
             esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "OTA update successful! Restarting...");
  vTaskDelay(pdMS_TO_TICKS(1000));
  esp_restart();
  return ESP_OK;
}

static bool is_version_newer(const char *current, const char *target) {
  if (current == NULL || target == NULL)
    return false;

  int v1_major = 0, v1_minor = 0, v1_patch = 0;
  int v2_major = 0, v2_minor = 0, v2_patch = 0;

  // Parse current version (supports X, X.Y, X.Y.Z)
  sscanf(current, "%d.%d.%d", &v1_major, &v1_minor, &v1_patch);

  // Parse target version
  sscanf(target, "%d.%d.%d", &v2_major, &v2_minor, &v2_patch);

  ESP_LOGD(TAG, "Comparing versions: Current(%d.%d.%d) vs Target(%d.%d.%d)",
           v1_major, v1_minor, v1_patch, v2_major, v2_minor, v2_patch);

  if (v2_major > v1_major)
    return true;
  if (v2_major < v1_major)
    return false;

  if (v2_minor > v1_minor)
    return true;
  if (v2_minor < v1_minor)
    return false;

  if (v2_patch > v1_patch)
    return true;

  return false;
}

bool ota_check_and_update(const char *backend_url, const char *device_mac) {
  ESP_LOGI(TAG, "Checking for firmware updates...");

  // Build check URL: GET
  // /api/firmware/check?mac=AA:BB:CC:DD:EE:FF&version=1.0.0
  char check_url[512];
  snprintf(check_url, sizeof(check_url),
           "%s/api/firmware/check?mac=%s&version=%s", backend_url, device_mac,
           FIRMWARE_VERSION);

  ESP_LOGI(TAG, "Check URL: %s", check_url);

  // Allocate buffer for response - needs to be done BEFORE request
  int buffer_size = 1024;
  char *response_buffer = malloc(buffer_size);
  if (!response_buffer) {
    ESP_LOGE(TAG, "Failed to allocate response buffer");
    return false;
  }

  // Initialize response data structure
  ota_response_data_t response_data = {
      .buffer = response_buffer, .buffer_len = buffer_size, .data_len = 0};

  // HTTP client config for checking
  esp_http_client_config_t config = {
      .url = check_url,
      .event_handler = http_client_event_handler,
      .user_data = &response_data, // Pass our buffer structure
      .timeout_ms = 10000,
  };

  esp_http_client_handle_t client = esp_http_client_init(&config);
  if (!client) {
    ESP_LOGE(TAG, "Failed to initialize HTTP client");
    free(response_buffer); // Clean up if init fails
    return false;
  }

  esp_err_t err = esp_http_client_perform(client);

  if (err != ESP_OK) {
    ESP_LOGE(TAG, "HTTP GET request failed: %s", esp_err_to_name(err));
    esp_http_client_cleanup(client);
    free(response_buffer);
    return false;
  }

  int status_code = esp_http_client_get_status_code(client);
  int content_length = esp_http_client_get_content_length(client); // Can be -1

  ESP_LOGI(TAG, "HTTP Status: %d, Content-Length: %d, Read Bytes: %d",
           status_code, content_length, response_data.data_len);

  if (status_code == 204) {
    // No update available
    ESP_LOGI(TAG, "Firmware is up to date");
    esp_http_client_cleanup(client);
    free(response_buffer);
    return false;
  }

  if (status_code != 200) {
    ESP_LOGW(TAG, "Unexpected status code: %d", status_code);
    esp_http_client_cleanup(client);
    free(response_buffer);
    return false;
  }

  // Cleanup client
  esp_http_client_cleanup(client);

  ESP_LOGI(TAG, "Response: %s", response_buffer);

  // Parse JSON response
  cJSON *root = cJSON_Parse(response_buffer);
  free(response_buffer);

  if (!root) {
    ESP_LOGE(TAG, "Failed to parse JSON response");
    return false;
  }

  cJSON *version_item = cJSON_GetObjectItem(root, "version");
  cJSON *url_item = cJSON_GetObjectItem(root, "url");

  if (!version_item || !url_item || !cJSON_IsString(version_item) ||
      !cJSON_IsString(url_item)) {
    ESP_LOGE(TAG, "Invalid JSON format");
    cJSON_Delete(root);
    return false;
  }

  const char *new_version = version_item->valuestring;
  const char *download_url = url_item->valuestring;

  ESP_LOGI(TAG, "New firmware available!");
  ESP_LOGI(TAG, "  Current version: %s", FIRMWARE_VERSION);
  ESP_LOGI(TAG, "  New version: %s", new_version);
  ESP_LOGI(TAG, "  New version: %s", new_version);
  ESP_LOGI(TAG, "  Download URL: %s", download_url);

  // Check if the new version is actually newer
  if (!is_version_newer(FIRMWARE_VERSION, new_version)) {
    ESP_LOGW(TAG, "Ignoring version %s (Current: %s). Update skipped.",
             new_version, FIRMWARE_VERSION);
    cJSON_Delete(root);
    return false;
  }

  // Perform OTA update
  esp_err_t ret = ota_update_from_url(download_url);

  cJSON_Delete(root);

  return (ret == ESP_OK);
}

static void ota_update_task(void *pvParameters) {
  const char *backend_url = (const char *)pvParameters;

  // Get device MAC address for identification
  uint8_t mac[6];
  esp_read_mac(mac, ESP_MAC_WIFI_STA);
  char mac_str[18];
  snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X", mac[0],
           mac[1], mac[2], mac[3], mac[4], mac[5]);

  ESP_LOGI(TAG, "OTA update task started (check interval: %d ms)",
           OTA_CHECK_INTERVAL_MS);

  // Initial delay to let system stabilize
  vTaskDelay(pdMS_TO_TICKS(5000)); // Wait 5 seconds after boot

  while (1) {
    ESP_LOGI(TAG, "Performing periodic firmware update check...");

    bool updated = ota_check_and_update(backend_url, mac_str);

    if (updated) {
      // Device will restart after successful update
      ESP_LOGI(TAG, "Update successful, device restarting...");
      vTaskDelay(pdMS_TO_TICKS(2000));
    }

    // Wait for next check interval
    vTaskDelay(pdMS_TO_TICKS(OTA_CHECK_INTERVAL_MS));
  }
}

void ota_start_update_task(const char *backend_url) {
  if (strlen(backend_url) == 0) {
    ESP_LOGW(TAG, "Backend URL not set, OTA update task not started");
    return;
  }

  strncpy(s_backend_url, backend_url, sizeof(s_backend_url) - 1);

  xTaskCreate(ota_update_task, "ota_update", 8192, (void *)s_backend_url, 5,
              NULL);
  ESP_LOGI(TAG, "OTA update task created");
}
