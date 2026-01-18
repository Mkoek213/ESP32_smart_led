#include "ble_provisioning.h"
#include "wifi_config.h"
#include "esp_log.h"
#include <cstring>
#include "wifi_station.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/timers.h"
#include "esp_system.h"
#include "esp_mac.h"
#include "esp_random.h"
#include "nvs_flash.h"
#include "nvs.h"

extern "C" {
    #include "nimble/nimble_port.h"
    #include "nimble/nimble_port_freertos.h"
    #include "host/ble_hs.h"
    #include "host/ble_uuid.h"
    #include "host/util/util.h"
    #include "services/gap/ble_svc_gap.h"
    #include "services/gatt/ble_svc_gatt.h"
    #include "store/config/ble_store_config.h"
}

static const char* TAG = "ble_prov";

// Nazwa urządzenia widoczna przez BLE
static const char* DEVICE_NAME = "LED_WiFi_Config";

// Provisioning timeout (60 seconds)
#define PROVISIONING_TIMEOUT_MS 60000

// NVS keys for PoP storage
#define NVS_NAMESPACE_DEVICE "device"
#define NVS_KEY_POP "pop"
#define POP_LENGTH 32

// uuid dla provisioning service
static ble_uuid128_t UUID_PROV_SVC = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
);

// uuid dla charakterystyki MAC ADDRESS (read)
static ble_uuid128_t UUID_CHR_MAC = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xe0
);

// uuid dla charakterystyki PROOF OF POSSESSION (read)
static ble_uuid128_t UUID_CHR_POP = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xe1
);

// uuid dla charakterystyki SSID (write)
static ble_uuid128_t UUID_CHR_SSID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf1
);

// uuid dla charakterystyki PASSWORD (write)
static ble_uuid128_t UUID_CHR_PASS = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf2
);

// uuid dla charakterystyki STATUS (read/notify)
static ble_uuid128_t UUID_CHR_STATUS = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf3
);

// uuid dla charakterystyki COMMAND (write)
static ble_uuid128_t UUID_CHR_CMD = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf4
);

// uuid dla charakterystyki CUSTOMER_ID (write)
static ble_uuid128_t UUID_CHR_CUSTOMER_ID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf5
);

// uuid dla charakterystyki LOCATION_ID (write)
static ble_uuid128_t UUID_CHR_LOCATION_ID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf6
);

// uuid dla charakterystyki DEVICE_ID (write)
static ble_uuid128_t UUID_CHR_DEVICE_ID = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf7
);

// uuid dla charakterystyki IDS (write)
static ble_uuid128_t UUID_CHR_IDS = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf5
);

// uuid dla charakterystyki BROKER_URL (write)
static ble_uuid128_t UUID_CHR_BROKER_URL = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf8
);

static LEDController* s_led = nullptr;
static uint8_t s_own_addr_type = 0;
static bool s_provisioning_active = false;
static WifiConfig s_temp_config = {};
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_status_val_handle = 0;
static TimerHandle_t s_provisioning_timer = nullptr;

// wartosci statusu zwracane przez płytke
enum ProvStatus : uint8_t {
    STATUS_READY = 0x00,
    STATUS_SSID_SET = 0x01,
    STATUS_PASS_SET = 0x02,
    STATUS_SAVED = 0x03,
    STATUS_ERROR = 0xFF
};

static uint8_t s_status = STATUS_READY;

// Provisioning timeout callback
static void provisioning_timeout_cb(TimerHandle_t xTimer)
{
    (void)xTimer;
    ESP_LOGW(TAG, "Provisioning timeout - stopping BLE");
    ble_provisioning_stop();
}

static void notify_status_if_possible()
{
    if (s_conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }
    if (s_status_val_handle == 0) {
        return;
    }

    int rc = ble_gatts_notify(s_conn_handle, s_status_val_handle);
    if (rc != 0) {
        ESP_LOGW(TAG, "ble_gatts_notify failed: %d", rc);
    }
}


static int gap_event(struct ble_gap_event *event, void *arg);
static void start_advertising();

// ========== MAC Address & Proof of Possession Functions ==========

// Get device MAC address
void ble_provisioning_get_mac_address(uint8_t mac[6]) {
    esp_read_mac(mac, ESP_MAC_WIFI_STA);
}

// Generate or load Proof of Possession (PoP)
bool ble_provisioning_get_pop(uint8_t pop[32]) {
    nvs_handle_t handle;
    esp_err_t err;
    
    // Try to open NVS
    err = nvs_open(NVS_NAMESPACE_DEVICE, NVS_READWRITE, &handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to open NVS for PoP: %s", esp_err_to_name(err));
        return false;
    }
    
    // Try to read existing PoP
    size_t required_size = POP_LENGTH;
    err = nvs_get_blob(handle, NVS_KEY_POP, pop, &required_size);
    
    if (err == ESP_OK && required_size == POP_LENGTH) {
        // PoP exists, return it
        ESP_LOGI(TAG, "Loaded existing PoP from NVS");
        nvs_close(handle);
        return true;
    }
    
    // Generate new PoP (first boot)
    ESP_LOGI(TAG, "Generating new PoP (first boot)");
    esp_fill_random(pop, POP_LENGTH);
    
    // Save to NVS
    err = nvs_set_blob(handle, NVS_KEY_POP, pop, POP_LENGTH);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to save PoP: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    
    err = nvs_commit(handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to commit PoP: %s", esp_err_to_name(err));
        nvs_close(handle);
        return false;
    }
    
    nvs_close(handle);
    
    // Log PoP in hex for debugging
    ESP_LOGI(TAG, "PoP generated and saved:");
    ESP_LOG_BUFFER_HEX(TAG, pop, POP_LENGTH);
    
    return true;
}

// Factory reset - wipe WiFi credentials and return to unclaimed state
void ble_provisioning_factory_reset() {
    ESP_LOGW(TAG, "Factory reset initiated - wiping all credentials");
    
    // Wipe WiFi config
    wifi_config_erase();
    
    // Note: We DO NOT wipe PoP - it persists across factory resets
    // The backend uses PoP to validate the device, so it should remain constant
    // Only a full flash erase would reset the PoP
    
    ESP_LOGI(TAG, "Factory reset complete - device ready for re-provisioning");
    
    // Restart device to apply changes
    vTaskDelay(pdMS_TO_TICKS(1000));
    esp_restart();
}

// ========== End MAC & PoP Functions ==========

// callback dla dostępu do charakterystyk GATT
static int gatt_chr_access_cb(uint16_t conn_handle,
                              uint16_t attr_handle,
                              struct ble_gatt_access_ctxt *ctxt,
                              void *arg)
{
    (void)conn_handle;
    (void)attr_handle;
    (void)arg;

    const ble_uuid_t* uuid = ctxt->chr->uuid;
    
    // odczyt MAC ADDRESS
    if (ble_uuid_cmp(uuid, &UUID_CHR_MAC.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            uint8_t mac[6];
            ble_provisioning_get_mac_address(mac);
            ESP_LOGI(TAG, "Client reading MAC: %02X:%02X:%02X:%02X:%02X:%02X",
                     mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
            int rc = os_mbuf_append(ctxt->om, mac, 6);
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // odczyt PROOF OF POSSESSION
    if (ble_uuid_cmp(uuid, &UUID_CHR_POP.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            uint8_t pop[POP_LENGTH];
            if (ble_provisioning_get_pop(pop)) {
                ESP_LOGI(TAG, "Client reading PoP");
                ESP_LOG_BUFFER_HEX(TAG, pop, POP_LENGTH);
                int rc = os_mbuf_append(ctxt->om, pop, POP_LENGTH);
                return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
            } else {
                ESP_LOGE(TAG, "Failed to get PoP");
                return BLE_ATT_ERR_UNLIKELY;
            }
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // ustawianie SSID
    if (ble_uuid_cmp(uuid, &UUID_CHR_SSID.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Reset timer on activity
            if (s_provisioning_timer != nullptr) {
                xTimerReset(s_provisioning_timer, 0);
            }

            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len > WIFI_SSID_MAX_LEN) {
                ESP_LOGW(TAG, "SSID too long: %d", len);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            
            memset(s_temp_config.ssid, 0, sizeof(s_temp_config.ssid));
            os_mbuf_copydata(ctxt->om, 0, len, s_temp_config.ssid);
            s_temp_config.ssid[len] = '\0';
            
            ESP_LOGI(TAG, "SSID received: '%s'", s_temp_config.ssid);
            s_status = STATUS_SSID_SET;
            notify_status_if_possible();
            return 0;
        }
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            // Allow reading back the current SSID
            int rc = os_mbuf_append(ctxt->om, s_temp_config.ssid, strlen(s_temp_config.ssid));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // ustawianie PASSWORD
    if (ble_uuid_cmp(uuid, &UUID_CHR_PASS.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Reset timer on activity
            if (s_provisioning_timer != nullptr) {
                xTimerReset(s_provisioning_timer, 0);
            }

            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len > WIFI_PASS_MAX_LEN) {
                ESP_LOGW(TAG, "Password too long: %d", len);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            
            memset(s_temp_config.password, 0, sizeof(s_temp_config.password));
            os_mbuf_copydata(ctxt->om, 0, len, s_temp_config.password);
            s_temp_config.password[len] = '\0';
            
            ESP_LOGI(TAG, "Password received (len=%d)", len);
            s_status = STATUS_PASS_SET;
            notify_status_if_possible();
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // zwracanie STATUS
    if (ble_uuid_cmp(uuid, &UUID_CHR_STATUS.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_READ_CHR) {
            int rc = os_mbuf_append(ctxt->om, &s_status, sizeof(s_status));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // wpisywanie COMMAND
    // komendy: 0x01 = SAVE, 0x02 = RESET (clear config), 0x03 = FACTORY_RESET (unbind device)
    if (ble_uuid_cmp(uuid, &UUID_CHR_CMD.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Reset timer on activity
            if (s_provisioning_timer != nullptr) {
                xTimerReset(s_provisioning_timer, 0);
            }

            uint8_t cmd = 0;
            os_mbuf_copydata(ctxt->om, 0, 1, &cmd);
            
            ESP_LOGI(TAG, "Command received: 0x%02X", cmd);
            
            if (cmd == 0x01) { // komenda zapisu
                if (strlen(s_temp_config.ssid) == 0) {
                    ESP_LOGW(TAG, "Cannot save: SSID empty");
                    s_status = STATUS_ERROR;
                    notify_status_if_possible();
                    return BLE_ATT_ERR_UNLIKELY;
                }
                
                if (wifi_config_save(s_temp_config)) {
                    ESP_LOGI(TAG, "WiFi config saved successfully");
                    s_status = STATUS_SAVED;
                    notify_status_if_possible();

                    // jesli jest połączenie BLE, zakończ je
                    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                        int term_rc = ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                        ESP_LOGI(TAG, "Terminated BLE connection (rc=%d)", term_rc);
                    }
                    // zatrzymaj reklame BLE
                    ble_gap_adv_stop();
                    s_provisioning_active = false;

                    // zrestartuj urządzenie, aby zastosować nową konfigurację
                    vTaskDelay(pdMS_TO_TICKS(200));
                    ESP_LOGI(TAG, "Rebooting to apply WiFi config...");
                    esp_restart();
                } else {
                    ESP_LOGE(TAG, "Failed to save WiFi config");
                    s_status = STATUS_ERROR;
                    notify_status_if_possible();
                }
                return 0;
            }
            else if (cmd == 0x02) { // komenda na reset config (clear WiFi only)
                wifi_config_erase();
                memset(&s_temp_config, 0, sizeof(s_temp_config));
                s_status = STATUS_READY;
                notify_status_if_possible();
                ESP_LOGI(TAG, "WiFi config erased");
                return 0;
            }
            else if (cmd == 0x03) { // komenda na factory reset (unbind device)
                ESP_LOGW(TAG, "Factory reset command received from backend");
                s_status = STATUS_READY;
                notify_status_if_possible();
                
                // Give time for status notification to be sent
                vTaskDelay(pdMS_TO_TICKS(200));
                
                // Terminate BLE connection
                if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
                    ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
                }
                
                // Perform factory reset (wipes WiFi, restarts device)
                ble_provisioning_factory_reset();
                return 0;
            }
            
            return BLE_ATT_ERR_UNLIKELY;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    // obsluga zapisu IDS
    if (ble_uuid_cmp(uuid, &UUID_CHR_IDS.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            // Reset timer on activity
            if (s_provisioning_timer != nullptr) {
                xTimerReset(s_provisioning_timer, 0);
            }

            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            
            // Max length check
            if (len >= sizeof(s_temp_config.customerId) + sizeof(s_temp_config.locationId) + sizeof(s_temp_config.deviceId) + 2) {
                ESP_LOGE(TAG, "IDs payload too long: %d", len);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            char buf[128];
            if (len >= sizeof(buf)) {
                 ESP_LOGE(TAG, "IDs payload too large for buffer");
                 return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }
            os_mbuf_copydata(ctxt->om, 0, len, buf);
            buf[len] = '\0';
            
            ESP_LOGI(TAG, "Received IDs payload: '%s'", buf);

            // Parse "customerId,locationId,deviceId"
            char* token = strtok(buf, ",");
            if (token) {
                strncpy(s_temp_config.customerId, token, sizeof(s_temp_config.customerId) - 1);
                s_temp_config.customerId[sizeof(s_temp_config.customerId) - 1] = '\0';
            } else {
                ESP_LOGE(TAG, "Failed to parse customerId");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            token = strtok(NULL, ",");
            if (token) {
                strncpy(s_temp_config.locationId, token, sizeof(s_temp_config.locationId) - 1);
                s_temp_config.locationId[sizeof(s_temp_config.locationId) - 1] = '\0';
            } else {
                ESP_LOGE(TAG, "Failed to parse locationId");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            token = strtok(NULL, ",");
            if (token) {
                strncpy(s_temp_config.deviceId, token, sizeof(s_temp_config.deviceId) - 1);
                s_temp_config.deviceId[sizeof(s_temp_config.deviceId) - 1] = '\0';
            } else {
                ESP_LOGE(TAG, "Failed to parse deviceId");
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            ESP_LOGI(TAG, "Parsed IDs: Customer='%s', Location='%s', Device='%s'", 
                s_temp_config.customerId, s_temp_config.locationId, s_temp_config.deviceId);

            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }

    // obsluga zapisu BROKER_URL
    if (ble_uuid_cmp(uuid, &UUID_CHR_BROKER_URL.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
            if (s_provisioning_timer != nullptr) {
                xTimerReset(s_provisioning_timer, 0);
            }

            uint16_t len = OS_MBUF_PKTLEN(ctxt->om);
            if (len > BROKER_URL_MAX_LEN) {
                ESP_LOGW(TAG, "Broker URL too long: %d", len);
                return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            }

            memset(s_temp_config.brokerUrl, 0, sizeof(s_temp_config.brokerUrl));
            os_mbuf_copydata(ctxt->om, 0, len, s_temp_config.brokerUrl);
            s_temp_config.brokerUrl[len] = '\0';

            ESP_LOGI(TAG, "Broker URL received: '%s'", s_temp_config.brokerUrl);
            return 0;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

static struct ble_gatt_chr_def prov_chrs[9];
static struct ble_gatt_svc_def gatt_svcs[2];

// Descriptors for characteristics (User Description)
static const char* desc_mac = "Device MAC Address";
static const char* desc_pop = "Proof of Possession";
static const char* desc_ssid = "WiFi SSID";
static const char* desc_pass = "WiFi Password";
static const char* desc_status = "Provisioning Status";
static const char* desc_cmd = "Command";
static const char* desc_ids = "Device IDs (JSON)";
static const char* desc_broker = "MQTT Broker URL";

// Static UUIDs for descriptors (CUD - Characteristic User Description, 0x2901)
static const ble_uuid16_t UUID_CUD = BLE_UUID16_INIT(0x2901);

// Descriptor access callbacks
static int desc_mac_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading MAC descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_mac, strlen(desc_mac));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int desc_pop_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading PoP descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_pop, strlen(desc_pop));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}
static int desc_ssid_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading SSID descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_ssid, strlen(desc_ssid));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int desc_pass_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading Password descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_pass, strlen(desc_pass));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int desc_status_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading Status descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_status, strlen(desc_status));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int desc_cmd_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading Command descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_cmd, strlen(desc_cmd));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int desc_ids_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading IDs descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_ids, strlen(desc_ids));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

static int desc_broker_access(uint16_t, uint16_t, struct ble_gatt_access_ctxt *ctxt, void*) {
    ESP_LOGI(TAG, "Reading Broker descriptor");
    int rc = os_mbuf_append(ctxt->om, desc_broker, strlen(desc_broker));
    return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
}

// Descriptor definitions - will be filled at runtime
static struct ble_gatt_dsc_def mac_dsc[2];
static struct ble_gatt_dsc_def pop_dsc[2];
static struct ble_gatt_dsc_def ssid_dsc[2];
static struct ble_gatt_dsc_def pass_dsc[2];
static struct ble_gatt_dsc_def status_dsc[2];
static struct ble_gatt_dsc_def cmd_dsc[2];
static struct ble_gatt_dsc_def ids_dsc[2];
static struct ble_gatt_dsc_def broker_dsc[2];

static int gatt_svr_init()
{
    memset(prov_chrs, 0, sizeof(prov_chrs));

    // Fill descriptor definitions at runtime
    memset(mac_dsc, 0, sizeof(mac_dsc));
    mac_dsc[0].uuid = &UUID_CUD.u;
    mac_dsc[0].att_flags = BLE_ATT_F_READ;
    mac_dsc[0].access_cb = desc_mac_access;
    mac_dsc[0].arg = nullptr;
    
    memset(pop_dsc, 0, sizeof(pop_dsc));
    pop_dsc[0].uuid = &UUID_CUD.u;
    pop_dsc[0].att_flags = BLE_ATT_F_READ;
    pop_dsc[0].access_cb = desc_pop_access;
    pop_dsc[0].arg = nullptr;
    
    memset(ssid_dsc, 0, sizeof(ssid_dsc));
    ssid_dsc[0].uuid = &UUID_CUD.u;
    ssid_dsc[0].att_flags = BLE_ATT_F_READ;
    ssid_dsc[0].access_cb = desc_ssid_access;
    ssid_dsc[0].arg = nullptr;
    // ssid_dsc[1] is terminator (already zeroed)

    memset(pass_dsc, 0, sizeof(pass_dsc));
    pass_dsc[0].uuid = &UUID_CUD.u;
    pass_dsc[0].att_flags = BLE_ATT_F_READ;
    pass_dsc[0].access_cb = desc_pass_access;
    pass_dsc[0].arg = nullptr;

    memset(status_dsc, 0, sizeof(status_dsc));
    status_dsc[0].uuid = &UUID_CUD.u;
    status_dsc[0].att_flags = BLE_ATT_F_READ;
    status_dsc[0].access_cb = desc_status_access;
    status_dsc[0].arg = nullptr;

    memset(cmd_dsc, 0, sizeof(cmd_dsc));
    cmd_dsc[0].uuid = &UUID_CUD.u;
    cmd_dsc[0].att_flags = BLE_ATT_F_READ;
    cmd_dsc[0].access_cb = desc_cmd_access;
    cmd_dsc[0].arg = nullptr;

    memset(ids_dsc, 0, sizeof(ids_dsc));
    ids_dsc[0].uuid = &UUID_CUD.u;
    ids_dsc[0].att_flags = BLE_ATT_F_READ;
    ids_dsc[0].access_cb = desc_ids_access;
    ids_dsc[0].arg = nullptr;

    memset(broker_dsc, 0, sizeof(broker_dsc));
    broker_dsc[0].uuid = &UUID_CUD.u;
    broker_dsc[0].att_flags = BLE_ATT_F_READ;
    broker_dsc[0].access_cb = desc_broker_access;
    broker_dsc[0].arg = nullptr;

    // Characteristic 0: MAC Address (READ only)
    prov_chrs[0].uuid = &UUID_CHR_MAC.u;
    prov_chrs[0].access_cb = gatt_chr_access_cb;
    prov_chrs[0].flags = BLE_GATT_CHR_F_READ;
    prov_chrs[0].descriptors = mac_dsc;

    // Characteristic 1: Proof of Possession (READ only)
    prov_chrs[1].uuid = &UUID_CHR_POP.u;
    prov_chrs[1].access_cb = gatt_chr_access_cb;
    prov_chrs[1].flags = BLE_GATT_CHR_F_READ;
    prov_chrs[1].descriptors = pop_dsc;

    // Characteristic 2: SSID (READ/WRITE)
    prov_chrs[2].uuid = &UUID_CHR_SSID.u;
    prov_chrs[2].access_cb = gatt_chr_access_cb;
    prov_chrs[2].flags = BLE_GATT_CHR_F_WRITE | BLE_GATT_CHR_F_READ;
    prov_chrs[2].descriptors = ssid_dsc;

    // Characteristic 3: Password (WRITE only)
    prov_chrs[3].uuid = &UUID_CHR_PASS.u;
    prov_chrs[3].access_cb = gatt_chr_access_cb;
    prov_chrs[3].flags = BLE_GATT_CHR_F_WRITE;
    prov_chrs[3].descriptors = pass_dsc;

    // Characteristic 4: Status (READ/NOTIFY)
    prov_chrs[4].uuid = &UUID_CHR_STATUS.u;
    prov_chrs[4].access_cb = gatt_chr_access_cb;
    prov_chrs[4].flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY;
    prov_chrs[4].val_handle = &s_status_val_handle; // store val handle for notify
    prov_chrs[4].descriptors = status_dsc;

    // Characteristic 5: Command (WRITE only)
    prov_chrs[5].uuid = &UUID_CHR_CMD.u;
    prov_chrs[5].access_cb = gatt_chr_access_cb;
    prov_chrs[5].flags = BLE_GATT_CHR_F_WRITE;
    prov_chrs[5].descriptors = cmd_dsc;
    
    // Characteristic 6: IDs (WRITE only)
    prov_chrs[6].uuid = &UUID_CHR_IDS.u;
    prov_chrs[6].access_cb = gatt_chr_access_cb;
    prov_chrs[6].flags = BLE_GATT_CHR_F_WRITE;
    prov_chrs[6].descriptors = ids_dsc;

    // Characteristic 7: Broker URL (WRITE only)
    prov_chrs[7].uuid = &UUID_CHR_BROKER_URL.u;
    prov_chrs[7].access_cb = gatt_chr_access_cb;
    prov_chrs[7].flags = BLE_GATT_CHR_F_WRITE;
    prov_chrs[7].descriptors = broker_dsc;

    // Characteristic 8: Terminator (already zeroed)


    memset(gatt_svcs, 0, sizeof(gatt_svcs));
    gatt_svcs[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
    gatt_svcs[0].uuid = &UUID_PROV_SVC.u;
    gatt_svcs[0].characteristics = prov_chrs;

    int rc = ble_gatts_count_cfg(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_count_cfg failed: %d", rc);
        return rc;
    }

    rc = ble_gatts_add_svcs(gatt_svcs);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gatts_add_svcs failed: %d", rc);
        return rc;
    }

    return 0;
}

static void start_advertising()
{
    struct ble_hs_adv_fields fields;
    struct ble_hs_adv_fields scan_rsp_fields;
    int rc;

    // Stop advertising if it's already active
    if (ble_gap_adv_active()) {
        ESP_LOGW(TAG, "Advertising is already active, stopping it first.");
        rc = ble_gap_adv_stop();
        if (rc != 0) {
            ESP_LOGE(TAG, "Failed to stop advertising; rc=%d", rc);
            // We can try to continue, but it might fail
        }
    }

    /**
     *  Set the advertisement data included in advertisement packets:
     *  - Flags
     *  - Advertising tx power
     *  - Device name
     */
    memset(&fields, 0, sizeof(fields));

    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;

    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting advertisement data; rc=%d", rc);
        return;
    }

    /**
     *  Set the scan response data included in scan response packets:
     *  - Complete 128-bit service UUID
     */
    memset(&scan_rsp_fields, 0, sizeof(scan_rsp_fields));
    scan_rsp_fields.uuids128 = &UUID_PROV_SVC;
    scan_rsp_fields.num_uuids128 = 1;
    scan_rsp_fields.uuids128_is_complete = 1;

    rc = ble_gap_adv_rsp_set_fields(&scan_rsp_fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "Error setting scan response data; rc=%d", rc);
        return;
    }

    /* Begin advertising */
    struct ble_gap_adv_params adv_params;
    memset(&adv_params, 0, sizeof(adv_params));
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;
    
    rc = ble_gap_adv_start(s_own_addr_type, nullptr, BLE_HS_FOREVER,
                           &adv_params, gap_event, nullptr);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_start failed: %d", rc);
        return;
    }
    
    ESP_LOGI(TAG, "BLE advertising as '%s' started", DEVICE_NAME);
}

static int gap_event(struct ble_gap_event *event, void *arg)
{
    (void)arg;
    
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        if (event->connect.status == 0) {
            ESP_LOGI(TAG, "Client connected, handle=%d", event->connect.conn_handle);
            s_conn_handle = event->connect.conn_handle;
        } else {
            ESP_LOGW(TAG, "Connect failed status=%d, restarting adv", event->connect.status);
            start_advertising();
        }
        return 0;
        
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Client disconnected reason=%d", event->disconnect.reason);
        s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
        
        // Jeśli konfiguracja została zapisana, możemy zakończyć provisioning
        if (s_status == STATUS_SAVED) {
            ESP_LOGI(TAG, "Config saved, stopping provisioning...");
            s_provisioning_active = false;
        } else {
            // W przeciwnym razie kontynuuj advertising
            start_advertising();
        }
        return 0;
        
    case BLE_GAP_EVENT_ADV_COMPLETE:
        ESP_LOGI(TAG, "Advertising complete; restarting");
        if (s_provisioning_active) {
            start_advertising();
        }
        return 0;
        
    default:
        return 0;
    }
}

void ble_provisioning_init_services() {
    // Rejestracja naszego provisioning service
    int rc = gatt_svr_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to init GATT server: %d", rc);
    }
}


void ble_provisioning_start(LEDController* led)
{
    if (s_provisioning_active) {
        ESP_LOGW(TAG, "BLE provisioning already active");
        return;
    }
    
    s_led = led;
    if (s_led) {
        s_led->start_blinking();
    }
    
    ESP_LOGI(TAG, "Starting BLE provisioning...");
    
    // Load saved config so user can read current SSID via BLE
    WifiConfig saved_config;
    if (wifi_config_load(saved_config)) {
        // Copy saved SSID to temp config (visible via READ)
        memcpy(&s_temp_config, &saved_config, sizeof(WifiConfig));
        ESP_LOGI(TAG, "Loaded saved SSID: '%s' (readable via BLE)", s_temp_config.ssid);
    } else {
        // No saved config - start empty
        memset(&s_temp_config, 0, sizeof(s_temp_config));
        ESP_LOGI(TAG, "No saved config - starting with empty SSID");
    }
    
    s_status = STATUS_READY;
    s_provisioning_active = true;
    
    // Inicjalizacja NimBLE usunieta - teraz w main.cpp
    
    // Pobierz typ adresu własnego (potrzebne do advertising)
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: %d", rc);
        s_provisioning_active = false;
        return;
    }

    // Start advertising
    start_advertising();
    
    // Create and start provisioning timeout timer
    s_provisioning_timer = xTimerCreate(
        "prov_timer",
        pdMS_TO_TICKS(PROVISIONING_TIMEOUT_MS),
        pdFALSE, // one-shot
        nullptr,
        provisioning_timeout_cb
    );
    
    if (s_provisioning_timer != nullptr) {
        xTimerStart(s_provisioning_timer, 0);
        ESP_LOGI(TAG, "Provisioning timeout set to %d seconds", PROVISIONING_TIMEOUT_MS / 1000);
    } else {
        ESP_LOGW(TAG, "Failed to create provisioning timer");
    }
    
    ESP_LOGI(TAG, "BLE provisioning started successfully");
}

void ble_provisioning_stop()
{
    if (!s_provisioning_active) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping BLE provisioning...");
    
    // Stop and delete timer
    if (s_provisioning_timer != nullptr) {
        xTimerStop(s_provisioning_timer, 0);
        xTimerDelete(s_provisioning_timer, 0);
        s_provisioning_timer = nullptr;
    }
    
    // Rozłącz klienta jeśli jest podłączony
    if (s_conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ble_gap_terminate(s_conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    
    // Stop advertising
    ble_gap_adv_stop();
    
    s_provisioning_active = false;
    
    if (s_led) {
        s_led->stop_blinking();
    }
    
    ESP_LOGI(TAG, "BLE provisioning stopped");
}

bool ble_provisioning_is_active()
{
    return s_provisioning_active;
}
