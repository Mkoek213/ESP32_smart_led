#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <strings.h>

extern "C"
{
#include <esp_log.h>
#include <nvs_flash.h>

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"

#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"

#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "store/config/ble_store_config.h"
    void ble_store_config_init(void);
}

namespace
{

    constexpr const char *TAG = "ble_gatt_client"; // do logów, nazwa modułu

    constexpr const char kTargetName_Sensor[] = "ATC_8E4B89"; // nazwa szukanego urzadzenia

    // standardowe UUID dla sensora
    constexpr const uint16_t kServiceUUIDEnv = 0x181A;
    constexpr const uint16_t kCharUUIDTemp = 0x2A6E;
    constexpr const uint16_t kCharUUIDHumidity = 0x2A6F;

    constexpr const uint16_t kServiceUUIDBattery = 0x180F;
    constexpr const uint16_t kCharUUIDBattery = 0x2A19;

    constexpr const char *kTempLabel = "Temperature";
    constexpr const char *kHumidityLabel = "Humidity";
    constexpr const char *kBatteryLabel = "Battery";

    // struktura w ktorj trzymamy stan polaczenia z jednym urzedzeniem
    struct TargetContext
    {
        uint16_t conn_handle = BLE_HS_CONN_HANDLE_NONE; // identyfikator polaczenia w BLE
        bool connecting = false;

        uint16_t env_start_handle = 0;
        uint16_t env_end_handle = 0;
        uint16_t temp_handle = 0;
        uint16_t humidity_handle = 0;

        uint16_t batt_start_handle = 0;
        uint16_t batt_end_handle = 0;
        uint16_t battery_handle = 0;
    };

    TargetContext g_ctx;

    void reset_context()
    {
        g_ctx = TargetContext{};
    }

    void start_scan();
    int characteristic_disc_cb(uint16_t, const struct ble_gatt_error *, const struct ble_gatt_chr *, void *);
    int service_disc_cb(uint16_t, const struct ble_gatt_error *, const struct ble_gatt_svc *, void *);

    // 5. odczytujemy dane
    int value_read_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                      struct ble_gatt_attr *attr, void *arg)
    {
        const char *label = static_cast<const char *>(arg);

        // jesli odczyt sie udal, kopiujemy dane do bufora
        if (error->status == 0)
        {
            uint8_t buffer[8] = {0};
            int len = OS_MBUF_PKTLEN(attr->om);
            if (len > static_cast<int>(sizeof(buffer)))
            {
                len = sizeof(buffer);
            }
            os_mbuf_copydata(attr->om, 0, len, buffer);

            // odczyt dla temperatury
            if (arg == kTempLabel && len >= 2)
            {
                const int16_t raw = (int16_t)(buffer[0] | (buffer[1] << 8));
                const float value = (float)raw / 100.0f;
                ESP_LOGI(TAG, "Odczyt -> Temperatura: %.2f *C", value);
            }
            // odczyt dla wiglotnosci
            else if (arg == kHumidityLabel && len >= 2)
            {
                const uint16_t raw = (uint16_t)(buffer[0] | (buffer[1] << 8));
                const float value = (float)raw / 100.0f;
                ESP_LOGI(TAG, "Odczyt -> Wilgotność: %.2f %%", value);
            }
            // odczyt bateri
            else if (arg == kBatteryLabel && len >= 1)
            {
                const uint8_t raw = buffer[0];
                ESP_LOGI(TAG, "Odczyt -> Bateria: %u %%", raw);
            }
        }

        if (arg == kTempLabel && g_ctx.humidity_handle)
        {
            // po temperaturze czytamy wilgotnosc
            ble_gattc_read(g_ctx.conn_handle, g_ctx.humidity_handle,
                           value_read_cb, (void *)kHumidityLabel);
        }
        else if (arg == kHumidityLabel && g_ctx.battery_handle)
        {
            // po wilgotnosci czytamt baterie
            ble_gattc_read(g_ctx.conn_handle, g_ctx.battery_handle,
                           value_read_cb, (void *)kBatteryLabel);
        }
        else if (arg == kBatteryLabel)
        {
            // po baterii konczymy i rozlaczamy
            ESP_LOGI(TAG, "Odczyt zakończony. Rozłączam.");
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }

    // lancuch odczytow wartosci
    void request_measurements()
    {
        if (g_ctx.conn_handle == BLE_HS_CONN_HANDLE_NONE)
        {
            return;
        }

        // odczytujemy temperature
        if (g_ctx.temp_handle)
        {
            ble_gattc_read(g_ctx.conn_handle, g_ctx.temp_handle,
                           value_read_cb, (void *)kTempLabel);
        }

        // odczytujemy wilgotnosc
        else if (g_ctx.humidity_handle)
        {
            ble_gattc_read(g_ctx.conn_handle, g_ctx.humidity_handle,
                           value_read_cb, (void *)kHumidityLabel);
        }

        // odczytujemy baterie
        else if (g_ctx.battery_handle)
        {
            ble_gattc_read(g_ctx.conn_handle, g_ctx.battery_handle,
                           value_read_cb, (void *)kBatteryLabel);
        }
        else
        {
            // rozlacza
            ble_gap_terminate(g_ctx.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
    }

    // 3. po odkryciu charakterystyk
    int characteristic_disc_cb(uint16_t conn_handle,
                               const struct ble_gatt_error *error,
                               const struct ble_gatt_chr *chr, void *arg)
    {

        const uint16_t service_uuid = arg ? *(const uint16_t *)arg : 0;

        if (error->status == 0 && chr != nullptr)
        {
            // czytamy charatkterystyke
            const uint16_t uuid16 = ble_uuid_u16(&chr->uuid.u);

            // serwis env
            if (service_uuid == kServiceUUIDEnv)
            {
                // jesli to kod temperatury
                if (uuid16 == kCharUUIDTemp)
                {
                    g_ctx.temp_handle = chr->val_handle;
                    ESP_LOGI(TAG, "  Znaleziono charakterystykę Temperatury (handle: %u)", g_ctx.temp_handle);
                }
                // jesli to kod wilgotnosci
                else if (uuid16 == kCharUUIDHumidity)
                {
                    g_ctx.humidity_handle = chr->val_handle;
                    ESP_LOGI(TAG, "  Znaleziono charakterystykę Wilgotności (handle: %u)", g_ctx.humidity_handle);
                }
            }
            // servis battery
            else if (service_uuid == kServiceUUIDBattery)
            {
                if (uuid16 == kCharUUIDBattery)
                {
                    g_ctx.battery_handle = chr->val_handle;
                    ESP_LOGI(TAG, "  Znaleziono charakterystykę Baterii (handle: %u)", g_ctx.battery_handle);
                }
            }
            return 0;
        }

        if (error->status == BLE_HS_EDONE)
        {

            if (service_uuid == kServiceUUIDEnv && g_ctx.batt_start_handle != 0)
            {
                // sprawdz servis battery
                ble_gattc_disc_all_chrs(conn_handle, g_ctx.batt_start_handle,
                                        g_ctx.batt_end_handle, characteristic_disc_cb,
                                        (void *)&kServiceUUIDBattery);
            }
            else
            {

                ESP_LOGI(TAG, "Odkrywanie zakończone. Żądam odczytu wartości...");
                request_measurements();
            }
            return 0;
        }
        return 0;
    }

    // 2. odkrywanie jakie rzeczy ma czujnik
    int service_disc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                        const struct ble_gatt_svc *svc, void *)
    {

        if (error->status == 0 && svc != nullptr)
        {
            const uint16_t uuid16 = ble_uuid_u16(&svc->uuid.u);
            if (uuid16 == kServiceUUIDEnv)
            {
                // zakres env sensing
                g_ctx.env_start_handle = svc->start_handle;
                g_ctx.env_end_handle = svc->end_handle;
                ESP_LOGI(TAG, "Znaleziono Environmental Sensing (handle: %u..%u)",
                         g_ctx.env_start_handle, g_ctx.env_end_handle);
            }
            else if (uuid16 == kServiceUUIDBattery)
            {
                // zakres battery service
                g_ctx.batt_start_handle = svc->start_handle;
                g_ctx.batt_end_handle = svc->end_handle;
                ESP_LOGI(TAG, "Znaleziono Battery Service (handle: %u..%u)",
                         g_ctx.batt_start_handle, g_ctx.batt_end_handle);
            }
            return 0;
        }

        if (error->status == BLE_HS_EDONE)
        {
            // koniec servicow na urzadzeniu, sprawdzanie charakterystyk env
            if (g_ctx.env_start_handle != 0)
            {
                ble_gattc_disc_all_chrs(conn_handle, g_ctx.env_start_handle,
                                        g_ctx.env_end_handle, characteristic_disc_cb,
                                        (void *)&kServiceUUIDEnv);
            }

            // sprawdzanie charakterystyk battery
            else if (g_ctx.batt_start_handle != 0)
            {
                ble_gattc_disc_all_chrs(conn_handle, g_ctx.batt_start_handle,
                                        g_ctx.batt_end_handle, characteristic_disc_cb,
                                        (void *)&kServiceUUIDBattery);
            }
            else
            {
                // rozlacz
                ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
            }
            return 0;
        }
        return 0;
    }

    // 1.  glowna funckja obslugi zdarzen GAP - skanowanie, laczenie
    int gap_event(struct ble_gap_event *event, void *)
    {
        switch (event->type)
        {

        // gdy znajdziemy urzadzenie, parsujemy go i sprawdzamy nazwe
        case BLE_GAP_EVENT_DISC:
        {

            if (g_ctx.connecting || g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE)
            {
                return 0;
            }

            struct ble_hs_adv_fields fields;
            if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                        event->disc.length_data) != 0)
            {
                return 0;
            }

            // jesli to nie nasze urzadzenie, to olewamy
            if (fields.name == nullptr ||
                strlen(kTargetName_Sensor) != fields.name_len ||
                memcmp(fields.name, kTargetName_Sensor, fields.name_len) != 0)
            {
                return 0;
            }

            ESP_LOGI(TAG, "Znaleziono cel!");

            // zatrzymujemy skanowanie
            ble_gap_disc_cancel();

            uint8_t own_addr_type;
            ble_hs_id_infer_auto(0, &own_addr_type);

            ble_gap_connect(own_addr_type, &event->disc.addr,
                            30000, nullptr, gap_event, nullptr);

            g_ctx.connecting = true;
            ESP_LOGI(TAG, "Łączenie...");
            return 0;
        }

        // polaczenie sie udalo
        case BLE_GAP_EVENT_CONNECT:
            g_ctx.connecting = false;
            // polaczenie sie udalo = status 0
            if (event->connect.status == 0)
            {
                g_ctx.conn_handle = event->connect.conn_handle;
                ESP_LOGI(TAG, "Połączono, handle %u. Rozpoczynam odkrywanie serwisów...", g_ctx.conn_handle);

                reset_context();
                g_ctx.conn_handle = event->connect.conn_handle;

                // odkrywamy serwisy
                ble_gattc_disc_all_svcs(g_ctx.conn_handle, service_disc_cb, nullptr);
            }
            else
            {

                start_scan();
            }
            return 0;

        // jesli nastapi rozlaczenie, wracamy do skanowania
        case BLE_GAP_EVENT_DISCONNECT:
            ESP_LOGI(TAG, "Rozłączono (powód %d). Restartuję skanowanie.", event->disconnect.reason);
            reset_context();
            start_scan();
            return 0;

        case BLE_GAP_EVENT_DISC_COMPLETE:

            if (!g_ctx.connecting)
            {
                start_scan();
            }
            return 0;

        default:
            return 0;
        }
    }

    // skanowanie
    void start_scan()
    {
        if (g_ctx.connecting || g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE)
        {
            return;
        }

        uint8_t own_addr_type;
        ble_hs_id_infer_auto(0, &own_addr_type);

        struct ble_gap_disc_params params;
        memset(&params, 0, sizeof(params));
        params.filter_duplicates = 1;
        params.passive = 0; // aktywny scan - wysyla scan request

        // znalezione urzadzenia zglaszaj przez funkcje gap_event
        ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &params,
                     gap_event, nullptr);

        ESP_LOGI(TAG, "Skanowanie...");
    }

    void on_reset(int reason)
    {
        ESP_LOGE(TAG, "Resetting, reason=%d", reason);
        reset_context();
    }

    void on_sync()
    {

        start_scan();
    }

    void host_task(void *param)
    {
        nimble_port_run();
        nimble_port_freertos_deinit();
    }

}

extern "C" void app_main(void)
{

    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // start stosu BLE
    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    // nasza nazwa urzadzenia
    ble_svc_gap_device_name_set("esp32-gatt-client");

    ble_store_config_init();

    nimble_port_freertos_init(host_task);
}