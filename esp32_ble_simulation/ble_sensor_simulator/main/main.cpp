
#include <cstdint>
#include <cstring>

extern "C"
{
#include "esp_log.h"
#include "nvs_flash.h"
#include "esp_random.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"
#include "store/config/ble_store_config.h"
}

extern "C" void ble_store_config_init(void)
{
}

namespace
{

    // do logow, nazwa naszego sensora
    constexpr const char *TAG = "ble_sensor_sim";

    // esp sie reklamuje pod ta nazwa, - nazwa taka sama jaka ma czujnik rzeczywisty nasz
    constexpr const char kDeviceName[] = "ATC_8E4B89";

    // kody servisow i charakterystyk - takich szuka klient i takie ma rzeczywisty czujnik tez
    static ble_uuid16_t UUID_SVC_ENV = BLE_UUID16_INIT(0x181A);
    static ble_uuid16_t UUID_CHR_TEMP = BLE_UUID16_INIT(0x2A6E);
    static ble_uuid16_t UUID_CHR_HUMI = BLE_UUID16_INIT(0x2A6F);
    static ble_uuid16_t UUID_SVC_BATT = BLE_UUID16_INIT(0x180F);
    static ble_uuid16_t UUID_CHR_BATT = BLE_UUID16_INIT(0x2A19);

    // adresy charatketystyk, NimBLE je wypelni
    static uint16_t g_temp_val_handle = 0;
    static uint16_t g_humi_val_handle = 0;
    static uint16_t g_batt_val_handle = 0;

    // tablica opisujaca jakie serwisy i charakterystuki udostapniamy - serwer GATT
    static ble_gatt_chr_def ENV_CHRS[3];
    static ble_gatt_chr_def BATT_CHRS[2];
    static ble_gatt_svc_def GATT_SVCS[3];

    // losowa temperatura (20.0 - 29.99 stopni)
    static int16_t gen_temp_raw()
    {

        uint32_t r = esp_random() % 1000;
        return static_cast<int16_t>(2000 + r);
    }

    // losowa wilgotnosc (40 - 59.99 procent)
    static uint16_t gen_humi_raw()
    {

        uint32_t r = esp_random() % 2000;
        return static_cast<uint16_t>(4000 + r);
    }

    // losowa bateria (50-100 procent)
    static uint8_t gen_batt_raw()
    {

        uint32_t r = esp_random() % 51;
        return static_cast<uint8_t>(50 + r);
    }

    // serwer odpowiada na odczyt, (kiedy klient wywoluje ble_gattc_read)
    static int gatt_chr_access_cb(uint16_t conn_handle,
                                  uint16_t attr_handle,
                                  ble_gatt_access_ctxt *ctxt,
                                  void *arg)
    {
        (void)conn_handle;
        (void)attr_handle;
        (void)arg;

        // ctxt->op to bufor wejsciowy, dane od klienta
        // jesli to nie odczyt charakterystyki
        if (ctxt->op != BLE_GATT_ACCESS_OP_READ_CHR)
        {
            return BLE_ATT_ERR_UNLIKELY;
        }

        // pole ktore klient czyta
        uint16_t uuid16 = ble_uuid_u16(ctxt->chr->uuid);

        // czyta temperature
        if (uuid16 == 0x2A6E)
        {
            int16_t raw = gen_temp_raw();
            uint8_t buf[2] = {
                static_cast<uint8_t>(raw & 0xFF),
                static_cast<uint8_t>((raw >> 8) & 0xFF)};
            ESP_LOGI(TAG, "READ Temp -> raw=%d (%.2f C)", raw, raw / 100.0f);

            // ctxt->om to bufor wyjsciowy, wyslemy go do klienta, do niego dodajemy dane

            return os_mbuf_append(ctxt->om, buf, sizeof(buf)) == 0
                       ? 0
                       : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        // czyta wilgotnosc
        else if (uuid16 == 0x2A6F)
        {
            uint16_t raw = gen_humi_raw();
            uint8_t buf[2] = {
                static_cast<uint8_t>(raw & 0xFF),
                static_cast<uint8_t>((raw >> 8) & 0xFF)};
            ESP_LOGI(TAG, "READ Humi -> raw=%u (%.2f %%)", raw, raw / 100.0f);
            return os_mbuf_append(ctxt->om, buf, sizeof(buf)) == 0
                       ? 0
                       : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        // czyta baterie
        else if (uuid16 == 0x2A19)
        {
            uint8_t raw = gen_batt_raw();
            ESP_LOGI(TAG, "READ Batt -> raw=%u %%", raw);
            return os_mbuf_append(ctxt->om, &raw, sizeof(raw)) == 0
                       ? 0
                       : BLE_ATT_ERR_INSUFFICIENT_RES;
        }

        return BLE_ATT_ERR_UNLIKELY;
    }

    // opisujemy co sensor potrafi
    static void gatt_svr_build_tables()
    {
        memset(ENV_CHRS, 0, sizeof(ENV_CHRS));
        memset(BATT_CHRS, 0, sizeof(BATT_CHRS));
        memset(GATT_SVCS, 0, sizeof(GATT_SVCS));

        // jakie sa charakterystyki env:

        // temperatura
        ENV_CHRS[0].uuid = &UUID_CHR_TEMP.u;
        ENV_CHRS[0].access_cb = gatt_chr_access_cb;
        ENV_CHRS[0].val_handle = &g_temp_val_handle;
        ENV_CHRS[0].flags = BLE_GATT_CHR_F_READ;

        // wilgotnosc
        ENV_CHRS[1].uuid = &UUID_CHR_HUMI.u;
        ENV_CHRS[1].access_cb = gatt_chr_access_cb;
        ENV_CHRS[1].val_handle = &g_humi_val_handle;
        ENV_CHRS[1].flags = BLE_GATT_CHR_F_READ;

        //  charakterystyka bateri
        BATT_CHRS[0].uuid = &UUID_CHR_BATT.u;
        BATT_CHRS[0].access_cb = gatt_chr_access_cb;
        BATT_CHRS[0].val_handle = &g_batt_val_handle;
        BATT_CHRS[0].flags = BLE_GATT_CHR_F_READ;

        // servis env sensing lda gatt
        // takie charakterystyki env
        GATT_SVCS[0].type = BLE_GATT_SVC_TYPE_PRIMARY;
        GATT_SVCS[0].uuid = &UUID_SVC_ENV.u;
        GATT_SVCS[0].characteristics = ENV_CHRS;

        // takie charakterystyki battery
        GATT_SVCS[1].type = BLE_GATT_SVC_TYPE_PRIMARY;
        GATT_SVCS[1].uuid = &UUID_SVC_BATT.u;
        GATT_SVCS[1].characteristics = BATT_CHRS;
    }

    // rejestracja charakterystyk w stosie nimblee
    static int gatt_svr_init_cpp()
    {
        gatt_svr_build_tables();

        int rc = ble_gatts_count_cfg(GATT_SVCS);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "ble_gatts_count_cfg failed: %d", rc);
            return rc;
        }
        rc = ble_gatts_add_svcs(GATT_SVCS);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "ble_gatts_add_svcs failed: %d", rc);
            return rc;
        }
        return 0;
    }

    static uint8_t g_own_addr_type = 0;

    // callback kotry dostaje info o polaczeniu/rozlaczeniu
    static int gap_event(struct ble_gap_event *event, void *arg);

    // rozglaszamy w poblizu naszego esp jako ATC_8....
    static void start_advertising()
    {
        ble_hs_adv_fields fields;
        memset(&fields, 0, sizeof(fields));
        fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
        fields.tx_pwr_lvl_is_present = 1;
        fields.tx_pwr_lvl = BLE_HS_ADV_TX_PWR_LVL_AUTO;
        fields.name = (uint8_t *)kDeviceName;
        fields.name_len = strlen(kDeviceName);
        fields.name_is_complete = 1;

        int rc = ble_gap_adv_set_fields(&fields);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "ble_gap_adv_set_fields failed: %d", rc);
            return;
        }

        ble_gap_adv_params adv_params;
        memset(&adv_params, 0, sizeof(adv_params));
        adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
        adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

        rc = ble_gap_adv_start(g_own_addr_type, nullptr, BLE_HS_FOREVER,
                               &adv_params, gap_event, nullptr);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "ble_gap_adv_start failed: %d", rc);
            return;
        }
        ESP_LOGI(TAG, "Advertising as '%s' started", kDeviceName);
    }

    // callback kotry dostaje info o polaczeniu/rozlaczeniu
    static int gap_event(struct ble_gap_event *event, void *arg)
    {
        (void)arg;
        switch (event->type)
        {
        case BLE_GAP_EVENT_CONNECT:
            if (event->connect.status == 0)
            {
                ESP_LOGI(TAG, "Client connected, handle=%d",
                         event->connect.conn_handle);
            }
            else
            {
                ESP_LOGW(TAG, "Connect failed status=%d, restarting adv",
                         event->connect.status);
                start_advertising();
            }
            return 0;

        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Client disconnected reason=%d",
                     event->disconnect.reason);
            start_advertising();
            return 0;

        case BLE_GAP_EVENT_ADV_COMPLETE:
            ESP_LOGI(TAG, "Advertising complete; restarting");
            start_advertising();
            return 0;

        default:
            return 0;
        }
    }

    static void on_reset(int reason)
    {
        ESP_LOGE(TAG, "Resetting; reason=%d", reason);
    }

    // gdy nimble jest gotowy to ustalamy typ adresu i startujemy rozglaszanie
    static void on_sync()
    {
        int rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
        if (rc != 0)
        {
            ESP_LOGE(TAG, "ble_hs_id_infer_auto failed: %d", rc);
            return;
        }
        uint8_t addr_val[6] = {0};
        ble_hs_id_copy_addr(g_own_addr_type, addr_val, nullptr);
        ESP_LOGI(TAG, "BLE addr: %02X:%02X:%02X:%02X:%02X:%02X",
                 addr_val[5], addr_val[4], addr_val[3],
                 addr_val[2], addr_val[1], addr_val[0]);
        start_advertising();
    }

    static void host_task(void *param)
    {
        (void)param;
        nimble_port_run();
        nimble_port_freertos_deinit();
    }

}

extern "C" void app_main(void)
{

    // przygotowanue pamieci nieulotnej NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES ||
        ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // inicjalizacja stosu BLE NimBLE i podpiecie callbackow
    ESP_ERROR_CHECK(nimble_port_init());
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // rejestracja wbudowanych serwisow GAT i usatwienie naszej nazwy
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set(kDeviceName);

    // rejestracja naxzych serwiscow
    ESP_ERROR_CHECK(gatt_svr_init_cpp());

    // konfiguracja pamieci na dane BLE - puste u nas
    ble_store_config_init();

    // startujemy hosta NimBLE jako task FreeRTOS
    nimble_port_freertos_init(host_task);
}
