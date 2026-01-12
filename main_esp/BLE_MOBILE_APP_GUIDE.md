# BLE Mobile App Integration Guide

## Quick Start for Mobile App Developers

This guide shows you exactly how to integrate with the ESP32 device's BLE provisioning service.

## BLE Service Details

### Service UUID
```
12345678-9abc-def0-1234-56789abcdef0
```

### Characteristics

| Name | UUID | Type | Description |
|------|------|------|-------------|
| MAC Address | `12345678-9abc-def0-1234-56789abcdef0` | READ | 6 bytes - Device MAC |
| Proof of Possession | `12345678-9abc-def0-1234-56789abcdef1` | READ | 32 bytes - Security token |
| WiFi SSID | `12345678-9abc-def0-1234-56789abcdef2` | READ/WRITE | String (max 32) |
| WiFi Password | `12345678-9abc-def0-1234-56789abcdef3` | WRITE | String (max 64) |
| Status | `12345678-9abc-def0-1234-56789abcdef4` | READ/NOTIFY | 1 byte - Status code |
| Command | `12345678-9abc-def0-1234-56789abcdef5` | WRITE | 1 byte - Command code |

## Discovery & Handshake Flow

### Step 1: Scan for Device
```javascript
// Pseudo-code for React Native / Expo
import { BleManager } from 'react-native-ble-plx';

const manager = new BleManager();

manager.startDeviceScan(null, null, (error, device) => {
  if (device?.name === 'LED_WiFi_Config') {
    manager.stopDeviceScan();
    connectToDevice(device);
  }
});
```

### Step 2: Connect to Device
```javascript
async function connectToDevice(device) {
  await device.connect();
  await device.discoverAllServicesAndCharacteristics();
  
  // Service UUID
  const serviceUUID = '12345678-9abc-def0-1234-56789abcdef0';
  
  // Read MAC Address (auto-send to backend)
  const macData = await device.readCharacteristicForService(
    serviceUUID,
    '12345678-9abc-def0-1234-56789abcdef0'
  );
  const macAddress = parseMAC(macData); // "AA:BB:CC:DD:EE:FF"
  
  // Read Proof of Possession (auto-send to backend)
  const popData = await device.readCharacteristicForService(
    serviceUUID,
    '12345678-9abc-def0-1234-56789abcdef1'
  );
  const pop = base64Encode(popData); // 32 bytes as base64
  
  // Store for later use
  return { device, macAddress, pop };
}
```

### Step 3: Helper Functions
```javascript
function parseMAC(base64Data) {
  // Convert base64 to hex string with colons
  const buffer = Buffer.from(base64Data, 'base64');
  return Array.from(buffer)
    .map(b => b.toString(16).padStart(2, '0').toUpperCase())
    .join(':');
}

function base64Encode(base64Data) {
  // Already base64 from BLE library, just ensure it's a string
  return base64Data;
}
```

## WiFi Configuration Flow

### Step 4: Subscribe to Status Updates
```javascript
async function subscribeToStatus(device) {
  const serviceUUID = '12345678-9abc-def0-1234-56789abcdef0';
  const statusUUID = '12345678-9abc-def0-1234-56789abcdef4';
  
  device.monitorCharacteristicForService(
    serviceUUID,
    statusUUID,
    (error, characteristic) => {
      if (characteristic) {
        const status = Buffer.from(characteristic.value, 'base64')[0];
        handleStatusUpdate(status);
      }
    }
  );
}

function handleStatusUpdate(status) {
  switch (status) {
    case 0x00:
      console.log('Device ready');
      break;
    case 0x01:
      console.log('SSID received');
      break;
    case 0x02:
      console.log('Password received');
      break;
    case 0x03:
      console.log('WiFi saved! Device restarting...');
      // Close BLE connection, device will restart
      break;
    case 0xFF:
      console.error('Error occurred');
      break;
  }
}
```

### Step 5: Send WiFi Credentials
```javascript
async function sendWiFiCredentials(device, ssid, password) {
  const serviceUUID = '12345678-9abc-def0-1234-56789abcdef0';
  
  // Write SSID
  const ssidUUID = '12345678-9abc-def0-1234-56789abcdef2';
  await device.writeCharacteristicWithResponseForService(
    serviceUUID,
    ssidUUID,
    base64Encode(ssid)
  );
  
  // Write Password
  const passwordUUID = '12345678-9abc-def0-1234-56789abcdef3';
  await device.writeCharacteristicWithResponseForService(
    serviceUUID,
    passwordUUID,
    base64Encode(password)
  );
  
  // Send SAVE command (0x01)
  const commandUUID = '12345678-9abc-def0-1234-56789abcdef5';
  await device.writeCharacteristicWithResponseForService(
    serviceUUID,
    commandUUID,
    base64Encode(Buffer.from([0x01]))
  );
  
  // Device will now save, restart, and connect to WiFi
}
```

## Backend Integration (Claiming)

### Step 6: Send Claim Request to Backend
```javascript
async function claimDevice(macAddress, pop, location, deviceName) {
  const response = await fetch('https://your-backend.com/api/devices/claim', {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${userToken}`
    },
    body: JSON.stringify({
      macAddress: macAddress,      // "AA:BB:CC:DD:EE:FF"
      proofOfPossession: pop,       // Base64 string (32 bytes)
      location: location,            // "Home" (user input)
      deviceName: deviceName,        // "Kitchen Light" (user input)
      userId: currentUserId
    })
  });
  
  if (!response.ok) {
    throw new Error('Failed to claim device');
  }
  
  return await response.json();
}
```

## Complete Claiming Flow

```javascript
async function completeProvisioningFlow() {
  try {
    // 1. Scan and connect
    const device = await scanForDevice();
    const { device, macAddress, pop } = await connectToDevice(device);
    
    // 2. User input
    const location = await promptUser('Select location');
    const deviceName = await promptUser('Enter device name');
    const ssid = await promptUser('Select WiFi network');
    const password = await promptUser('Enter WiFi password');
    
    // 3. Send to backend FIRST (validation)
    await claimDevice(macAddress, pop, location, deviceName);
    
    // 4. Configure WiFi on device
    await subscribeToStatus(device);
    await sendWiFiCredentials(device, ssid, password);
    
    // 5. Wait for success
    await waitForStatus(0x03); // STATUS_SAVED
    
    // 6. Device will restart and connect to WiFi
    await device.cancelConnection();
    
    showSuccess('Device claimed successfully!');
  } catch (error) {
    showError(`Provisioning failed: ${error.message}`);
  }
}
```

## Unbinding Flow (Factory Reset)

### Backend Triggered
```javascript
async function unbindDevice(deviceId) {
  // Backend API call
  const response = await fetch(`https://your-backend.com/api/devices/${deviceId}/unbind`, {
    method: 'POST',
    headers: {
      'Content-Type': 'application/json',
      'Authorization': `Bearer ${userToken}`
    }
  });
  
  // Backend will:
  // 1. Send factory reset command (0x03) to device via BLE (if nearby)
  // 2. Remove device-user link from database
  // 3. Mark device as available for claiming
  
  return await response.json();
}
```

### Manual Factory Reset via BLE
```javascript
async function factoryResetDevice(device) {
  const serviceUUID = '12345678-9abc-def0-1234-56789abcdef0';
  const commandUUID = '12345678-9abc-def0-1234-56789abcdef5';
  
  // Send FACTORY_RESET command (0x03)
  await device.writeCharacteristicWithResponseForService(
    serviceUUID,
    commandUUID,
    base64Encode(Buffer.from([0x03]))
  );
  
  // Device will:
  // 1. Wipe WiFi credentials
  // 2. Restart
  // 3. Enter provisioning mode (BLE advertising)
  
  await device.cancelConnection();
}
```

## Status Codes Reference

| Code | Name | Description |
|------|------|-------------|
| `0x00` | READY | Device ready for provisioning |
| `0x01` | SSID_SET | SSID received successfully |
| `0x02` | PASS_SET | Password received successfully |
| `0x03` | SAVED | WiFi saved, device restarting |
| `0xFF` | ERROR | Error occurred during provisioning |

## Command Codes Reference

| Code | Name | Description |
|------|------|-------------|
| `0x01` | SAVE | Save WiFi credentials and restart |
| `0x02` | RESET | Clear WiFi config only |
| `0x03` | FACTORY_RESET | Complete unbind (wipe WiFi, restart) |

## Error Handling

### Common Issues

**Device not found:**
```javascript
// Increase scan timeout
setTimeout(() => {
  if (!deviceFound) {
    showError('Device not found. Make sure it\'s powered on and in range.');
  }
}, 10000); // 10 seconds
```

**Connection timeout:**
```javascript
try {
  await Promise.race([
    device.connect(),
    new Promise((_, reject) => 
      setTimeout(() => reject(new Error('Connection timeout')), 5000)
    )
  ]);
} catch (error) {
  showError('Failed to connect. Try again.');
}
```

**Backend validation fails:**
```javascript
try {
  await claimDevice(macAddress, pop, location, deviceName);
} catch (error) {
  if (error.status === 409) {
    showError('Device already claimed by another user');
  } else if (error.status === 400) {
    showError('Invalid device credentials');
  } else {
    showError('Failed to claim device. Try again.');
  }
}
```

## React Native Example Component

```javascript
import React, { useState } from 'react';
import { View, Button, Text, TextInput } from 'react-native';
import { BleManager } from 'react-native-ble-plx';

const DeviceProvisioningScreen = () => {
  const [status, setStatus] = useState('Scanning...');
  const [ssid, setSsid] = useState('');
  const [password, setPassword] = useState('');
  const [device, setDevice] = useState(null);
  const [macAddress, setMacAddress] = useState('');
  const [pop, setPop] = useState('');
  
  const manager = new BleManager();
  
  const startProvisioning = async () => {
    // Scan
    manager.startDeviceScan(null, null, async (error, dev) => {
      if (dev?.name === 'LED_WiFi_Config') {
        manager.stopDeviceScan();
        setStatus('Found device! Connecting...');
        
        // Connect
        await dev.connect();
        await dev.discoverAllServicesAndCharacteristics();
        
        // Read MAC & PoP
        const serviceUUID = '12345678-9abc-def0-1234-56789abcdef0';
        const macData = await dev.readCharacteristicForService(
          serviceUUID, '12345678-9abc-def0-1234-56789abcdef0'
        );
        const popData = await dev.readCharacteristicForService(
          serviceUUID, '12345678-9abc-def0-1234-56789abcdef1'
        );
        
        setDevice(dev);
        setMacAddress(parseMAC(macData));
        setPop(popData);
        setStatus('Connected! Enter WiFi credentials.');
      }
    });
  };
  
  const provisionDevice = async () => {
    setStatus('Claiming device...');
    
    // 1. Claim on backend
    await claimDevice(macAddress, pop, 'Home', 'My Device');
    
    // 2. Send WiFi credentials
    const serviceUUID = '12345678-9abc-def0-1234-56789abcdef0';
    await device.writeCharacteristicWithResponseForService(
      serviceUUID, '12345678-9abc-def0-1234-56789abcdef2',
      btoa(ssid)
    );
    await device.writeCharacteristicWithResponseForService(
      serviceUUID, '12345678-9abc-def0-1234-56789abcdef3',
      btoa(password)
    );
    await device.writeCharacteristicWithResponseForService(
      serviceUUID, '12345678-9abc-def0-1234-56789abcdef5',
      btoa(String.fromCharCode(0x01))
    );
    
    setStatus('Success! Device is connecting to WiFi.');
    await device.cancelConnection();
  };
  
  return (
    <View>
      <Text>{status}</Text>
      {!device && <Button title="Start" onPress={startProvisioning} />}
      {device && (
        <>
          <Text>MAC: {macAddress}</Text>
          <TextInput placeholder="WiFi SSID" value={ssid} onChangeText={setSsid} />
          <TextInput placeholder="Password" value={password} onChangeText={setPassword} secureTextEntry />
          <Button title="Provision" onPress={provisionDevice} />
        </>
      )}
    </View>
  );
};
```

## Testing with nRF Connect App

For development testing, use the **nRF Connect** app:

1. Scan for `LED_WiFi_Config`
2. Connect to device
3. Navigate to service `12345678-9abc-def0-1234-56789abcdef0`
4. Read MAC Address characteristic (6 bytes)
5. Read PoP characteristic (32 bytes)
6. Write SSID: `"MyWiFi"` (as ASCII/UTF-8)
7. Write Password: `"password123"` (as ASCII/UTF-8)
8. Write Command: `0x01` (single byte)
9. Device will save and restart

## Support

For issues or questions, refer to the main documentation:
- `PROVISIONING_IMPLEMENTATION.md` - Complete technical details
- ESP32 logs - Check serial monitor for detailed debugging
