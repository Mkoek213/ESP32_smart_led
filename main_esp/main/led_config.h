#ifndef LED_CONFIG_H
#define LED_CONFIG_H

#include <cstdint>

// LED Configuration Structure
struct RGBColor {
  uint8_t r;
  uint8_t g;
  uint8_t b;
};

struct LEDConfig {
  // Humidity thresholds (4 values: min, low-med boundary, med-high boundary, max)
  float humidity_thresholds[4];  // e.g., [0, 30, 70, 100]
  
  // RGB colors for each zone (3 colors for 3 zones)
  RGBColor colors[3];
  
  // Brightness settings
  uint8_t manual_brightness_pct;  // 0-100%
  bool auto_brightness;           // true = use photoresistor, false = use manual
  
  // Number of LEDs to activate (1-6)
  uint8_t num_leds_active;        // How many of the 6 LEDs to power up
  
  // LED timeout settings (in milliseconds)
  uint32_t no_motion_timeout_ms;  // Turn off after X ms of no motion (e.g., 15000 = 15s)
  uint32_t max_on_duration_ms;    // Maximum LED on time (e.g., 300000 = 5min)
  
  // Motion detection settings
  float distance_threshold_cm;    // Detection distance threshold in cm (e.g., 50.0)
};

// LED Configuration Manager
class LEDConfigManager {
public:
  static LEDConfigManager& getInstance();
  
  // Get current configuration
  const LEDConfig& getConfig() const { return config_; }
  
  // Update configuration
  void setConfig(const LEDConfig& new_config);
  
  // Update individual settings
  void setHumidityThresholds(const float thresholds[4]);
  void setColors(const RGBColor colors[3]);
  void setBrightness(uint8_t pct, bool auto_mode);
  
  // Persistence
  bool loadFromNVS();
  bool saveToNVS();
  
  // Helper: Get color for given humidity level
  RGBColor getColorForHumidity(float humidity) const;
  
  // Helper: Get brightness based on ambient light (0-100%)
  uint8_t getBrightnessForAmbientLight(uint8_t ambient_light_pct) const;

private:
  LEDConfigManager();
  ~LEDConfigManager() = default;
  LEDConfigManager(const LEDConfigManager&) = delete;
  LEDConfigManager& operator=(const LEDConfigManager&) = delete;
  
  void initDefaultConfig();
  
  LEDConfig config_;
  static const char* NVS_NAMESPACE;
  static const char* NVS_KEY;
};

#endif // LED_CONFIG_H
