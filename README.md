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

## Backend API

For detailed API usage, including request/response schemas, visit the Swagger UI once the application is running locally
at:
[http://localhost:8080/api/swagger-ui/index.html](http://localhost:8080/api/swagger-ui/index.html)
