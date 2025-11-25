#include "ble_provisioning.h"
#include "wifi_config.h"
#include "esp_log.h"
#include <cstring>
#include "wifi_station.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

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

// uuid dla provisioning service
static ble_uuid128_t UUID_PROV_SVC = BLE_UUID128_INIT(
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0,
    0x12, 0x34, 0x56, 0x78, 0x9a, 0xbc, 0xde, 0xf0
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

static LEDController* s_led = nullptr;
static uint8_t s_own_addr_type = 0;
static bool s_provisioning_active = false;
static WifiConfig s_temp_config = {};
static uint16_t s_conn_handle = BLE_HS_CONN_HANDLE_NONE;
static uint16_t s_status_val_handle = 0;

// wartosci statusu zwracane przez płytke
enum ProvStatus : uint8_t {
    STATUS_READY = 0x00,
    STATUS_SSID_SET = 0x01,
    STATUS_PASS_SET = 0x02,
    STATUS_SAVED = 0x03,
    STATUS_ERROR = 0xFF
};

static uint8_t s_status = STATUS_READY;

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
    
    // ustawianie SSID
    if (ble_uuid_cmp(uuid, &UUID_CHR_SSID.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
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
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    // ustawianie PASSWORD
    if (ble_uuid_cmp(uuid, &UUID_CHR_PASS.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
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
    // komendy: 0x01 = SAVE, 0x02 = RESET, czyli wpisujesz HEX 01 albo 02 w zależności od komendy
    if (ble_uuid_cmp(uuid, &UUID_CHR_CMD.u) == 0) {
        if (ctxt->op == BLE_GATT_ACCESS_OP_WRITE_CHR) {
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
            else if (cmd == 0x02) { // komenda na reset
                wifi_config_erase();
                memset(&s_temp_config, 0, sizeof(s_temp_config));
                s_status = STATUS_READY;
                notify_status_if_possible();
                ESP_LOGI(TAG, "WiFi config erased");
                return 0;
            }
            
            return BLE_ATT_ERR_UNLIKELY;
        }
        return BLE_ATT_ERR_UNLIKELY;
    }
    
    return BLE_ATT_ERR_UNLIKELY;
}

static struct ble_gatt_chr_def prov_chrs[5];
static struct ble_gatt_svc_def gatt_svcs[2];

static int gatt_svr_init()
{
    memset(prov_chrs, 0, sizeof(prov_chrs));

    prov_chrs[0].uuid = &UUID_CHR_SSID.u;
    prov_chrs[0].access_cb = gatt_chr_access_cb;
    prov_chrs[0].flags = BLE_GATT_CHR_F_WRITE;

    prov_chrs[1].uuid = &UUID_CHR_PASS.u;
    prov_chrs[1].access_cb = gatt_chr_access_cb;
    prov_chrs[1].flags = BLE_GATT_CHR_F_WRITE;

    prov_chrs[2].uuid = &UUID_CHR_STATUS.u;
    prov_chrs[2].access_cb = gatt_chr_access_cb;
    prov_chrs[2].flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_NOTIFY;
    prov_chrs[2].val_handle = &s_status_val_handle; // store val handle for notify

    prov_chrs[3].uuid = &UUID_CHR_CMD.u;
    prov_chrs[3].access_cb = gatt_chr_access_cb;
    prov_chrs[3].flags = BLE_GATT_CHR_F_WRITE;


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
    memset(&fields, 0, sizeof(fields));
    
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.tx_pwr_lvl_is_present = 1;
    fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
    fields.name = (uint8_t *)DEVICE_NAME;
    fields.name_len = strlen(DEVICE_NAME);
    fields.name_is_complete = 1;
    
    int rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_gap_adv_set_fields failed: %d", rc);
        return;
    }
    
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

static void on_reset(int reason)
{
    ESP_LOGE(TAG, "BLE stack resetting; reason=%d", reason);
}

static void on_sync()
{
    int rc = ble_hs_id_infer_auto(0, &s_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: %d", rc);
        return;
    }
    
    uint8_t addr_val[6] = {0};
    ble_hs_id_copy_addr(s_own_addr_type, addr_val, nullptr);
    ESP_LOGI(TAG, "BLE addr: %02X:%02X:%02X:%02X:%02X:%02X",
             addr_val[5], addr_val[4], addr_val[3],
             addr_val[2], addr_val[1], addr_val[0]);
    
    start_advertising();
}

static void host_task(void *param)
{
    (void)param;
    ESP_LOGI(TAG, "BLE host task started");
    nimble_port_run();
    nimble_port_freertos_deinit();
}

extern "C" void ble_store_config_init(void) {
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
    
    // Reset stanu
    memset(&s_temp_config, 0, sizeof(s_temp_config));
    s_status = STATUS_READY;
    s_provisioning_active = true;
    
    // Inicjalizacja NimBLE
    ESP_ERROR_CHECK(nimble_port_init());
    
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;
    
    // Rejestracja wbudowanych serwisów GAP i GATT
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(DEVICE_NAME);
    
    // Rejestracja naszego provisioning service
    int rc = gatt_svr_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "Failed to init GATT server: %d", rc);
        s_provisioning_active = false;
        return;
    }
    
    // Konfiguracja store (minimalna)
    ble_store_config_init();
    
    // Start BLE host task
    nimble_port_freertos_init(host_task);
    
    ESP_LOGI(TAG, "BLE provisioning started successfully");
}

void ble_provisioning_stop()
{
    if (!s_provisioning_active) {
        return;
    }
    
    ESP_LOGI(TAG, "Stopping BLE provisioning...");
    
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
    
    // Deinit NimBLE
    nimble_port_deinit();
    
    ESP_LOGI(TAG, "BLE provisioning stopped");
}

bool ble_provisioning_is_active()
{
    return s_provisioning_active;
}
