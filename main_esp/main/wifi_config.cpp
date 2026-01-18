#include "wifi_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>

static const char* TAG = "wifi_config";
static const char* NVS_NAMESPACE = "wifi_cfg";
static const char* NVS_KEY_SSID = "ssid";
static const char* NVS_KEY_PASS = "password";
static const char* NVS_KEY_CUSTOMER_ID = "customer_id";
static const char* NVS_KEY_LOCATION_ID = "location_id";
static const char* NVS_KEY_DEVICE_ID = "device_id";
static const char* NVS_KEY_BROKER_URL = "broker_url";

bool wifi_config_nvs_init() {
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_LOGW(TAG, "NVS partition was truncated, erasing...");
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to init NVS: %s", esp_err_to_name(ret));
        return false;
    }
    return true;
}

bool wifi_config_load(WifiConfig& cfg) {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No saved WiFi config found");
        return false;
    }

    size_t required_size = sizeof(cfg.ssid);
    err = nvs_get_str(handle, NVS_KEY_SSID, cfg.ssid, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read SSID: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    required_size = sizeof(cfg.password);
    err = nvs_get_str(handle, NVS_KEY_PASS, cfg.password, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read password: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    required_size = sizeof(cfg.customerId);
    err = nvs_get_str(handle, NVS_KEY_CUSTOMER_ID, cfg.customerId, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read customerId: %s", esp_err_to_name(err));
        // Not fatal, might be an older config
    }

    required_size = sizeof(cfg.locationId);
    err = nvs_get_str(handle, NVS_KEY_LOCATION_ID, cfg.locationId, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read locationId: %s", esp_err_to_name(err));
        // Not fatal
    }

    required_size = sizeof(cfg.deviceId);
    err = nvs_get_str(handle, NVS_KEY_DEVICE_ID, cfg.deviceId, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read deviceId: %s", esp_err_to_name(err));
        // Not fatal
    }

    required_size = sizeof(cfg.brokerUrl);
    err = nvs_get_str(handle, NVS_KEY_BROKER_URL, cfg.brokerUrl, &required_size);
    if (err != ESP_OK) {
        ESP_LOGW(TAG, "Failed to read brokerUrl: %s", esp_err_to_name(err));
        // Not fatal, will use default
        cfg.brokerUrl[0] = '\0';
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Loaded WiFi config: SSID='%s'", cfg.ssid);
    ESP_LOGI(TAG, "Loaded IDs: Customer='%s', Location='%s', Device='%s'", cfg.customerId, cfg.locationId, cfg.deviceId);
    ESP_LOGI(TAG, "Loaded Broker: '%s'", cfg.brokerUrl);
    return true;
}

bool wifi_config_save(const WifiConfig& cfg) {
    if (strlen(cfg.ssid) == 0) {
        ESP_LOGE(TAG, "Cannot save empty SSID");
        return false;
    }

    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_set_str(handle, NVS_KEY_SSID, cfg.ssid);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write SSID: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_set_str(handle, NVS_KEY_PASS, cfg.password);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write password: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_set_str(handle, NVS_KEY_CUSTOMER_ID, cfg.customerId);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write customerId: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_set_str(handle, NVS_KEY_LOCATION_ID, cfg.locationId);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write locationId: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_set_str(handle, NVS_KEY_DEVICE_ID, cfg.deviceId);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write deviceId: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    if (strlen(cfg.brokerUrl) > 0) {
        err = nvs_set_str(handle, NVS_KEY_BROKER_URL, cfg.brokerUrl);
        if (err != ESP_OK) {
            ESP_LOGE(TAG, "Failed to write brokerUrl: %s", esp_err_to_name(err));
            nvs_close(handle);
            return false;
        }
    }

    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit NVS: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    nvs_close(handle);
    ESP_LOGI(TAG, "Saved WiFi config: SSID='%s'", cfg.ssid);
    ESP_LOGI(TAG, "Saved IDs: Customer='%s', Location='%s', Device='%s'", cfg.customerId, cfg.locationId, cfg.deviceId);
    ESP_LOGI(TAG, "Saved Broker: '%s'", cfg.brokerUrl);
    return true;
}

bool wifi_config_exists() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) {
        return false;
    }

    size_t required_size = 0;
    err = nvs_get_str(handle, NVS_KEY_SSID, nullptr, &required_size);
    nvs_close(handle);
    
    return (err == ESP_OK && required_size > 1);
}

bool wifi_config_erase() {
    nvs_handle_t handle;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS: %s", esp_err_to_name(err));
        return false;
    }

    err = nvs_erase_all(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to erase config: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }

    err = nvs_commit(handle);
    nvs_close(handle);
    
    ESP_LOGI(TAG, "WiFi config erased");
    return (err == ESP_OK);
}
