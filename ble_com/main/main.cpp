#include <cstdint>
#include <cstddef>
#include <cstring>
#include <cstdio>
#include <strings.h>

extern "C" {
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

namespace {

constexpr const char *TAG = "ble_gatt_client";

// Cel, z którym się łączymy
constexpr const char kTargetName_Sensor[] = "ATC_8E4B89";

// Definicje standardowych serwisów i charakterystyk
constexpr const uint16_t kServiceUUIDEnv = 0x181A;
constexpr const uint16_t kCharUUIDTemp = 0x2A6E;
constexpr const uint16_t kCharUUIDHumidity = 0x2A6F;

constexpr const uint16_t kServiceUUIDBattery = 0x180F;
constexpr const uint16_t kCharUUIDBattery = 0x2A19;

// Etykiety do rozróżnienia odczytów w callbacku
constexpr const char *kTempLabel = "Temperature";
constexpr const char *kHumidityLabel = "Humidity";
constexpr const char *kBatteryLabel = "Battery";

// Globalny kontekst przechowujący uchwyty (handles)
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
};

TargetContext g_ctx;

void reset_context() {
    g_ctx = TargetContext{};
}

void start_scan();
int characteristic_disc_cb(uint16_t, const struct ble_gatt_error *, const struct ble_gatt_chr *, void *);
int service_disc_cb(uint16_t, const struct ble_gatt_error *, const struct ble_gatt_svc *, void *);

/**
 * @brief Krok 5: Wywoływana po odczytaniu wartości. Odczytuje dane i uruchamia kolejny odczyt.
 */
int value_read_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                  struct ble_gatt_attr *attr, void *arg) {
    const char *label = static_cast<const char *>(arg);

    if (error->status == 0) {
        uint8_t buffer[8] = {0};
        int len = OS_MBUF_PKTLEN(attr->om);
        if (len > static_cast<int>(sizeof(buffer))) {
            len = sizeof(buffer);
        }
        os_mbuf_copydata(attr->om, 0, len, buffer);

        // Parsowanie danych na podstawie etykiety
        if (arg == kTempLabel && len >= 2) {
            const int16_t raw = (int16_t)(buffer[0] | (buffer[1] << 8));
            const float value = (float)raw / 100.0f;
            ESP_LOGI(TAG, "Odczyt -> Temperatura: %.2f *C", value);
        } else if (arg == kHumidityLabel && len >= 2) {
            const uint16_t raw = (uint16_t)(buffer[0] | (buffer[1] << 8));
            const float value = (float)raw / 100.0f;
            ESP_LOGI(TAG, "Odczyt -> Wilgotność: %.2f %%", value);
        } else if (arg == kBatteryLabel && len >= 1) {
            const uint8_t raw = buffer[0];
            ESP_LOGI(TAG, "Odczyt -> Bateria: %u %%", raw);
        }
    }

    // === Logika łańcucha odczytów ===
    // Po odczycie temperatury, żądaj wilgotności
    if (arg == kTempLabel && g_ctx.humidity_handle) {
         ble_gattc_read(g_ctx.conn_handle, g_ctx.humidity_handle,
                                value_read_cb, (void*)kHumidityLabel);
    // Po odczycie wilgotności, żądaj baterii
    } else if (arg == kHumidityLabel && g_ctx.battery_handle) {
         ble_gattc_read(g_ctx.conn_handle, g_ctx.battery_handle,
                                value_read_cb, (void*)kBatteryLabel);
    // Po odczycie baterii (ostatni odczyt), zakończ połączenie
    } else if (arg == kBatteryLabel) {
        ESP_LOGI(TAG, "Odczyt zakończony. Rozłączam.");
        ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
    return 0;
}

/**
 * @brief Krok 4: Rozpoczyna łańcuch odczytów wartości.
 */
void request_measurements() {
    if (g_ctx.conn_handle == BLE_HS_CONN_HANDLE_NONE) {
        return;
    }

    // Rozpocznij łańcuch odczytów od temperatury (jeśli ją znaleziono)
    if (g_ctx.temp_handle) {
        ble_gattc_read(g_ctx.conn_handle, g_ctx.temp_handle,
                                value_read_cb, (void*)kTempLabel);
    } 
    // Jeśli nie było temp, zacznij od wilgotności
    else if (g_ctx.humidity_handle) {
         ble_gattc_read(g_ctx.conn_handle, g_ctx.humidity_handle,
                                value_read_cb, (void*)kHumidityLabel);
    } 
    // A jeśli nie, zacznij od baterii
    else if (g_ctx.battery_handle) {
        ble_gattc_read(g_ctx.conn_handle, g_ctx.battery_handle,
                                value_read_cb, (void*)kBatteryLabel);
    } else {
        // Nie znaleziono nic, rozłącz się
        ble_gap_terminate(g_ctx.conn_handle, BLE_ERR_REM_USER_CONN_TERM);
    }
}

/**
 * @brief Krok 3: Wywoływana po odkryciu charakterystyki.
 */
int characteristic_disc_cb(uint16_t conn_handle,
                           const struct ble_gatt_error *error,
                           const struct ble_gatt_chr *chr, void *arg) {
    
    const uint16_t service_uuid = arg ? *(const uint16_t*)arg : 0;

    // Zapisz uchwyty (handles) dla znalezionych charakterystyk
    if (error->status == 0 && chr != nullptr) {
        const uint16_t uuid16 = ble_uuid_u16(&chr->uuid.u);
        
        if (service_uuid == kServiceUUIDEnv) {
            if (uuid16 == kCharUUIDTemp) {
                g_ctx.temp_handle = chr->val_handle;
                ESP_LOGI(TAG, "  Znaleziono charakterystykę Temperatury (handle: %u)", g_ctx.temp_handle);
            } else if (uuid16 == kCharUUIDHumidity) {
                g_ctx.humidity_handle = chr->val_handle;
                ESP_LOGI(TAG, "  Znaleziono charakterystykę Wilgotności (handle: %u)", g_ctx.humidity_handle);
            }
        } else if (service_uuid == kServiceUUIDBattery) {
            if (uuid16 == kCharUUIDBattery) {
                g_ctx.battery_handle = chr->val_handle;
                ESP_LOGI(TAG, "  Znaleziono charakterystykę Baterii (handle: %u)", g_ctx.battery_handle);
            }
        }
        return 0;
    }

    // Zakończono odkrywanie charakterystyk dla danego serwisu
    if (error->status == BLE_HS_EDONE) {
        // Jeśli właśnie skończyliśmy serwis Env I znaleźliśmy też serwis Batt
        if (service_uuid == kServiceUUIDEnv && g_ctx.batt_start_handle != 0) {
            // Uruchom odkrywanie dla serwisu Baterii
            ble_gattc_disc_all_chrs(conn_handle, g_ctx.batt_start_handle,
                                                   g_ctx.batt_end_handle, characteristic_disc_cb,
                                                   (void*)&kServiceUUIDBattery);
        } else {
            // W przeciwnym razie, skończyliśmy odkrywanie wszystkiego
            ESP_LOGI(TAG, "Odkrywanie zakończone. Żądam odczytu wartości...");
            request_measurements();
        }
        return 0;
    }
    return 0;
}

/**
 * @brief Krok 2: Wywoływana po odkryciu serwisu.
 */
int service_disc_cb(uint16_t conn_handle, const struct ble_gatt_error *error,
                    const struct ble_gatt_svc *svc, void *) {
    
    // Zapisz uchwyty (handles) dla znalezionych serwisów
    if (error->status == 0 && svc != nullptr) {
        const uint16_t uuid16 = ble_uuid_u16(&svc->uuid.u);
        if (uuid16 == kServiceUUIDEnv) {
            g_ctx.env_start_handle = svc->start_handle;
            g_ctx.env_end_handle = svc->end_handle;
            ESP_LOGI(TAG, "Znaleziono Environmental Sensing (handle: %u..%u)",
                     g_ctx.env_start_handle, g_ctx.env_end_handle);
        } else if (uuid16 == kServiceUUIDBattery) {
            g_ctx.batt_start_handle = svc->start_handle;
            g_ctx.batt_end_handle = svc->end_handle;
            ESP_LOGI(TAG, "Znaleziono Battery Service (handle: %u..%u)",
                     g_ctx.batt_start_handle, g_ctx.batt_end_handle);
        }
        return 0;
    }

    // Zakończono odkrywanie serwisów
    if (error->status == BLE_HS_EDONE) {
        // Rozpocznij odkrywanie charakterystyk, zaczynając od serwisu Env (jeśli istnieje)
        if (g_ctx.env_start_handle != 0) {
            ble_gattc_disc_all_chrs(conn_handle, g_ctx.env_start_handle,
                                                   g_ctx.env_end_handle, characteristic_disc_cb,
                                                   (void*)&kServiceUUIDEnv);
        }
        // Jeśli nie było Env, ale było Batt
        else if (g_ctx.batt_start_handle != 0) {
            ble_gattc_disc_all_chrs(conn_handle, g_ctx.batt_start_handle,
                                                   g_ctx.batt_end_handle, characteristic_disc_cb,
                                                   (void*)&kServiceUUIDBattery);
        } else {
            // Nie znaleziono żadnego z interesujących nas serwisów
            ble_gap_terminate(conn_handle, BLE_ERR_REM_USER_CONN_TERM);
        }
        return 0;
    }
    return 0; 
}

/**
 * @brief Krok 1: Główna funkcja obsługi zdarzeń GAP (skanowanie, łączenie).
 */
int gap_event(struct ble_gap_event *event, void *) {
    switch (event->type) {
    
    // Odkryto urządzenie
    case BLE_GAP_EVENT_DISC: {
        // Ignoruj, jeśli już się łączymy lub jesteśmy połączeni
        if (g_ctx.connecting || g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            return 0;
        }

        struct ble_hs_adv_fields fields;
        if (ble_hs_adv_parse_fields(&fields, event->disc.data,
                                    event->disc.length_data) != 0) {
            return 0; // Błąd parsowania
        }

        // Sprawdź, czy nazwa pasuje do naszego celu
        if (fields.name == nullptr || 
            strlen(kTargetName_Sensor) != fields.name_len ||
            memcmp(fields.name, kTargetName_Sensor, fields.name_len) != 0) {
            return 0; // To nie to urządzenie
        }

        ESP_LOGI(TAG, "Znaleziono cel!");

        // Zatrzymujemy skanowanie i próbujemy się połączyć
        ble_gap_disc_cancel();
        
        uint8_t own_addr_type;
        ble_hs_id_infer_auto(0, &own_addr_type);
        
        ble_gap_connect(own_addr_type, &event->disc.addr,
                             30000, nullptr, gap_event, nullptr);
        
        g_ctx.connecting = true;
        ESP_LOGI(TAG, "Łączenie...");
        return 0;
    }

    // Połączono
    case BLE_GAP_EVENT_CONNECT:
        g_ctx.connecting = false;
        if (event->connect.status == 0) {
            g_ctx.conn_handle = event->connect.conn_handle;
            ESP_LOGI(TAG, "Połączono, handle %u. Rozpoczynam odkrywanie serwisów...", g_ctx.conn_handle);

            // Resetujemy uchwyty przed odkrywaniem
            reset_context();
            g_ctx.conn_handle = event->connect.conn_handle; 

            // Krok 2: Rozpocznij odkrywanie wszystkich serwisów
            ble_gattc_disc_all_svcs(g_ctx.conn_handle, service_disc_cb, nullptr);
        } else {
            // Połączenie nieudane, wznów skanowanie
            start_scan();
        }
        return 0;

    // Rozłączono
    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "Rozłączono (powód %d). Restartuję skanowanie.", event->disconnect.reason);
        reset_context();
        start_scan(); // Uruchom skanowanie od nowa
        return 0;

    // Skanowanie zakończone (timeout)
    case BLE_GAP_EVENT_DISC_COMPLETE:
        // Jeśli skanowanie się zakończyło i nie znaleźliśmy celu
        if (!g_ctx.connecting) {
            start_scan();
        }
        return 0;

    default:
        return 0;
    }
}

/**
 * @brief Krok 0: Rozpoczyna skanowanie.
 */
void start_scan() {
    if (g_ctx.connecting || g_ctx.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        return;
    }

    uint8_t own_addr_type;
    ble_hs_id_infer_auto(0, &own_addr_type);

    struct ble_gap_disc_params params;
    memset(&params, 0, sizeof(params));
    params.filter_duplicates = 1;
    params.passive = 0; // Skanowanie AKTYWNE (potrzebne do odczytania nazwy)

    ble_gap_disc(own_addr_type, BLE_HS_FOREVER, &params,
                      gap_event, nullptr);
    
    ESP_LOGI(TAG, "Skanowanie...");
}

// === Funkcje startowe NimBLE ===

void on_reset(int reason) {
    ESP_LOGE(TAG, "Resetting, reason=%d", reason);
    reset_context();
}

void on_sync() {
    // Stos BLE jest gotowy, uruchom skanowanie
    start_scan();
}

void host_task(void *param) {
    nimble_port_run(); // Główna pętla NimBLE
    nimble_port_freertos_deinit();
}

} 

// === Główna funkcja programu ===

extern "C" void app_main(void) {
    // Inicjalizacja NVS (pamięć flash)
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }
    ESP_ERROR_CHECK(ret);

    // Inicjalizacja NimBLE
    ESP_ERROR_CHECK(nimble_port_init());

    // Ustawienie callbacków dla resetu i synchronizacji
    ble_hs_cfg.reset_cb = on_reset;
    ble_hs_cfg.sync_cb = on_sync;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    // Inicjalizacja serwisów GAP i GATT
    ble_svc_gap_init();
    ble_svc_gatt_init();
    ble_svc_gap_device_name_set("esp32-gatt-client");

    ble_store_config_init();

    // Uruchomienie głównego wątku NimBLE
    nimble_port_freertos_init(host_task);
}