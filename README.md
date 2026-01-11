# ESP32 Smart LED System

A comprehensive IoT solution featuring a dual-ESP32 setup for smart lighting control. The system integrates
environmental sensing with adaptive lighting, managed through a robust backend infrastructure.

## Project Overview

The project consists of two main hardware components:

1. **Sensor Node (ESP32)**: Monitors environmental conditions including humidity, light levels, and motion.
2. **Actuator Node (ESP32)**: Controls RGB LEDs, adapting color and brightness based on sensor data received from the
   first node.

These devices communicate with a central server to log data and receive user commands via a mobile application.

## System Architecture

The system is built on a microservices-like architecture orchestrated via Docker Compose:

- **Backend Service**: A Spring Boot application that manages business logic, exposes a REST API, and handles MQTT
  messaging.
- **MQTT Broker**: Eclipse Mosquitto acting as the communication bridge between ESP32 devices and the backend.
- **Database**: PostgreSQL for persistent storage of user data, device configurations, and telemetry.

## Communication Protocol

Communication between the ESP32 devices and the backend relies on the MQTT protocol.

### Topic Structure

The topic hierarchy is designed for multi-tenancy and precise device targeting:
`customer/{customerId}/location/{locationId}/device/{deviceId}/{type}`

### Topic Types & Payloads

| Topic Type    | Suffix       | Direction        | Description                              |
|:--------------|:-------------|:-----------------|:-----------------------------------------|
| **Telemetry** | `/telemetry` | Device → Backend | Batch sensor readings.                   |
| **Status**    | `/status`    | Device → Backend | Connectivity state (`online`/`offline`). |
| **Command**   | `/cmd`       | Backend → Device | Remote control instructions.             |

#### Payload Examples

**Telemetry** (JSON Array):

```json
[
  {
    "timestamp": 1702464000000,
    "temperature": 22.5,
    "humidity": 45.0,
    "pressure": 1013.2,
    "motionDetected": true
  }
]
```

**Status** (JSON Object):

```json
{
  "state": "online"
}
```

**Command** (JSON Object):

```json
{
  "type": "SET_COLOR",
  "payload": {
    "red": 255,
    "green": 0,
    "blue": 0
  }
}
```

## Getting Started

### Prerequisites

- **Docker** and **Docker Compose** installed on your machine.

### Running the Application

The entire backend stack can be launched with a single command.

1. Navigate to the project root directory.
2. Run the following command:

   ```bash
   docker compose up --build
   ```

This will:

- Build the Spring Boot backend image.
- Start **PostgreSQL** (Port 5432).
- Start **Mosquitto MQTT Broker** (Ports 1883, 9001).
- Start **Backend Service** (Port 8080).

To stop the services, run `docker compose down` or press `Ctrl+C`.

## Mobile Application (Frontend)

The user interface is a Progressive Web App (PWA) built with **React** and **Vite**, designed to be installed on mobile devices for native-like control without App Store distribution.

### Features
- **User Authentication**: Secure Login/Registration.
- **Dashboard**: Room-based organization of devices.
- **Real-time Control**: Toggle devices, change colors, and view live telemetry.
- **Device Provisioning**: Integrated **Web Bluetooth** flow to claim and configure new ESP32 devices directly from the browser (supports **Bluefy** on iOS).

### Running the Frontend locally

1. Navigate to the frontend directory:
   ```bash
   cd frontend
   ```
2. Install dependencies:
   ```bash
   npm install
   ```
3. Start the development server (Network accessible):
   ```bash
   npm run dev -- --host
   ```
4. Access the app:
   - **On Computer**: [http://localhost:5173](http://localhost:5173)
   - **On Mobile**: Use the Network URL shown in the terminal (e.g., `http://192.168.1.X:5173`) while connected to the same WiFi.
     - *iOS Users*: Open in **Safari**, tap **Share** > **Add to Home Screen** for the full PWA experience.
     - *For Provisioning*: Use the **Bluefy** browser on iOS (Safari does not support Web Bluetooth).

## ESP32 Firmware Setup

Instructions for hardware developers setting up the nodes.

### 1. Configuration - Critical Step
The ESP32 needs to know where your MQTT Broker (backend) is running.

1. Find your computer's local IP address (e.g., run `ifconfig` or `ipconfig`).
2. Open `main_esp/main/app_mqtt.cpp`.
3. Locate the `BROKER_URI` definition (Line ~18).
4. Update the IP address to match your computer:
   ```cpp
   // Replace 192.168.X.X with your actual computer IP
   #define BROKER_URI "mqtt://192.168.18.30:1883" 
   ```

### 2. Build & Flash
Use the **VS Code ESP-IDF Extension**:
1. Connect the ESP32 via USB.
2. Open the Command Palette (`Cmd+Shift+P` / `Ctrl+Shift+P`).
3. Run `ESP-IDF: Build, Flash and Monitor`.
4. Select your device port and target (e.g., `esp32` or `esp32s3`).

### 3. Provisioning (Connecting to WiFi)
New or reset devices start in **Provisioning Mode** (Blue LED blinking). They broadcast a Bluetooth Low Energy (BLE) signal.

1. Ensure the backend and frontend are running.
2. Open the Frontend App on a Bluetooth-capable browser (**Chrome** on Android/Desktop, **Bluefy** on iOS).
3. Navigate to the "Setup WiFi" page.
4. Click "Start Scanning" and select the device (e.g., `PROV_...`).
5. Enter your WiFi credentials to transmit them to the ESP32.
6. The device will restart, connect to your WiFi, and the backend will verify it comes online.

## Backend API

For detailed API usage, including request/response schemas, visit the Swagger UI once the application is running locally
at:
[http://localhost:8080/api/swagger-ui/index.html](http://localhost:8080/api/swagger-ui/index.html)
