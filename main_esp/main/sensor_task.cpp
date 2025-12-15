#include "sensor_manager.h"
#include "sensor_task.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "app_common.h"
#include <ctime>
#include <cstdlib>

static const char* TAG = "sensor_task";
static const int SENSOR_READ_INTERVAL_MS = 30000; // Read every 30 seconds
static const int MOTION_CHECK_INTERVAL_MS = 5000; // Check for motion every 5s

#include "esp_log.h"
#include "esp_event.h"
#include "freertos/event_groups.h"
#include "sensor_manager.h"
#include "config.h"
#include "ble_provisioning.h"
#include "bmp280.h"  // For pressure sensor
#include "person_counter.h"  // Thread-safe person counter
#include "latest_sensor_data.h"  // Thread-safe latest sensor readings

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/ble_gatt.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <strings.h>

namespace {
    constexpr const char* BLE_TAG = "ble_gatt_client";
    constexpr const char kTargetName_Sensor[] = "ATC_8E4B89";

    // UUIDs
    constexpr const uint16_t kServiceUUIDEnv = 0x181A;
    constexpr const uint16_t kCharUUIDTemp = 0x2A6E;
    constexpr const uint16_t kCharUUIDHumidity = 0x2A6F;

    constexpr const uint16_t kServiceUUIDBattery = 0x180F;
    constexpr const uint16_t kCharUUIDBattery = 0x2A19;

    constexpr const char *kTempLabel = "Temperature";
    constexpr const char *kHumidityLabel = "Humidity";
    constexpr const char *kBatteryLabel = "Battery";

    struct SensorData {
        float temperature = 0.0f;
        float humidity = 0.0f;
        uint8_t battery = 0;
        bool valid = false;
    };

    struct TargetContext {
        uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE;
        bool connecting = false;

        uint16_t env_start_handle = 0;
        uint16_t env_end_handle = 0;
        uint16_t temp_handle = 0;
        uint16_t humidity_handle = 0;

        uint16_t batt_start_handle = 0;
        uint16_t batt_end_handle = 0;
        uint16_t battery_handle = 0;
        
        SensorData current_data;
    };

    static TargetContext g_ctx;
    static SemaphoreHandle_t s_ble_sem = nullptr;

    void reset_context() {
        g_ctx = TargetContext{};
        // Keep semaphore logic separate or ensure it's not overwritten if it was part of struct in previous attempts (it was not)
    }

    // Forward declarations
    int characteristic_disc_cb(uint16_t, const struct ble_gatt_error *, const struct ble_gatt_chr *, void *);
    int service_disc_cb(uint16_t, const struct ble_gatt_error *, const struct ble_gatt_svc *, void *);


    int value_read_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                      struct ble_gatt_attr *attr, void *arg) {

        if (error->status == 0) {
            uint8_t buffer[8] = {0};
            int len = OS_MBUF_PKTLEN(attr->om);
            if (len > (int)sizeof(buffer)) len = sizeof(buffer);
            os_mbuf_copydata(attr->om, 0, len, buffer);

            if (arg == kTempLabel && len >= 2) {
                const int16_t raw = (int16_t)(buffer[0] | (buffer[1] << 8));
                g_ctx.current_data.temperature = (float)raw / 100.0f;
                ESP_LOGI(BLE_TAG, "Read Temp: %.2f", g_ctx.current_data.temperature);
            } else if (arg == kHumidityLabel && len >= 2) {
                const uint16_t raw = (uint16_t)(buffer[0] | (buffer[1] << 8));
                g_ctx.current_data.humidity = (float)raw / 100.0f;
                ESP_LOGI(BLE_TAG, "Read Hum: %.2f", g_ctx.current_data.humidity);
            } else if (arg == kBatteryLabel && len >= 1) {
                g_ctx.current_data.battery = buffer[0];
                ESP_LOGI(BLE_TAG, "Read Bat: %u%%", g_ctx.current_data.battery);
            }
        }

        // Chain reads
        if (arg == kTempLabel && g_ctx.humidity_handle) {
             ble_gattc_read(conn_handle, g_ctx.humidity_handle, value_read_cb, (void*)kHumidityLabel);
        } else if ((arg == kTempLabel || arg == kHumidityLabel) && g_ctx.battery_handle) {
             ble_gattc_read(conn_handle, g_ctx.battery_handle, value_read_cb, (void*)kBatteryLabel);
        } else {
            // Done
            ESP_LOGI(BLE_TAG, "All reads finished. Disconnecting.");
            if (error->status == 0) g_ctx.current_data.valid = true;
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }

    void request_measurements() {
        if (g_ctx.conn_handle == BLE_HS_CONN_HANDLE_NONE) return;

        if (g_ctx.temp_handle) {
            ble_gattc_read(g_ctx.conn_handle, g_ctx.temp_handle, value_read_cb, (void *)kTempLabel);
        } else if (g_ctx.humidity_handle) {
            ble_gattc_read(g_ctx.conn_handle, g_ctx.humidity_handle, value_read_cb, (void *)kHumidityLabel);
        } else if (g_ctx.battery_handle) {
            ble_gattc_read(g_ctx.conn_handle, g_ctx.battery_handle, value_read_cb, (void *)kBatteryLabel);
        } else {
            ble_gap_terminate(g_ctx.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
    }

    int characteristic_disc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                               const struct ble_gatt_chr *chr, void *arg) {
        
        const uint16_t service_uuid = arg ? *(const uint16_t *)arg : 0;

        if (error->status == 0 && chr) {
            uint16_t uuid16 = ble_uuid_u16(&chr->uuid.u);
            if (service_uuid == kServiceUUIDEnv) {
                if (uuid16 == kCharUUIDTemp) {
                    g_ctx.temp_handle = chr->val_handle;
                    ESP_LOGI(BLE_TAG, "Found Temp handle: %u", g_ctx.temp_handle);
                } else if (uuid16 == kCharUUIDHumidity) {
                    g_ctx.humidity_handle = chr->val_handle;
                    ESP_LOGI(BLE_TAG, "Found Hum handle: %u", g_ctx.humidity_handle);
                }
            } else if (service_uuid == kServiceUUIDBattery) {
                if (uuid16 == kCharUUIDBattery) {
                    g_ctx.battery_handle = chr->val_handle;
                    ESP_LOGI(BLE_TAG, "Found Bat handle: %u", g_ctx.battery_handle);
                }
            }
            return 0;
        }

        if (error->status == BLE_HS_EDONE) {
            if (service_uuid == kServiceUUIDEnv && g_ctx.batt_start_handle != 0) {
                ble_gattc_disc_all_chrs(conn_handle, g_ctx.batt_start_handle, g_ctx.batt_end_handle,
                                        characteristic_disc_cb, (void *)&kServiceUUIDBattery);
            } else {
                ESP_LOGI(BLE_TAG, "Discovery done. Requesting measurements...");
                request_measurements();
            }
        }
        return 0;
    }

    int service_disc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                        const struct ble_gatt_svc *svc, void *arg) {
        if (error->status == 0 && svc) {
            const uint16_t uuid16 = ble_uuid_u16(&svc->uuid.u);
            if (uuid16 == kServiceUUIDEnv) {
                g_ctx.env_start_handle = svc->start_handle;
                g_ctx.env_end_handle = svc->end_handle;
                ESP_LOGI(BLE_TAG, "Found Env Service: %u..%u", g_ctx.env_start_handle, g_ctx.env_end_handle);
            } else if (uuid16 == kServiceUUIDBattery) {
                g_ctx.batt_start_handle = svc->start_handle;
                g_ctx.batt_end_handle = svc->end_handle;
                ESP_LOGI(BLE_TAG, "Found Bat Service: %u..%u", g_ctx.batt_start_handle, g_ctx.batt_end_handle);
            }
            return 0;
        }
        if (error->status == BLE_HS_EDONE) {
             if (g_ctx.env_start_handle != 0) {
                 ble_gattc_disc_all_chrs(conn_handle, g_ctx.env_start_handle, g_ctx.env_end_handle,
                                         characteristic_disc_cb, (void *)&kServiceUUIDEnv);
             } else if (g_ctx.batt_start_handle != 0) {
                 ble_gattc_disc_all_chrs(conn_handle, g_ctx.batt_start_handle, g_ctx.batt_end_handle,
                                         characteristic_disc_cb, (void *)&kServiceUUIDBattery);
             } else {
                 ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
             }
        }
        return 0;
    }
    
    int gap_event(struct ble_gap_event *event, void *arg);

    void start_scan() {
        if (ble_provisioning_is_active()) return;
        
        // Don't full reset here if we are just restarting scan from disconnect, 
        // but for a new cycle from task it is called explicitly.
        
        // But wait, user code reset_context() in start_scan logic? No, only disconnect calls reset context and start scan.
        // User code: start_scan checks if connected.
        // Our task logic calls start_scan every loop. 
        // We should ensure we are clean.
        
        if (g_ctx.connecting || g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE) return;

        uint8_t own_addr_type;
        ble_hs_id_infer_auto(0, &own_addr_type);
        
        struct ble_gap_disc_params params = {0};
        params.passive = 0; // Active scan
        params.itvl = 0;
        params.window = 0;
        params.filter_duplicates = 1;

        ble_gap_disc(own_addr_type, 10000, &params, gap_event, nullptr);
        ESP_LOGI(BLE_TAG, "Scanning...");
    }

    int gap_event(struct ble_gap_event *event, void *arg) {
        switch (event->type) {
            case BLE_GAP_EVENT_DISC: {
                if (g_ctx.connecting || g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE) return 0;

                struct ble_hs_adv_fields fields;
                if (ble_hs_adv_parse_fields(&fields, event->disc.data, event->disc.length_data) == 0) {
                    if (fields.name_len == strlen(kTargetName_Sensor) &&
                        memcmp(fields.name, kTargetName_Sensor, fields.name_len) == 0) {
                        
                        ESP_LOGI(BLE_TAG, "Found target!");
                        ble_gap_disc_cancel();
                        
                        uint8_t own_addr_type;
                        ble_hs_id_infer_auto(0, &own_addr_type);
                        ble_gap_connect(own_addr_type, &event->disc.addr, 30000, nullptr, gap_event, nullptr);
                        g_ctx.connecting = true;
                    }
                }
                break;
            }
            case BLE_GAP_EVENT_CONNECT:
                g_ctx.connecting = false;
                if (event->connect.status == 0) {
                    g_ctx.conn_handle = event->connect.conn_handle;
                    ESP_LOGI(BLE_TAG, "Connected. Discovering services...");
                    
                    // Reset handles but keep connection
                    uint16_t h = g_ctx.conn_handle;
                    reset_context();
                    g_ctx.conn_handle = h;
                    
                    ble_gattc_disc_all_svcs(g_ctx.conn_handle, service_disc_cb, nullptr);
                } else {
                    start_scan(); // Retry
                }
                break;
            case BLE_GAP_EVENT_DISCONNECT:
                ESP_LOGI(BLE_TAG, "Disconnected");
                // Do not reset full context to preserve data for task
                g_ctx.conn_handle = BLE_HS_CONN_HANDLE_NONE; 
                g_ctx.connecting = false;
                // Signal completion to task
                if (s_ble_sem) xSemaphoreGive(s_ble_sem);
                break;
            
            case BLE_GAP_EVENT_DISC_COMPLETE:
                if (!g_ctx.connecting) {
                    // Task handles timeout by semaphore timeout, so we don't necessarily need to restart scan immediately 
                    // if we are controlled by task loop.
                    // However, user code restarts scan.
                    // For us: if we didn't find device, task should timeout. 
                    // Just let task handle it.
                }
                break;
        }
        return 0;
    }
}

static void sensor_reading_task(void* arg) {
    ESP_LOGI(TAG, "Sensor reading task started");
    
    // Init semaphore for BLE
    s_ble_sem = xSemaphoreCreateBinary();

    // Wait for time sync and BLE stack
    xEventGroupWaitBits(s_app_event_group, TIME_SYNCED_BIT | BLE_STACK_READY_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "Sync complete, starting loop");

    while (true) {
        // Check provisioning
        if (ble_provisioning_is_active()) {
            ESP_LOGI(TAG, "Provisioning active, pausing sensor reads");
            vTaskDelay(pdMS_TO_TICKS(5000));
            continue;
        }

        // BLE sensor mode - scan for physical BLE device
        ESP_LOGI(TAG, "Starting BLE scan for sensor...");
        start_scan();

        // Wait for completion (timeout 30s to handle slow BLE connections)
        if (xSemaphoreTake(s_ble_sem, pdMS_TO_TICKS(30000)) == pdTRUE) {
            // Check if we got valid data
            if (g_ctx.current_data.valid) {
                 // Update the latest sensor data cache (always available for LED colors)
                 LatestSensorData::update(g_ctx.current_data.temperature, 
                                         g_ctx.current_data.humidity);
                 
                 time_t now;
                 time(&now);
                 Telemetry data;
                 data.timestamp = (int64_t)now;
                 data.humidity = g_ctx.current_data.humidity;      // From BLE
                 data.temperature = g_ctx.current_data.temperature; // From BLE
                 data.pressure = 1013.25f; // Default sea level pressure (BMP280 not connected)
                 data.person_count = PersonCounter::get_and_reset(); // Thread-safe get and reset
                 
                 SensorManager::getInstance().enqueue(data);
                 ESP_LOGI(TAG, "BLE telemetry: T=%.2f H=%.2f PersonCount=%d", 
                          data.temperature, data.humidity, data.person_count);
            } else {
                ESP_LOGW(TAG, "BLE transaction finished but no valid data");
            }
        } else {
            ESP_LOGW(TAG, "BLE timeout - cancelling");
            ble_gap_disc_cancel();
            if (g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                ble_gap_terminate(g_ctx.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(SENSOR_READ_INTERVAL_MS));
    }
}

void sensor_reading_task_start(void) {
    xTaskCreate(sensor_reading_task, "sensor_read", 4096, NULL, 5, NULL);
}

