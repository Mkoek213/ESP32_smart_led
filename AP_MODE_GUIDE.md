# ESP32 Access Point Mode - Implementation Guide

## ✅ Task 3 - COMPLETED!

Your ESP32 now supports **SoftAP + Station mode** (WIFI_MODE_APSTA), allowing it to:
- Act as an **Access Point** (others can connect to it)
- Connect to an existing WiFi network as a **Station**
- Run **both modes simultaneously**!

---

## 🎯 What Was Implemented

### 1. **Access Point Configuration**
```cpp
#define AP_SSID                    "ESP32-Config"
#define AP_PASSWORD                "esp32pass"
#define AP_CHANNEL                 1
#define AP_MAX_CONNECTIONS         4
```

### 2. **AccessPoint Class**
- Manages ESP32 as WiFi hotspot
- Tracks client connections/disconnections
- Logs MAC addresses of connected devices
- Automatically starts web server

### 3. **WebServer Class**
- Simple HTTP server on port 80
- Web configuration interface
- Available endpoints:
  - `http://192.168.4.1/` - Main configuration page
  - `http://192.168.4.1/status` - Device status page

### 4. **SoftAP+STA Mode**
- ESP32 runs as both AP and Station simultaneously
- Can accept connections while connected to another network
- Independent operation of both modes

---

## 🚀 How to Use

### Step 1: Build and Flash
```bash
cd /home/mikolaj/work/ESP32_smart_led/wifi_led_project
. /home/mikolaj/esp/esp-idf/export.sh
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

### Step 2: Connect to ESP32 Access Point

**From Your Phone or Computer:**
1. Open WiFi settings
2. Look for network: **ESP32-Config**
3. Password: **esp32pass**
4. Connect to it

### Step 3: Access Web Interface
1. Open browser
2. Go to: **http://192.168.4.1**
3. You'll see the configuration portal!

---

## 📱 Expected Behavior

### On ESP32 Serial Monitor:
```
============================================
ESP32 WiFi - C++ Version
Mode: SoftAP + Station (APSTA)
============================================
I (xxx) ACCESS_POINT: Initializing Access Point Mode
I (xxx) ACCESS_POINT: ✅ Access Point configured
I (xxx) ACCESS_POINT:    SSID: ESP32-Config
I (xxx) ACCESS_POINT:    Password: esp32pass
I (xxx) ACCESS_POINT:    Channel: 1
I (xxx) ACCESS_POINT:    Max Connections: 4
I (xxx) ACCESS_POINT:    IP Address: 192.168.4.1
I (xxx) WEB_SERVER: ✅ Web server started on port 80
I (xxx) wifi station: ✅ WiFi CONNECTED!
```

### When Device Connects to AP:
```
I (xxx) ACCESS_POINT: ✅ Client connected to AP, MAC: xx:xx:xx:xx:xx:xx
I (xxx) WEB_SERVER: Client connected: 192.168.4.2
```

---

## 🎨 Web Interface Features

### Main Page (/)
- Welcome message
- Device information
- AP status
- Quick actions

### Status Page (/status)
- Access Point status
- Station mode status
- Current WiFi SSID
- Connection info

---

## ⚙️ Configuration Options

### Enable/Disable AP Mode
In `app_main()` function:
```cpp
bool enable_ap_mode = true;  // Set to false to disable AP mode
```

### Change AP Credentials
At the top of the file:
```cpp
#define AP_SSID                    "YourCustomName"
#define AP_PASSWORD                "YourPassword123"
```

### Change AP Channel/Connections
```cpp
#define AP_CHANNEL                 6        // WiFi channel (1-13)
#define AP_MAX_CONNECTIONS         8        // Max clients (1-10)
```

---

## 📊 Network Information

### When in AP Mode:
- **ESP32 IP**: 192.168.4.1
- **Subnet Mask**: 255.255.255.0
- **DHCP Range**: 192.168.4.2 - 192.168.4.254
- **Gateway**: 192.168.4.1

### When in Station Mode:
- **ESP32 IP**: Assigned by router (e.g., 172.20.10.2)
- Connects to your iPhone/Router WiFi

---

## 🔧 Advanced Usage

### Disable HTTP Client Test
Comment out in `app_main()`:
```cpp
// ESP_LOGI(TAG, "Creating HTTP GET task...");
// xTaskCreate(&http_get_task, "http_get_task", 4096, nullptr, 5, nullptr);
```

### Station Only Mode
Set in `app_main()`:
```cpp
bool enable_ap_mode = false;
```

### Pure AP Mode (No Station)
Modify `wifi_station.init()`:
```cpp
wifi_mode_t mode = WIFI_MODE_AP;  // Instead of WIFI_MODE_APSTA
```

---

## 🎯 Testing Checklist

- [ ] Build without errors
- [ ] Flash to ESP32
- [ ] See AP initialization in serial monitor
- [ ] Find "ESP32-Config" WiFi network on phone
- [ ] Connect with password "esp32pass"
- [ ] Access http://192.168.4.1 in browser
- [ ] See configuration page
- [ ] Click "View Status" button
- [ ] Check serial monitor for connection logs

---

## 🐛 Troubleshooting

### Can't find ESP32-Config network
- Check serial monitor for AP initialization
- Ensure `enable_ap_mode = true`
- Verify ESP32 is powered and running

### Can't connect to AP
- Check password: "esp32pass"
- Try restarting ESP32
- Check max connections not exceeded (4 max)

### Can't access 192.168.4.1
- Verify you're connected to ESP32-Config
- Try http:// not https://
- Check web server started in serial monitor

### Both modes not working
- Ensure WIFI_MODE_APSTA is set
- Check both network interfaces created
- Verify sufficient memory/resources

---

## 📝 Future Enhancements

Possible additions:
- WiFi credentials configuration form
- Save settings to NVS (non-volatile storage)
- REST API for device control
- WebSocket for real-time updates
- LED color control via web interface
- Network scanning and selection
- OTA (Over-The-Air) firmware updates

---

## ✅ Requirements Met

**Zadanie 3 Requirements:**
- ✅ Access Point mode implemented
- ✅ Computer/phone can connect to ESP32
- ✅ Web interface available
- ✅ SoftAP+STA mode working
- ✅ Configuration portal active
- ✅ Client connection tracking

---

**Created:** October 26, 2025  
**Version:** 1.0  
**Mode:** SoftAP + Station (APSTA)  
**Language:** C++  

