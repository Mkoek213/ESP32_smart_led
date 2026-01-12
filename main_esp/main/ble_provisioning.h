#ifndef BLE_PROVISIONING_H
#define BLE_PROVISIONING_H

#include "led_controller.h"
#include <stdint.h>

// Inicjalizacja serwisów provisioning (wołane raz przy starcie)
void ble_provisioning_init_services();

// Start BLE provisioning (advertising)
void ble_provisioning_start(LEDController* led);

// Stop BLE provisioning
void ble_provisioning_stop();

// Sprawdzenie czy BLE provisioning jest aktywny
bool ble_provisioning_is_active();

// Get device MAC address (6 bytes)
void ble_provisioning_get_mac_address(uint8_t mac[6]);

// Get or generate Proof of Possession (32 bytes)
bool ble_provisioning_get_pop(uint8_t pop[32]);

// Factory reset - wipe all credentials and return to unclaimed state
void ble_provisioning_factory_reset();

#endif // BLE_PROVISIONING_H
