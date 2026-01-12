# BLE Provisioning Implementation for Device Claiming

## Overview

This document describes the complete BLE provisioning implementation that supports the claiming/unbinding process as defined by the backend system.

## Architecture

### Phase A: Discovery & Secure Handshake (Local)

**Device Side Implementation:**
- Device broadcasts BLE advertisements with the name `LED_WiFi_Config`
- Upon BLE connection, the device exposes two READ-only characteristics:
  - **MAC Address** (6 bytes): Device's WiFi MAC address
  - **Proof of Possession (PoP)** (32 bytes): Cryptographically random token generated on first boot

**MAC Address:**
```cpp
void ble_provisioning_get_mac_address(uint8_t mac[6]);
```
- Reads the ESP32's WiFi Station MAC address using `esp_read_mac()`
- Returns 6 bytes in format: `AA:BB:CC:DD:EE:FF`

**Proof of Possession (PoP):**
```cpp
bool ble_provisioning_get_pop(uint8_t pop[32]);
```
- On first boot: Generates 32 random bytes using `esp_fill_random()` and stores in NVS
- On subsequent boots: Loads the saved PoP from NVS
- PoP persists across WiFi resets but NOT across factory resets
- Storage location: NVS namespace `device`, key `pop`

### Phase B: Network Configuration (Local)

**Device Side Implementation:**
The device exposes BLE characteristics for WiFi configuration:

1. **SSID Characteristic** (READ/WRITE)
   - UUID: `...de:f2`
   - Mobile app writes WiFi SSID (max 32 chars)
   - Device stores in temporary buffer

2. **Password Characteristic** (WRITE only)
   - UUID: `...de:f3`
   - Mobile app writes WiFi password (max 64 chars)
   - Device stores in temporary buffer

3. **Status Characteristic** (READ/NOTIFY)
   - UUID: `...de:f4`
   - Values:
     - `0x00`: Ready
     - `0x01`: SSID set
     - `0x02`: Password set
     - `0x03`: Saved successfully
     - `0xFF`: Error
   - Device notifies mobile app of status changes

4. **Command Characteristic** (WRITE only)
   - UUID: `...de:f5`
   - Commands:
     - `0x01`: SAVE - Save WiFi credentials and restart
     - `0x02`: RESET - Clear WiFi config only
     - `0x03`: FACTORY_RESET - Unbind device (full reset)

**WiFi Connection Flow:**
1. Mobile app writes SSID and Password
2. Mobile app writes command `0x01` (SAVE)
3. Device saves WiFi credentials to NVS
4. Device terminates BLE connection
5. Device restarts and connects to WiFi

### Phase C: Registration (Cloud)

**Backend Integration:**

The mobile app collects:
- MAC Address (from BLE characteristic)
- Proof of Possession (from BLE characteristic)
- Location (user input)
- Device Name (user input)

The backend receives a `claimDevice` request with this data and:
1. Validates the PoP
2. Links the device to the user account
3. Stores device metadata (location, name)

**Device Side:**
- Once WiFi is connected, the device can communicate with the backend via MQTT
- Device publishes to topics based on configured customer/location/device IDs
- Backend can send commands via MQTT to the device

## Unbinding Process (Offboarding)

**Trigger:**
Backend sends command `0x03` (FACTORY_RESET) via BLE when user removes the device.

**Device Implementation:**
```cpp
void ble_provisioning_factory_reset();
```

**Process:**
1. Device receives factory reset command via BLE
2. Device wipes WiFi credentials from NVS
3. Device notifies mobile app (status update)
4. Device terminates BLE connection
5. Device restarts and enters provisioning mode
6. Device is now ready to be claimed by a new user

**Important Notes:**
- PoP is **preserved** during factory reset (remains in NVS)
- This allows backend to track device history
- Only a full flash erase would reset the PoP
- WiFi credentials are completely erased

## BLE Service Structure

**Service UUID:** `12345678-9abc-def0-1234-56789abcdef0`

**Characteristics:**

| Characteristic | UUID | Properties | Purpose |
|---------------|------|------------|---------|
| MAC Address | `...de:f0` | READ | Device identification |
| Proof of Possession | `...de:f1` | READ | Security token for claiming |
| WiFi SSID | `...de:f2` | READ/WRITE | WiFi network name |
| WiFi Password | `...de:f3` | WRITE | WiFi password (security) |
| Status | `...de:f4` | READ/NOTIFY | Provisioning status updates |
| Command | `...de:f5` | WRITE | Control commands |

## Code Files Modified

### 1. `ble_provisioning.h`
Added new public API functions:
- `ble_provisioning_get_mac_address()`
- `ble_provisioning_get_pop()`
- `ble_provisioning_factory_reset()`

### 2. `ble_provisioning.cpp`
Implemented:
- MAC address retrieval from ESP32 hardware
- PoP generation and NVS storage/retrieval
- Two new BLE characteristics (MAC, PoP) with READ access
- Factory reset command (0x03) handling
- Enhanced command processing with unbinding support

### 3. NVS Storage Schema

**WiFi Configuration:**
- Namespace: `wifi_cfg`
- Keys: `ssid`, `password`

**Device Identity:**
- Namespace: `device`
- Keys: `pop` (32 bytes)

## Usage Flow

### Initial Setup (First Boot)
1. Device generates PoP and stores in NVS
2. Device enters BLE provisioning mode
3. Mobile app discovers and connects
4. Mobile app reads MAC and PoP
5. User enters WiFi credentials
6. Mobile app sends claimDevice request to backend (with MAC, PoP, location, name)
7. Backend validates and links device to user
8. Mobile app sends WiFi credentials to device
9. Device saves and restarts
10. Device connects to WiFi and MQTT

### Re-provisioning (Factory Reset)
1. User taps "Remove Device" in mobile app
2. Backend sends factory reset command to device via BLE
3. Device wipes WiFi credentials
4. Device restarts in provisioning mode
5. New user can claim the device (same PoP)

### Button-Triggered Provisioning
1. User holds BOOT button for 3 seconds
2. Device enters BLE provisioning mode
3. Mobile app can read MAC/PoP and reconfigure WiFi

## Security Considerations

1. **PoP Uniqueness**: Each device has a unique 32-byte random token
2. **PoP Persistence**: PoP survives factory resets, allowing backend to track device history
3. **Password Security**: WiFi password is write-only (cannot be read back via BLE)
4. **BLE Timeout**: Provisioning mode automatically times out after 60 seconds
5. **Connection Security**: BLE connection can be terminated remotely by backend

## Testing Checklist

- [ ] First boot generates and stores PoP correctly
- [ ] MAC address is readable via BLE
- [ ] PoP is readable via BLE (32 bytes)
- [ ] WiFi provisioning works (SSID + Password + SAVE command)
- [ ] Factory reset command (0x03) wipes WiFi and restarts
- [ ] PoP persists after factory reset
- [ ] Device re-enters provisioning mode after factory reset
- [ ] Button hold (3s) triggers provisioning mode
- [ ] Provisioning timeout (60s) stops BLE advertising
- [ ] Status notifications work correctly

## Integration with Backend

The backend should implement:

1. **claimDevice API endpoint**
   - Input: `{ macAddress: string, proofOfPossession: string, location: string, deviceName: string, userId: string }`
   - Validates PoP is unique or previously unclaimed
   - Creates device record linked to user
   - Returns success/failure

2. **unbindDevice API endpoint**
   - Input: `{ deviceId: string, userId: string }`
   - Validates user owns device
   - Sends factory reset command (0x03) via BLE if device is nearby
   - Removes device-user link in database
   - Marks device as available for re-claiming

3. **MQTT Integration**
   - Device publishes telemetry to: `customer/{custId}/location/{locId}/device/{deviceId}/telemetry`
   - Backend can send commands to: `customer/{custId}/location/{locId}/device/{deviceId}/cmd`

## Troubleshooting

**Issue: PoP not generating**
- Check NVS is initialized: `wifi_config_nvs_init()` must be called first
- Check flash has space for NVS partition

**Issue: MAC address all zeros**
- Ensure WiFi is initialized before reading MAC

**Issue: Factory reset not working**
- Verify command byte is exactly `0x03`
- Check BLE connection is active when command is sent
- Verify NVS erase permissions

**Issue: Device not advertising**
- Check BLE stack is initialized and synced
- Verify no WiFi config exists (otherwise provisioning is skipped)
- Check provisioning timeout hasn't expired

## Future Enhancements

1. **OTA Updates**: Add BLE characteristic for firmware updates
2. **Device Information**: Add characteristics for firmware version, hardware revision
3. **Encrypted BLE**: Implement BLE pairing and encryption
4. **PoP Rotation**: Allow backend to request new PoP generation
5. **Multi-WiFi**: Support multiple WiFi network profiles
