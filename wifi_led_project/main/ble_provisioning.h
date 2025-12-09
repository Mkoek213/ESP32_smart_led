#ifndef BLE_PROVISIONING_H
#define BLE_PROVISIONING_H

#include "led_controller.h"

// Inicjalizacja i start BLE provisioning
void ble_provisioning_start(LEDController* led);

// Stop BLE provisioning
void ble_provisioning_stop();

// Sprawdzenie czy BLE provisioning jest aktywny
bool ble_provisioning_is_active();

#endif // BLE_PROVISIONING_H
