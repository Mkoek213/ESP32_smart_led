#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "esp_random.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_pm.h"

#include "ble_provisioning.h"
#include "sensor_task.h"
#include "sensor_manager.h"
#include "config.h"
#include "led_controller.h"
#include "led_config.h"
#include "ws2812b_controller.h"
#include "hc_sr04.h"
#include "person_counter.h"  // Thread-safe person counter
#include "latest_sensor_data.h"  // Thread-safe latest sensor readings
#include "wifi_config.h"
#include "wifi_station.h"
#include "bmp280.h"
#include "driver/i2c_master.h"

#include <ctime>

extern "C" {
#include "app_common.h"
#include "app_mqtt.h"
#include "app_sntp.h"
#include "ota_update.h"
#include "http_server.h"
#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
}

static const char *TAG = "main";

// Global LED controller for WS2812B
static WS2812BController* g_ws2812b = nullptr;

// Global HC-SR04 distance sensor controller
static HCSR04* g_hc_sr04 = nullptr;

// Global BMP280 handle
static bmp280_handle_t g_bmp280 = NULL;

// NimBLE callbacks
static void on_reset(int reason) {
    ESP_LOGE(TAG, "BLE stack resetting; reason=%d", reason);
}

static void on_sync(void) {
    ESP_LOGI(TAG, "BLE stack synced");
    xEventGroupSetBits(s_app_event_group, BLE_STACK_READY_BIT);
}

static void host_task(void *param) {
    ESP_LOGI(TAG, "BLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

// Tracking MQTT/SNTP state - accessible from button monitor task
static volatile bool mqtt_started = false;
static volatile bool sntp_started = false;

// Flag to indicate if animation is running (don't override with status colors in main loop)
static volatile bool animation_running = false;

// ADC handle for photoresistor
static adc_oneshot_unit_handle_t adc1_handle = NULL;

// Forward declarations
static uint8_t read_photoresistor(void);

// HTTP Server Callbacks
static void http_on_led_control(uint8_t red, uint8_t green, uint8_t blue, uint8_t brightness) {
    ESP_LOGI(TAG, "HTTP LED Control: R:%d G:%d B:%d Brightness:%d", red, green, blue, brightness);
    
    if (g_ws2812b != nullptr) {
        g_ws2812b->stop_animation();
        vTaskDelay(pdMS_TO_TICKS(100));
        g_ws2812b->clear();
        
        // Set all LEDs to requested color
        const LEDConfig& led_config = LEDConfigManager::getInstance().getConfig();
        uint8_t num_leds = led_config.num_leds_active;
        for (uint8_t i = 0; i < num_leds && i < WS2812B_NUM_LEDS; i++) {
            g_ws2812b->set_pixel_brightness(i, red, green, blue, brightness);
        }
        g_ws2812b->refresh();
        
        ESP_LOGI(TAG, "LEDs updated via HTTP API");
    }
}

static void http_on_config_update(const char* json_config) {
    ESP_LOGI(TAG, "HTTP Config Update: %s", json_config);
    // Parse and apply configuration updates
    // Could update LEDConfig, sensor thresholds, etc.
}

static const char* http_get_status(void) {
    static char status_json[512];
    
    // Get latest sensor data
    float temperature = LatestSensorData::get_temperature();
    float humidity = LatestSensorData::get_humidity();
    int person_count = PersonCounter::get();
    
    // Read photoresistor
    uint8_t ambient_light = read_photoresistor();
    
    // Build JSON status
    snprintf(status_json, sizeof(status_json),
        "{"
        "\"temperature\":%.1f,"
        "\"humidity\":%.1f,"
        "\"personCount\":%d,"
        "\"ambientLight\":%d,"
        "\"wifiConnected\":%s,"
        "\"firmwareVersion\":\"%s\""
        "}",
        temperature,
        humidity,
        person_count,
        ambient_light,
        wifi_station_is_connected() ? "true" : "false",
        ota_get_current_version()
    );
    
    return status_json;
}

// Initialize ADC for photoresistor
static void init_photoresistor_adc() {
  adc_oneshot_unit_init_cfg_t init_config = {
    .unit_id = ADC_UNIT_1,
    .clk_src = ADC_RTC_CLK_SRC_DEFAULT,
    .ulp_mode = ADC_ULP_MODE_DISABLE,
  };
  ESP_ERROR_CHECK(adc_oneshot_new_unit(&init_config, &adc1_handle));
  
  adc_oneshot_chan_cfg_t config = {
    .atten = PHOTORESISTOR_ADC_ATTEN,
    .bitwidth = PHOTORESISTOR_ADC_BITWIDTH,
  };
  ESP_ERROR_CHECK(adc_oneshot_config_channel(adc1_handle, PHOTORESISTOR_ADC_CHANNEL, &config));
  
  ESP_LOGI(TAG, "Photoresistor ADC initialized on GPIO%d", PHOTORESISTOR_GPIO);
}

// Read photoresistor and return ambient light percentage (0-100%)
static uint8_t read_photoresistor() {
  int adc_raw = 0;
  ESP_ERROR_CHECK(adc_oneshot_read(adc1_handle, PHOTORESISTOR_ADC_CHANNEL, &adc_raw));
  
  // DEBUG: Log removal for less spam (moved to periodic task)
  // static int log_counter = 0;
  // if (++log_counter >= 20) { ... }
  
  // Map ADC value (0-4095) to percentage (0-100%)
  // Clamp to calibrated min/max range
  if (adc_raw < PHOTORESISTOR_MIN_ADC) adc_raw = PHOTORESISTOR_MIN_ADC;
  if (adc_raw > PHOTORESISTOR_MAX_ADC) adc_raw = PHOTORESISTOR_MAX_ADC;
  
  // Linear mapping: (value - min) / (max - min) * 100
  uint8_t percentage = ((adc_raw - PHOTORESISTOR_MIN_ADC) * 100) / 
                       (PHOTORESISTOR_MAX_ADC - PHOTORESISTOR_MIN_ADC);
  
  return percentage;
}

// Task reading HC-SR04 distance sensor
static void distance_sensor_task(void *arg) {
  HCSR04 *sensor = static_cast<HCSR04 *>(arg);
  
  // Person counting variables - track detection SESSIONS not individual detections
  static bool in_detection_session = false;  // Track if we're in an active detection session
  
  // LED timer variables
  static int64_t last_motion_time = 0;        // Last time motion was detected
  static int64_t led_session_start_time = 0;  // When LED session started
  static bool leds_on = false;
  
  // Store current LED color (set once when LEDs activate, don't change during session)
  static uint8_t current_led_red = 0;
  static uint8_t current_led_green = 0;
  static uint8_t current_led_blue = 0;
  
  // Measure distance every 50ms for ultra-fast detection (20 times per second)
  while (true) {
    float distance_cm = sensor->measure_distance_cm();

    if (distance_cm > 0) {
      // Distance measurement successful (logging disabled to reduce clutter)
      
      // Get configurable detection threshold
      float detection_threshold = LEDConfigManager::getInstance().getConfig().distance_threshold_cm;
      
      // Check if someone is detected (distance < threshold)
      if (distance_cm < detection_threshold) {
        // Update last motion time (for LED timeout tracking)
        int64_t current_time = esp_timer_get_time() / 1000;
        last_motion_time = current_time;
        
        // Count person ONLY when starting a NEW detection session
        if (!in_detection_session) {
          PersonCounter::increment();
          in_detection_session = true;
          ESP_LOGI(TAG, "New person detected! Total count: %d", PersonCounter::get());
          
          // Start LED session if not already on
          if (!leds_on) {
            led_session_start_time = current_time;
          }
        }
        
        // Get latest BLE sensor data for LED color selection
        float temperature = LatestSensorData::get_temperature();
        float humidity = LatestSensorData::get_humidity();
        
        ESP_LOGD(TAG, "Motion detected! Distance: %.1f cm", distance_cm);
        ESP_LOGD(TAG, "Environmental Data - Temp: %.1f°C, Humidity: %.1f%%", temperature, humidity);
        
        // Get LED configuration
        LEDConfigManager& config_manager = LEDConfigManager::getInstance();
        const LEDConfig& led_config = config_manager.getConfig();
        
        // Determine LED color based on CONFIGURABLE humidity thresholds
        RGBColor color = config_manager.getColorForHumidity(humidity);
        uint8_t red = color.r;
        uint8_t green = color.g;
        uint8_t blue = color.b;
        
        ESP_LOGD(TAG, "Humidity-based color: R:%d G:%d B:%d", red, green, blue);
        
        // Determine LED brightness based on photoresistor (or manual setting)
        uint8_t ambient_light_pct;
        
        #if USE_REAL_PHOTORESISTOR
          // Read real photoresistor on GPIO34
          ambient_light_pct = read_photoresistor();
        #else
          // Simulate photoresistor reading (0-100% ambient light)
          ambient_light_pct = esp_random() % 101;
        #endif
        
        uint8_t brightness = config_manager.getBrightnessForAmbientLight(ambient_light_pct);
        
        if (led_config.auto_brightness) {
          ESP_LOGD(TAG, "Ambient light: %d%% → Auto-brightness: %d%%", 
                   ambient_light_pct, (brightness * 100) / 255);
        } else {
          ESP_LOGD(TAG, "Manual brightness: %d%%", (brightness * 100) / 255);
        }
        
        // Activate LEDs if not already on
        if (g_ws2812b != nullptr && !leds_on) {
          ESP_LOGI(TAG, "Activating LEDs...");
          g_ws2812b->stop_animation();
          
          // Give a moment for the animation task to fully stop and clear
          vTaskDelay(pdMS_TO_TICKS(100));
          
          // Clear all LEDs first
          g_ws2812b->clear();
          
          // Store the color for this session (won't change until LEDs turn off)
          current_led_red = red;
          current_led_green = green;
          current_led_blue = blue;
          
          // Set only the configured number of LEDs
          uint8_t num_leds = led_config.num_leds_active;
          for (uint8_t i = 0; i < num_leds && i < WS2812B_NUM_LEDS; i++) {
            g_ws2812b->set_pixel_brightness(i, red, green, blue, brightness);
          }
          g_ws2812b->refresh();
          
          ESP_LOGD(TAG, "%d LEDs set to R:%d G:%d B:%d at %d%% brightness", 
                   num_leds, red, green, blue, (brightness * 100) / 255);
          
          ESP_LOGI(TAG, "LEDs activated (will stay on while motion detected, max 5min)");
          leds_on = true;
        }
      } else {
        // No motion detected - end detection session when no longer in range
        in_detection_session = false;
      }
    } else {
      ESP_LOGW(TAG, "Distance measurement failed");
    }
    
    
    // Smart LED auto-off logic
    if (leds_on && g_ws2812b != nullptr) {
      int64_t current_time = esp_timer_get_time() / 1000;
      int64_t time_since_motion = current_time - last_motion_time;
      int64_t session_duration = current_time - led_session_start_time;
      
      // Get configurable timeout values
      const LEDConfig& timeout_config = LEDConfigManager::getInstance().getConfig();
      
      // Turn off LEDs if:
      // 1. No motion for configured timeout, OR
      // 2. Session exceeded configured maximum duration
      if (time_since_motion >= timeout_config.no_motion_timeout_ms || 
          session_duration >= timeout_config.max_on_duration_ms) {
        
        if (session_duration >= timeout_config.max_on_duration_ms) {
          ESP_LOGI(TAG, "Turning off LEDs (max duration %lus reached)", 
                   (unsigned long)(timeout_config.max_on_duration_ms / 1000));
        } else {
          ESP_LOGI(TAG, "Turning off LEDs (%lus of no motion)", 
                   (unsigned long)(timeout_config.no_motion_timeout_ms / 1000));
        }
        
        g_ws2812b->clear();
        g_ws2812b->refresh();
        leds_on = false;
        
        // End detection session
        in_detection_session = false;
      } else {
        // LEDs are still on - update brightness dynamically based on ambient light
        // Update every 1 second (50ms * 20 cycles)
        static int brightness_update_counter = 0;
        if (++brightness_update_counter >= 20) {
          brightness_update_counter = 0;
          
          LEDConfigManager& config_mgr = LEDConfigManager::getInstance();
          const LEDConfig& current_config = config_mgr.getConfig();
          
          // Only update if auto-brightness is enabled
          if (current_config.auto_brightness) {
            // Read current ambient light
            uint8_t ambient_light_pct;
            
            #if USE_REAL_PHOTORESISTOR
              // Read real photoresistor
              ambient_light_pct = read_photoresistor();
            #else
              // Simulate
              ambient_light_pct = esp_random() % 101;
            #endif
            
            // Calculate new brightness based on ambient light
            uint8_t new_brightness = config_mgr.getBrightnessForAmbientLight(ambient_light_pct);
            
            // Update LEDs with new brightness BUT KEEP THE SAME COLOR
            uint8_t num_leds = current_config.num_leds_active;
            g_ws2812b->clear();
            for (uint8_t i = 0; i < num_leds && i < WS2812B_NUM_LEDS; i++) {
              // Use stored color values (current_led_red/green/blue), only change brightness
              g_ws2812b->set_pixel_brightness(i, current_led_red, current_led_green, current_led_blue, new_brightness);
            }
            g_ws2812b->refresh();
            
            ESP_LOGI(TAG, "Updated LED brightness: %d%% (ambient light: %d%%)", 
                     (new_brightness * 100) / 255, ambient_light_pct);
          }
        }
      }
    }
    
    // Person count telemetry is now handled by sensor_task every 30 seconds
    // No duplicate telemetry needed here

    vTaskDelay(pdMS_TO_TICKS(50));  // Read every 50ms
  }
}

// Task monitorujący przycisk konfiguracyjny
static void button_monitor_task(void *arg) {
  LEDController *led = static_cast<LEDController *>(arg);

  // Konfiguracja GPIO przycisku
  gpio_config_t btn_cfg = {};
  btn_cfg.pin_bit_mask = (1ULL << CONFIG_BUTTON_GPIO);
  btn_cfg.mode = GPIO_MODE_INPUT;
  btn_cfg.pull_up_en = GPIO_PULLUP_ENABLE;
  btn_cfg.pull_down_en = GPIO_PULLDOWN_DISABLE;
  btn_cfg.intr_type = GPIO_INTR_DISABLE;
  gpio_config(&btn_cfg);

  int64_t press_start_time = 0;
  bool was_pressed = false;

  ESP_LOGI(TAG, "Button monitor task started on GPIO%d", CONFIG_BUTTON_GPIO);

  while (true) {
    int level = gpio_get_level(CONFIG_BUTTON_GPIO);
    bool is_pressed = (level == 0); // Active LOW (pullup)

    int64_t now = esp_timer_get_time() / 1000; // ms

    if (is_pressed && !was_pressed) {
      // Przycisk został naciśnięty
      press_start_time = now;
      ESP_LOGI(TAG, "Button pressed");
    }

    if (!is_pressed && was_pressed) {
      // Przycisk został zwolniony
      press_start_time = 0;
      ESP_LOGI(TAG, "Button released");
    }

    if (is_pressed && press_start_time > 0) {
      int64_t press_duration = now - press_start_time;

      if (press_duration >= CONFIG_BUTTON_LONG_PRESS_MS) {
        ESP_LOGI(TAG, "Long press detected! Entering BLE provisioning mode...");

        // Stop MQTT and SNTP
        if (mqtt_started) {
          ESP_LOGI(TAG, "Stopping MQTT...");
          app_mqtt_stop();
          mqtt_started = false;
        }
        if (sntp_started) {
          ESP_LOGI(TAG, "Stopping SNTP...");
          app_sntp_stop();
          sntp_started = false;
        }

        // Zatrzymaj WiFi
        wifi_station_stop();

        // Uruchom BLE provisioning
        ble_provisioning_start(led);

        // Czekaj aż provisioning się zakończy
        while (ble_provisioning_is_active()) {
          vTaskDelay(pdMS_TO_TICKS(1000));
        }

        // Po zakończeniu provisioning, wczytaj nową konfigurację i uruchom WiFi
        WifiConfig cfg;
        if (wifi_config_load(cfg)) {
          ESP_LOGI(TAG, "Restarting WiFi with new config...");
          wifi_station_start(led, &cfg);
        } else {
          ESP_LOGW(TAG, "No config saved, starting with default");
          wifi_station_start(led, nullptr);
        }

        press_start_time = 0;
      }
    }

    was_pressed = is_pressed;
    vTaskDelay(pdMS_TO_TICKS(50));
  }
}

extern "C" void app_main(void) {
  ESP_LOGI(TAG, "=== WiFi LED Project with BLE Provisioning ===");

  // Initialize global event group for MQTT/WiFi coordination
  s_app_event_group = xEventGroupCreate();
  if (s_app_event_group == NULL) {
    ESP_LOGE(TAG, "Failed to create event group!");
    return;
  }

  // Inicjalizacja NVS
  if (!wifi_config_nvs_init()) {
    ESP_LOGE(TAG, "Failed to init NVS!");
    return;
  }

  // Inicjalizacja MQTT client
  ESP_LOGI(TAG, "Initializing MQTT...");
  app_mqtt_init();

  // Inicjalizacja BLE Stack (NimBLE)
  ESP_LOGI(TAG, "Initializing BLE Stack...");
  ESP_ERROR_CHECK(nimble_port_init());
  ble_hs_cfg.reset_cb = on_reset;
  ble_hs_cfg.sync_cb = on_sync;
  ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

  ble_svc_gap_init();
  ble_svc_gatt_init();
  ble_svc_gap_device_name_set("ESP32_LED_Controller"); // Default name, overridden by provisioning or app

  // Zarejestruj serwisy provisioning (muszą być dostępne zawsze)
  ble_provisioning_init_services();
  
  // Start BLE host task
  nimble_port_freertos_init(host_task);

  // Wait for BLE stack to be ready
  ESP_LOGI(TAG, "Waiting for BLE stack sync...");
  xEventGroupWaitBits(s_app_event_group, BLE_STACK_READY_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
  ESP_LOGI(TAG, "BLE Stack ready");

  // Inicjalizacja LED controllera
  LEDController led(LED_GPIO, LED_BLINK_PERIOD_MS);

  // Inicjalizacja WS2812B LED strip
  WS2812BController ws2812b(WS2812B_GPIO, WS2812B_NUM_LEDS);
  if (!ws2812b.init()) {
    ESP_LOGE(TAG, "Failed to initialize WS2812B LED strip!");
    return;
  }
  g_ws2812b = &ws2812b;
  
  // Inicjalizacja HC-SR04 distance sensor
  HCSR04 hc_sr04(HC_SR04_TRIG_GPIO, HC_SR04_ECHO_GPIO);
  if (!hc_sr04.init()) {
    ESP_LOGE(TAG, "Failed to initialize HC-SR04 distance sensor!");
    return;
  }
  g_hc_sr04 = &hc_sr04;
  
  // Initialize photoresistor ADC
  ESP_LOGI(TAG, "Initializing photoresistor...");
  init_photoresistor_adc();

  // Initialize I2C and BMP280
  ESP_LOGI(TAG, "Initializing BMP280...");
  i2c_master_bus_config_t i2c_mst_config = {
      .i2c_port = -1,
      .sda_io_num = I2C_MASTER_SDA_IO,
      .scl_io_num = I2C_MASTER_SCL_IO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .intr_priority = 0,
      .trans_queue_depth = 0,
      .flags = {
          .enable_internal_pullup = 1,
          .allow_pd = false,
      },
  };
  
  i2c_master_bus_handle_t bus_handle;
  esp_err_t err = i2c_new_master_bus(&i2c_mst_config, &bus_handle);
  if (err != ESP_OK) {
      ESP_LOGE(TAG, "Failed to create I2C bus: %s", esp_err_to_name(err));
  } else {
      ESP_LOGI(TAG, "I2C bus created. Checking BMP280 addresses...");
      const uint8_t addrs[] = {0x76, 0x77};
      bool found = false;
      for (uint8_t addr : addrs) {
          esp_err_t probe_err = i2c_master_probe(bus_handle, addr, 50);
          if (probe_err == ESP_OK) {
              ESP_LOGI(TAG, "Found I2C device at address 0x%02x", addr);
              found = true;
          } else {
              ESP_LOGW(TAG, "No device at 0x%02x (Error: %s)", addr, esp_err_to_name(probe_err));
          }
      }
      if (!found) {
          ESP_LOGE(TAG, "No I2C devices found at standard BMP280 addresses! Check wiring/pull-ups.");
      }
      ESP_LOGI(TAG, "I2C check complete.");

      bmp280_config_t bmp_conf = BMP280_CONFIG_DEFAULT();
      err = bmp280_init(bus_handle, &bmp_conf, &g_bmp280);
      if (err != ESP_OK) {
          ESP_LOGE(TAG, "Failed to initialize BMP280: %s", esp_err_to_name(err));
          g_bmp280 = NULL;
      } else {
          ESP_LOGI(TAG, "BMP280 initialized successfully");
      }
  }
  
  // Start color brightness cycling animation - DISABLED (LEDs only turn on when motion detected)
  // ESP_LOGI(TAG, "Starting LED color brightness cycle");
  // ws2812b.power_up_animation();  // This will run in a separate task

  // Sprawdź czy istnieje zapisana konfiguracja WiFi
  WifiConfig cfg;
  bool has_config = wifi_config_load(cfg);

  if (!has_config) {
    ESP_LOGI(TAG, "No saved WiFi config found. Starting BLE provisioning...");

    // Brak konfiguracji - uruchom BLE provisioning
    ble_provisioning_start(&led);

    // Czekaj na zakończenie provisioning
    while (ble_provisioning_is_active()) {
      vTaskDelay(pdMS_TO_TICKS(1000));
    }

    // Spróbuj ponownie wczytać konfigurację
    has_config = wifi_config_load(cfg);

    if (!has_config) {
      ESP_LOGW(TAG, "Still no config after provisioning. Using defaults.");
    }
  }

  // Uruchom WiFi Station
  if (has_config) {
    ESP_LOGI(TAG, "Starting WiFi with saved config: SSID='%s'", cfg.ssid);
    wifi_station_start(&led, &cfg);
  } else {
    ESP_LOGI(TAG, "Starting WiFi with default config");
    wifi_station_start(&led, nullptr);
  }

  // Uruchom task monitorujący przycisk (pozwala na zmianę konfiguracji)
  xTaskCreate(button_monitor_task, "btn_monitor", 4096, &led, 5, NULL);
  
  // Initialize thread-safe person counter before starting tasks
  PersonCounter::init();
  
  // Initialize thread-safe latest sensor data cache
  LatestSensorData::init();
  
  // Start HC-SR04 distance sensor reading task
  xTaskCreate(distance_sensor_task, "distance_sensor", 4096, &hc_sr04, 3, NULL);
  
  // Start BLE sensor reading task
  sensor_reading_task_start(g_bmp280);
  
  // Initialize OTA update system
  ESP_LOGI(TAG, "Initializing OTA update system...");
  ota_update_init();
  
  // Register HTTP server callbacks
  http_server_callbacks_t http_callbacks = {
    .on_led_control = http_on_led_control,
    .on_config_update = http_on_config_update,
    .get_status = http_get_status
  };
  http_server_register_callbacks(&http_callbacks);
  
  // Mark animation as running - main loop will not override it
  animation_running = true;
  
  ESP_LOGI(
      TAG,
      "System initialized. Hold button for %dms to enter provisioning mode.",
      CONFIG_BUTTON_LONG_PRESS_MS);

  while (true) {
    if (wifi_station_is_connected()) {
      // WiFi connected - start SNTP if not already started
      if (!sntp_started) {
        ESP_LOGI(TAG, "WiFi connected - starting SNTP...");
        app_sntp_init();
        sntp_started = true;
      }

      // Start MQTT if not already started
      if (!mqtt_started) {
        // Give network stack a moment to stabilize (ARP, etc.) to avoid initial
        // connection timeout
        vTaskDelay(pdMS_TO_TICKS(3000));

        ESP_LOGI(TAG, "WiFi connected - starting MQTT...");
        app_mqtt_start();
        app_mqtt_start_publishing_task();
        mqtt_started = true;
        ESP_LOGI(TAG, "MQTT started and publishing");
        
        // Start HTTP REST API server
        ESP_LOGI(TAG, "Starting HTTP REST API server...");
        http_server_start();
        
        // Mark current boot as valid (OTA rollback protection)
        ota_mark_boot_valid();
        
        // Start OTA update background task
        ESP_LOGI(TAG, "Starting OTA update background task...");
        ota_start_update_task(BACKEND_URL);
      }
    } else {
      // WiFi disconnected - reset state
      if (mqtt_started || sntp_started) {
        ESP_LOGI(TAG, "WiFi disconnected - resetting MQTT/SNTP state");
        mqtt_started = false;
        sntp_started = false;
        
        // Stop HTTP server when WiFi is down
        http_server_stop();
      }
    }

    // Main loop monitoring task - animation continues running independently in background
    // Log WiFi/MQTT status without interrupting LED animation
    ESP_LOGD(TAG, "WiFi: %s, MQTT: %s, SNTP: %s", 
             wifi_station_is_connected() ? "connected" : "disconnected",
             mqtt_started ? "started" : "stopped",
             sntp_started ? "started" : "stopped");

    vTaskDelay(pdMS_TO_TICKS(5000));  // Check status every 5 seconds instead of 1 second
  }
}
