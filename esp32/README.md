# ESP32 MQTT Sensor Data Publisher

This project connects an ESP32 to WiFi, synchronizes time via SNTP, and publishes simulated sensor data (humidity, motion, light level) to an MQTT broker at regular intervals.

## Features

- WiFi connection with automatic reconnection
- NTP time synchronization (Central European Time)
- MQTT client for publishing sensor data
- JSON formatted sensor data with timestamps
- Configurable via menuconfig

## Hardware Required

- Any ESP32 development board
- WiFi network access
- MQTT broker (e.g., Mosquitto, HiveMQ, etc.)

## Configuration

Before building, you need to configure your WiFi credentials and MQTT broker:

```bash
idf.py menuconfig
```

Navigate to **"MQTT Sensor Configuration"** and set:
- **WiFi SSID** - Your WiFi network name
- **WiFi Password** - Your WiFi password
- **MQTT Broker URI** - Your MQTT broker address (format: `mqtt://host:port`)
- **MQTT Sensor Data Topic** - Topic to publish to (default: `customer-1/location-1/device-1/sensor-data`)

## Build and Flash

Build the project and flash it to the board:

```bash
idf.py build
idf.py -p PORT flash monitor
```

Replace `PORT` with your ESP32's serial port (e.g., `/dev/ttyUSB0` on Linux, `COM3` on Windows).

To exit the serial monitor, type `Ctrl-]`.

## Example Output

```
I (3714) mqtt_example: Got IP address: 192.168.1.100
I (3714) mqtt_example: Initializing SNTP for time sync...
I (5314) mqtt_example: Time synchronized!
I (5314) mqtt_example: Current time: 2025-11-09 14:30:45
I (4164) mqtt_example: MQTT_EVENT_CONNECTED
I (65314) mqtt_example: Published sensor data, msg_id=1: [{"timestamp":"2025-11-09T14:31:45","humidity":65.0,"motionDetected":true,"lightLevel":450}]
```

## Sensor Data Format

The sensor data is published as a JSON array:

```json
[{
  "timestamp": "2025-11-09T14:31:45",
  "humidity": 65.0,
  "motionDetected": true,
  "lightLevel": 450
}]
```

- **timestamp**: ISO 8601 format (YYYY-MM-DDTHH:MM:SS)
- **humidity**: 30.0-80.0% (simulated)
- **motionDetected**: true/false (simulated)
- **lightLevel**: 100-1000 lux (simulated)

Data is published every 60 seconds.

## Project Structure

```
├── CMakeLists.txt
├── main/
│   ├── CMakeLists.txt
│   ├── Kconfig.projbuild
│   └── app_main.c
├── sdkconfig.defaults
└── README.md
```

## License

Public Domain (CC0)
I (454) coexist: coex firmware version: 8da3f50af
I (481) coexist: coexist rom version 5b8dcfa
I (481) main_task: Started on CPU0
I (481) main_task: Calling app_main()
I (482) fg_mcu_slave: *********************************************************************
I (491) fg_mcu_slave:                 ESP-Hosted-MCU Slave FW version :: 0.0.6
I (501) fg_mcu_slave:                 Transport used :: SDIO only
I (510) fg_mcu_slave: *********************************************************************
I (519) fg_mcu_slave: Supported features are:
I (524) fg_mcu_slave: - WLAN over SDIO
I (528) h_bt: - BT/BLE
I (531) h_bt:    - HCI Over SDIO
I (535) h_bt:    - BLE only
I (539) fg_mcu_slave: capabilities: 0xd
I (543) fg_mcu_slave: Supported extended features are:
I (549) h_bt: - BT/BLE (extended)
I (553) fg_mcu_slave: extended capabilities: 0x0
I (563) h_bt: ESP Bluetooth MAC addr: 40:4c:ca:5b:a0:8a
I (564) BLE_INIT: Using main XTAL as clock source
I (574) BLE_INIT: ble controller commit:[7491a85]
I (575) BLE_INIT: Bluetooth MAC: 40:4c:ca:5b:a0:8a
I (581) phy_init: phy_version 310,dde1ba9,Jun  4 2024,16:38:11
I (641) phy: libbtbb version: 04952fd, Jun  4 2024, 16:38:26
I (642) SDIO_SLAVE: Using SDIO interface
I (642) SDIO_SLAVE: sdio_init: sending mode: SDIO_SLAVE_SEND_STREAM
I (648) SDIO_SLAVE: sdio_init: ESP32-C6 SDIO RxQ[20] timing[0]

I (1155) fg_mcu_slave: Start Data Path
I (1165) fg_mcu_slave: Initial set up done
I (1165) slave_ctrl: event ESPInit
```

### esp_hosted: Example Output of the master device (ESP32-P4)

```
I (1833) sdio_wrapper: Function 0 Blocksize: 512
I (1843) sdio_wrapper: Function 1 Blocksize: 512
I (1843) H_SDIO_DRV: SDIO Host operating in STREAMING MODE
I (1853) H_SDIO_DRV: generate slave intr
I (1863) transport: Received INIT event from ESP32 peripheral
I (1873) transport: EVENT: 12
I (1873) transport: EVENT: 11
I (1873) transport: capabilities: 0xd
I (1873) transport: Features supported are:
I (1883) transport: 	 * WLAN
I (1883) transport: 	   - HCI over SDIO
I (1893) transport: 	   - BLE only
I (1893) transport: EVENT: 13
I (1893) transport: ESP board type is : 13

I (1903) transport: Base transport is set-up

I (1903) transport: Slave chip Id[12]
I (1913) hci_stub_drv: Host BT Support: Disabled
I (1913) H_SDIO_DRV: Received INIT event
I (1923) rpc_evt: EVENT: ESP INIT

I (1923) rpc_wrap: Received Slave ESP Init
I (2703) rpc_core: <-- RPC_Req  [0x116], uid 1
I (2823) rpc_rsp:  --> RPC_Resp [0x216], uid 1
I (2823) rpc_core: <-- RPC_Req  [0x139], uid 2
I (2833) rpc_rsp:  --> RPC_Resp [0x239], uid 2
I (2833) rpc_core: <-- RPC_Req  [0x104], uid 3
I (2843) rpc_rsp:  --> RPC_Resp [0x204], uid 3
I (2843) rpc_core: <-- RPC_Req  [0x118], uid 4
I (2933) rpc_rsp:  --> RPC_Resp [0x218], uid 4
I (2933) example_connect: Connecting to Cermakowifi...
I (2933) rpc_core: <-- RPC_Req  [0x11c], uid 5
I (2943) rpc_evt: Event [0x2b] received
I (2943) rpc_evt: Event [0x2] received
I (2953) rpc_evt: EVT rcvd: Wi-Fi Start
I (2953) rpc_core: <-- RPC_Req  [0x101], uid 6
I (2973) rpc_rsp:  --> RPC_Resp [0x21c], uid 5
I (2973) H_API: esp_wifi_remote_connect
I (2973) rpc_core: <-- RPC_Req  [0x11a], uid 7
I (2983) rpc_rsp:  --> RPC_Resp [0x201], uid 6
I (3003) rpc_rsp:  --> RPC_Resp [0x21a], uid 7
I (3003) example_connect: Waiting for IP(s)
I (5723) rpc_evt: Event [0x2b] received
I (5943) esp_wifi_remote: esp_wifi_internal_reg_rxcb: sta: 0x400309fe
0x400309fe: wifi_sta_receive at /home/david/esp/idf/components/esp_wifi/src/wifi_netif.c:38

I (7573) example_connect: Got IPv6 event: Interface "example_netif_sta" address: fe80:0000:0000:0000:424c:caff:fe5b:a088, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (9943) esp_netif_handlers: example_netif_sta ip: 192.168.0.29, mask: 255.255.255.0, gw: 192.168.0.1
I (9943) example_connect: Got IPv4 event: Interface "example_netif_sta" address: 192.168.0.29
I (9943) example_common: Connected to example_netif_sta
I (9953) example_common: - IPv4 address: 192.168.0.29,
I (9963) example_common: - IPv6 address: fe80:0000:0000:0000:424c:caff:fe5b:a088, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (9973) mqtt_example: Other event id:7
I (9973) main_task: Returned from app_main()
I (10253) mqtt_example: MQTT_EVENT_CONNECTED
I (10253) mqtt_example: sent publish successful, msg_id=45053
I (10253) mqtt_example: sent subscribe successful, msg_id=34643
I (10263) mqtt_example: sent subscribe successful, msg_id=2358
I (10263) mqtt_example: sent unsubscribe successful, msg_id=57769
I (10453) mqtt_example: MQTT_EVENT_PUBLISHED, msg_id=45053
I (10603) mqtt_example: MQTT_EVENT_SUBSCRIBED, msg_id=34643
I (10603) mqtt_example: sent publish successful, msg_id=0
I (10603) mqtt_example: MQTT_EVENT_SUBSCRIBED, msg_id=2358
I (10613) mqtt_example: sent publish successful, msg_id=0
I (10613) mqtt_example: MQTT_EVENT_UNSUBSCRIBED, msg_id=57769
I (10713) mqtt_example: MQTT_EVENT_DATA
TOPIC=/topic/qos0
DATA=data
I (10863) mqtt_example: MQTT_EVENT_DATA
TOPIC=/topic/qos0
DATA=data
```

### <a name="eppp"></a>eppp: Configure master-slave verification

In order to secure the physical connection between the ESP32-P4 (master) and the slave device, it is necessary to set certificates and keys for each side.
To bootstrap this step, you can use one-time generated self-signed RSA keys and certificates running:
```
./managed_components/espressif__esp_wifi_remote/examples/test_certs/generate_test_certs.sh espressif.local
```

#### eppp: Configure the slave project

It is recommended to create a new project from `esp_wifi_remote` component's example with
```
idf.py create-project-from-example "espressif/esp_wifi_remote:server"
```
but you can also build and flash the slave project directly from the `managed_components` directory using:
```
idf.py -C managed_components/espressif__esp_wifi_remote/examples/server/ -B build_slave
```

Please follow these steps to setup the slave application:
* `idf.py set-target` -- choose the slave target (must support Wi-Fi)
* `idf.py menuconfig` -- configure the physical connection and verification details:
  - `CONFIG_ESP_WIFI_REMOTE_EPPP_CLIENT_CA` -- CA for verifying ESP32-P4 application
  - `CONFIG_ESP_WIFI_REMOTE_EPPP_SERVER_CRT` -- slave's certificate
  - `CONFIG_ESP_WIFI_REMOTE_EPPP_SERVER_KEY` -- slave's private key
* `idf.py build flash monitor`

#### eppp: Configure the master project (ESP32-P4)

similarly to the slave project, we have to configure
* the physical connection
* the verification
  - `CONFIG_ESP_WIFI_REMOTE_EPPP_SERVER_CA` -- CA for verifying the slave application
  - `CONFIG_ESP_WIFI_REMOTE_EPPP_CLIENT_CRT` -- our own certificate
  - `CONFIG_ESP_WIFI_REMOTE_EPPP_CLIENT_KEY` -- our own private key

After project configuration, you build and flash the board with
```
idf.py build flash monitor
```

### eppp: Example Output of the slave device

```
I (7982) main_task: Returned from app_main()
I (8242) rpc_server: Received header id 2
I (8242) pp: pp rom version: 5b8dcfa
I (8242) net80211: net80211 rom version: 5b8dcfa
I (8252) wifi:wifi driver task: 4082be8c, prio:23, stack:6656, core=0
I (8252) wifi:wifi firmware version: feaf82d
I (8252) wifi:wifi certification version: v7.0
I (8252) wifi:config NVS flash: enabled
I (8262) wifi:config nano formatting: disabled
I (8262) wifi:mac_version:HAL_MAC_ESP32AX_761,ut_version:N, band:0x1
I (8272) wifi:Init data frame dynamic rx buffer num: 32
I (8272) wifi:Init static rx mgmt buffer num: 5
I (8282) wifi:Init management short buffer num: 32
I (8282) wifi:Init dynamic tx buffer num: 32
I (8292) wifi:Init static tx FG buffer num: 2
I (8292) wifi:Init static rx buffer size: 1700 (rxctrl:92, csi:512)
I (8302) wifi:Init static rx buffer num: 10
I (8302) wifi:Init dynamic rx buffer num: 32
I (8302) wifi_init: rx ba win: 6
I (8312) wifi_init: accept mbox: 6
I (8312) wifi_init: tcpip mbox: 32
I (8322) wifi_init: udp mbox: 6
I (8322) wifi_init: tcp mbox: 6
I (8322) wifi_init: tcp tx win: 5760
I (8332) wifi_init: tcp rx win: 5760
I (8332) wifi_init: tcp mss: 1440
I (8342) wifi_init: WiFi IRAM OP enabled
I (8342) wifi_init: WiFi RX IRAM OP enabled
I (8352) wifi_init: WiFi SLP IRAM OP enabled
I (8362) rpc_server: Received header id 11
I (8362) rpc_server: Received header id 4
I (8372) rpc_server: Received header id 6
I (8372) phy_init: phy_version 270,339aa07,Apr  3 2024,16:36:11
I (8492) wifi:enable tsf
I (8492) rpc_server: Received WIFI event 41
I (8502) rpc_server: Received WIFI event 2
I (8732) rpc_server: Received header id 10
I (8742) rpc_server: Received header id 5
I (8752) rpc_server: Received header id 8
I (11452) wifi:new:<6,0>, old:<1,0>, ap:<255,255>, sta:<6,0>, prof:1, snd_ch_cfg:0x0
I (11452) wifi:(connect)dot11_authmode:0x3, pairwise_cipher:0x3, group_cipher:0x1
I (11452) wifi:state: init -> auth (0xb0)
I (11462) rpc_server: Received WIFI event 41
I (11462) wifi:state: auth -> assoc (0x0)
I (11472) wifi:(assoc)RESP, Extended Capabilities length:8, operating_mode_notification:0
I (11472) wifi:(assoc)RESP, Extended Capabilities, MBSSID:0, TWT Responder:0, OBSS Narrow Bandwidth RU In OFDMA Tolerance:0
I (11482) wifi:Extended Capabilities length:8, operating_mode_notification:1
I (11492) wifi:state: assoc -> run (0x10)
I (11492) wifi:(trc)phytype:CBW20-SGI, snr:50, maxRate:144, highestRateIdx:0
W (11502) wifi:(trc)band:2G, phymode:3, highestRateIdx:0, lowestRateIdx:11, dataSchedTableSize:14
I (11512) wifi:(trc)band:2G, rate(S-MCS7, rateIdx:0), ampdu(rate:S-MCS7, schedIdx(0, stop:8)), snr:50, ampduState:wait operational
I (11522) wifi:ifidx:0, rssi:-45, nf:-95, phytype(0x3, CBW20-SGI), phymode(0x3, 11bgn), max_rate:144, he:0, vht:0, ht:1
I (11532) wifi:(ht)max.RxAMPDULenExponent:3(65535 bytes), MMSS:6(8 us)
W (11542) wifi:<ba-add>idx:0, ifx:0, tid:0, TAHI:0x1002cb4, TALO:0x1b942980, (ssn:0, win:64, cur_ssn:0), CONF:0xc0000005
I (11572) wifi:connected with Cermakowifi, aid = 2, channel 6, BW20, bssid = 80:29:94:1b:b4:2c
I (11572) wifi:cipher(pairwise:0x3, group:0x1), pmf:0, security:WPA2-PSK, phy:11bgn, rssi:-45
I (11582) wifi:pm start, type: 1, twt_start:0

I (11582) wifi:pm start, type:1, aid:0x2, trans-BSSID:80:29:94:1b:b4:2c, BSSID[5]:0x2c, mbssid(max-indicator:0, index:0), he:0
I (11592) wifi:dp: 1, bi: 102400, li: 3, scale listen interval from 307200 us to 307200 us
I (11602) wifi:set rx beacon pti, rx_bcn_pti: 10, bcn_timeout: 25000, mt_pti: 10, mt_time: 10000
I (11612) wifi:[ADDBA]TX addba request, tid:0, dialogtoken:1, bufsize:64, A-MSDU:0(not supported), policy:1(IMR), ssn:0(0x0)
I (11622) wifi:[ADDBA]TX addba request, tid:7, dialogtoken:2, bufsize:64, A-MSDU:0(not supported), policy:1(IMR), ssn:0(0x20)
I (11632) wifi:[ADDBA]TX addba request, tid:5, dialogtoken:3, bufsize:64, A-MSDU:0(not supported), policy:1(IMR), ssn:0(0x0)
I (11642) wifi:[ADDBA]RX addba response, status:0, tid:7/tb:0(0x1), bufsize:64, batimeout:0, txa_wnd:64
I (11652) wifi:[ADDBA]RX addba response, status:0, tid:5/tb:0(0x1), bufsize:64, batimeout:0, txa_wnd:64
I (11662) wifi:[ADDBA]RX addba response, status:0, tid:0/tb:1(0x1), bufsize:64, batimeout:0, txa_wnd:64
I (11672) wifi:AP's beacon interval = 102400 us, DTIM period = 1
I (11682) rpc_server: Received WIFI event 4
I (15682) esp_netif_handlers: sta ip: 192.168.0.33, mask: 255.255.255.0, gw: 192.168.0.1
I (15682) rpc_server: Received IP event 0
I (15682) rpc_server: Main DNS:185.162.24.55
I (15682) rpc_server: IP address:192.168.0.33
```

### eppp: Example Output of the master device (ESP32-P4)

```
I (445) example_connect: Start example_connect.
I (455) uart: queue free spaces: 16
I (455) eppp_link: Waiting for IP address 0
I (3195) esp-netif_lwip-ppp: Connected
I (3195) eppp_link: Got IPv4 event: Interface "pppos_client(EPPP0)" address: 192.168.11.2
I (3195) esp-netif_lwip-ppp: Connected
I (3195) eppp_link: Connected! 0
I (5475) example_connect: Waiting for IP(s)
I (8405) esp_wifi_remote: esp_wifi_internal_reg_rxcb: sta: 0x4001c68a
I (9445) example_connect: Got IPv6 event: Interface "pppos_client" address: fe80:0000:0000:0000:5632:04ff:fe08:5054, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (12415) rpc_client: Main DNS:185.162.24.55
I (12415) esp_netif_handlers: pppos_client ip: 192.168.11.2, mask: 255.255.255.255, gw: 192.168.11.1
I (12415) rpc_client: EPPP IP:192.168.11.1
I (12415) example_connect: Got IPv4 event: Interface "pppos_client" address: 192.168.11.2
I (12425) rpc_client: WIFI IP:192.168.0.33
I (12435) example_common: Connected to pppos_client
I (12445) rpc_client: WIFI GW:192.168.0.1
I (12455) example_common: - IPv6 address: fe80:0000:0000:0000:5632:04ff:fe08:5054, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (12455) rpc_client: WIFI mask:255.255.255.0
I (12465) example_common: Connected to pppos_client
I (12475) example_common: - IPv4 address: 192.168.11.2,
I (12475) example_common: - IPv6 address: fe80:0000:0000:0000:5c3b:1291:05ca:6dc8, type: ESP_IP6_ADDR_IS_LINK_LOCAL
I (12495) mqtt_example: Other event id:7
I (12495) main_task: Returned from app_main()
I (12905) mqtt_example: MQTT_EVENT_CONNECTED
I (12905) mqtt_example: sent publish successful, msg_id=36013
I (12905) mqtt_example: sent subscribe successful, msg_id=44233
I (12905) mqtt_example: sent subscribe successful, msg_id=36633
I (12915) mqtt_example: sent unsubscribe successful, msg_id=15480
I (13115) mqtt_example: MQTT_EVENT_PUBLISHED, msg_id=36013
I (13415) mqtt_example: MQTT_EVENT_SUBSCRIBED, msg_id=44233
I (13415) mqtt_example: sent publish successful, msg_id=0
I (13415) mqtt_example: MQTT_EVENT_SUBSCRIBED, msg_id=36633
I (13415) mqtt_example: sent publish successful, msg_id=0
I (13425) mqtt_example: MQTT_EVENT_DATA
TOPIC=/topic/qos1
DATA=data_3
I (13435) mqtt_example: MQTT_EVENT_UNSUBSCRIBED, msg_id=15480
I (13615) mqtt_example: MQTT_EVENT_DATA
TOPIC=/topic/qos0
DATA=data
I (13925) mqtt_example: MQTT_EVENT_DATA
TOPIC=/topic/qos0
```
