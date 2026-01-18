#ifndef WIFI_CONFIG_H
#define WIFI_CONFIG_H

#include <cstddef>
#include <cstdint>

// Maksymalne długości dla SSID i hasła
constexpr size_t WIFI_SSID_MAX_LEN = 32;
constexpr size_t WIFI_PASS_MAX_LEN = 64;
constexpr size_t MQTT_ID_MAX_LEN = 36; // Max length for UUIDs or other IDs
constexpr size_t BROKER_URL_MAX_LEN = 128;

struct WifiConfig {
    char ssid[WIFI_SSID_MAX_LEN + 1];
    char password[WIFI_PASS_MAX_LEN + 1];
    char customerId[MQTT_ID_MAX_LEN + 1];
    char locationId[MQTT_ID_MAX_LEN + 1];
    char deviceId[MQTT_ID_MAX_LEN + 1];
    char brokerUrl[BROKER_URL_MAX_LEN + 1];
};

// Inicjalizacja NVS dla wifi config
bool wifi_config_nvs_init();

// Wczytanie konfiguracji z NVS
bool wifi_config_load(WifiConfig& cfg);

// Zapis konfiguracji do NVS
bool wifi_config_save(const WifiConfig& cfg);

// Sprawdzenie czy jest zapisana konfiguracja
bool wifi_config_exists();

// Usunięcie konfiguracji z NVS (factory reset)
bool wifi_config_erase();

#endif // WIFI_CONFIG_H
