# ESP32 BLE Client & Sensor Simulator

## Overview

This project contains two independent ESP-IDF applications that communicate over BLE using the NimBLE stack: a GATT Central client (ble_client) and a GATT Peripheral server (ble_sensor_simulator). The client discovers and reads temperature, humidity, and battery characteristics from a device advertising as ATC_8E4B89, while the simulator emulates such a sensor by exposing standard Environmental Sensing and Battery services with random measurement values.

## Project structure

ble_client/ – NimBLE GATT Central reading data from a BLE sensor named ATC_8E4B89.

ble_sensor_simulator/ – NimBLE GATT Peripheral advertising as ATC_8E4B89 and serving random temperature, humidity, and battery values via standard services and characteristics.

Each directory is a standalone ESP-IDF project with its own CMakeLists.txt and main/ component containing main.cpp and idf_component.yml.

## BLE client (ble_client)

The client uses NimBLE as a GATT Central to perform active scanning and filter advertising packets by complete device name equal to ATC_8E4B89. Once the target is found, it cancels scanning and initiates a connection, logging messages like Skanowanie... and Znaleziono cel! followed by a GAP connect procedure.

After connecting, the client discovers all primary services and looks specifically for Environmental Sensing (0x181A) and Battery Service (0x180F) to record their handle ranges. It then discovers characteristics within those ranges and identifies Temperature (0x2A6E), Humidity (0x2A6F), and Battery Level (0x2A19) value handles.

The client performs a sequence of GATT Reads: temperature, then humidity, then battery level, decoding them as int16 in 0.01 °C, uint16 in 0.01%, and uint8 in %, and printing human‑readable values to the console. After finishing all three reads, it terminates the connection and restarts scanning, which produces a repeating log pattern of Odczyt -> Temperatura..., Odczyt -> Wilgotność..., Odczyt -> Bateria... followed by Rozłączono ... Restartuję skanowanie..

## BLE sensor simulator (ble_sensor_simulator)

The simulator is a GATT server (Peripheral) built with NimBLE that advertises continuously as ATC_8E4B89 so that the client treats it as a real BLE sensor. It defines two primary services: Environmental Sensing (0x181A) with Temperature (0x2A6E) and Humidity (0x2A6F) readable characteristics, and Battery Service (0x180F) with a readable Battery Level (0x2A19) characteristic.

On each client read, the simulator generates random but reasonable raw values: temperature in the range about 20–30 °C, humidity around 40–60%, and battery around 50–100%, encoded in exactly the same format as expected by the client. All read operations are logged on the simulator with messages such as READ Temp -> raw=2030 (20.30 C), READ Humi -> raw=5284 (52.84%), and READ Batt -> raw=80%.

The GAP event handler restarts advertising after every disconnection, so the log shows a loop of Client connected, three READ lines, Client disconnected reason=531, and then Advertising as 'ATC_8E4B89' started. This pattern mirrors the client's connect–read–disconnect cycle and is ideal for demonstrating a complete BLE Central/Peripheral interaction.

## Build and flash

Prerequisites for both projects: ESP-IDF properly installed and configured, and a classic ESP32 target (e.g. ESP32-WROOM). For each project, set the target once and build:

```bash
# In ble_sensor_simulator/
idf.py set-target esp32
idf.py build

# In ble_client/
idf.py set-target esp32
idf.py build
```

To flash and monitor the simulator on the first board:

```bash
cd ble_sensor_simulator
idf.py -p PORT_SIM flash monitor
```

To flash and monitor the client on the second board:

```bash
cd ble_client
idf.py -p PORT_CLIENT flash monitor
```

After boot, the simulator logs that it is advertising as ATC_8E4B89, and the client starts scanning and will quickly find, connect to, and read from this device.

## Testing scenarios and erase-flash

To test the behaviour when the server disappears, you can disconnect or reprogram the simulator board. If you completely erase the simulator's flash with:

```bash
idf.py -p PORT_SIM erase-flash
```

the external flash will be wiped, leaving only the ROM bootloader, so after reset the board will no longer run the simulator firmware and will not advertise ATC_8E4B89 until you flash a new application. In this state, the client's log will show only repeated scanning messages without finding the target, clearly indicating that no compatible BLE sensor is present.

Obviously, for the logs to appear and for the system to work correctly, both devices must have their Bluetooth connections active: the client must be scanning and the sensor simulator must be advertising. If either device is not running or has Bluetooth disabled, no communication will occur.
