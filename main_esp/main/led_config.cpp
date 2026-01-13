#include "led_config.h"
#include "esp_log.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <cstring>

static const char* TAG = "led_config";

const char* LEDConfigManager::NVS_NAMESPACE = "led_config";
const char* LEDConfigManager::NVS_KEY = "config";

LEDConfigManager& LEDConfigManager::getInstance() {
  static LEDConfigManager instance;
  return instance;
}

LEDConfigManager::LEDConfigManager() {
  initDefaultConfig();
  
  // Try to load from NVS, fallback to defaults if not found
  if (!loadFromNVS()) {
    ESP_LOGI(TAG, "No saved configuration, using defaults");
    saveToNVS();  // Save defaults for next boot
  }
}

void LEDConfigManager::initDefaultConfig() {
  // Default humidity thresholds: 0-30% (low), 30-70% (medium), 70-100% (high)
  config_.humidity_thresholds[0] = 0.0f;
  config_.humidity_thresholds[1] = 30.0f;
  config_.humidity_thresholds[2] = 70.0f;
  config_.humidity_thresholds[3] = 100.0f;
  
  // Default colors: Red (low humidity), Blue (medium), Green (high)
  config_.colors[0] = {255, 0, 0};    // Red
  config_.colors[1] = {0, 0, 255};    // Blue
  config_.colors[2] = {0, 255, 0};    // Green
  
  // Default brightness: 80%, auto-brightness enabled
  config_.manual_brightness_pct = 80;
  config_.auto_brightness = true;
  
  // Default: Use all 6 LEDs
  config_.num_leds_active = 5;
  
  // Default timeout settings
  config_.no_motion_timeout_ms = 15000;   // 15 seconds
  config_.max_on_duration_ms = 300000;    // 5 minutes
  
  // Default motion detection threshold
  config_.distance_threshold_cm = 50.0f;  // 50 cm
  
  ESP_LOGI(TAG, "Initialized default configuration");
}

void LEDConfigManager::setConfig(const LEDConfig& new_config) {
  config_ = new_config;
  saveToNVS();
  ESP_LOGI(TAG, "Configuration updated");
}

void LEDConfigManager::setHumidityThresholds(const float thresholds[4]) {
  memcpy(config_.humidity_thresholds, thresholds, sizeof(config_.humidity_thresholds));
  saveToNVS();
  ESP_LOGI(TAG, "Humidity thresholds updated: [%.1f, %.1f, %.1f, %.1f]",
           thresholds[0], thresholds[1], thresholds[2], thresholds[3]);
}

void LEDConfigManager::setColors(const RGBColor colors[3]) {
  memcpy(config_.colors, colors, sizeof(config_.colors));
  saveToNVS();
  ESP_LOGI(TAG, "Colors updated");
}

void LEDConfigManager::setBrightness(uint8_t pct, bool auto_mode) {
  config_.manual_brightness_pct = pct;
  config_.auto_brightness = auto_mode;
  saveToNVS();
  ESP_LOGI(TAG, "Brightness updated: %d%%, auto=%s", pct, auto_mode ? "true" : "false");
}

bool LEDConfigManager::loadFromNVS() {
  nvs_handle_t handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &handle);
  
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to open NVS namespace");
    return false;
  }
  
  size_t required_size = sizeof(LEDConfig);
  err = nvs_get_blob(handle, NVS_KEY, &config_, &required_size);
  nvs_close(handle);
  
  if (err != ESP_OK) {
    ESP_LOGW(TAG, "Failed to load configuration from NVS");
    return false;
  }
  
  ESP_LOGI(TAG, "Configuration loaded from NVS");
  return true;
}

bool LEDConfigManager::saveToNVS() {
  nvs_handle_t handle;
  esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &handle);
  
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to open NVS namespace for writing");
    return false;
  }
  
  err = nvs_set_blob(handle, NVS_KEY, &config_, sizeof(LEDConfig));
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to save configuration to NVS");
    nvs_close(handle);
    return false;
  }
  
  err = nvs_commit(handle);
  nvs_close(handle);
  
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "Failed to commit NVS");
    return false;
  }
  
  ESP_LOGI(TAG, "Configuration saved to NVS");
  return true;
}

RGBColor LEDConfigManager::getColorForHumidity(float humidity) const {
  // Determine which zone the humidity falls into
  if (humidity < config_.humidity_thresholds[1]) {
    return config_.colors[0];  // Low zone
  } else if (humidity < config_.humidity_thresholds[2]) {
    return config_.colors[1];  // Medium zone
  } else {
    return config_.colors[2];  // High zone
  }
}

uint8_t LEDConfigManager::getBrightnessForAmbientLight(uint8_t ambient_light_pct) const {
  if (!config_.auto_brightness) {
    // Use manual brightness
    return (uint8_t)((config_.manual_brightness_pct * 255) / 100);
  }
  
  // Auto-brightness based on ambient light
  // Dark (0-30%): 100% brightness
  // Medium (30-70%): 50% brightness  
  // Bright (70-100%): 20% brightness
  
  if (ambient_light_pct < 30) {
    return 255;  // 100%
  } else if (ambient_light_pct < 70) {
    return 128;  // 50%
  } else {
    return 51;   // 20%
  }
}
