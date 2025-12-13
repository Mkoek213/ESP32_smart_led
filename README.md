# ESP32_smart_led

Smart Lighting System with Dual ESP32 – The first ESP32 reads humidity, light, and motion sensors, sending data to the
second ESP32 that controls RGB LEDs. LED color and brightness adapt to sensor data. A mobile app shows stats and lets
users customize lighting.

## Backend

The backend consists of a Spring Boot application, a Mosquitto MQTT broker, and a PostgreSQL database. The entire stack
can be easily run using Docker Compose.

### Prerequisites

- Docker

### Running the Application with Docker Compose

To build and run all the services (backend, MQTT broker, and database), use the following command from the project root
directory:

```bash
docker compose up --build
```

This command will:

1. Build the Docker image for the backend service as defined in the `Dockerfile`.
2. Start the backend service, Mosquitto broker, and PostgreSQL database containers in the correct order.
3. The services will be networked together, allowing them to communicate.

To stop all the running services, you can press `Ctrl+C` in the terminal where `docker compose` is running, or run
`docker compose down` from another terminal in the project root.

### API Documentation

The backend provides a Swagger UI for exploring and testing the REST API.
Once the application is running, you can access it at:
`http://localhost:8080/swagger-ui/index.html`

### MQTT Communication Protocol

The system uses MQTT for communication between the backend and ESP32 devices. The topic structure is hierarchical:
`customer/{customerId}/location/{locationId}/device/{deviceId}/{type}`

#### Topic Types

| Type          | Topic Suffix | Direction         | Description                |
|:--------------|:-------------|:------------------|:---------------------------|
| **Telemetry** | `/telemetry` | Device -> Backend | Sensor data readings       |
| **Status**    | `/status`    | Device -> Backend | Device connectivity status |
| **Command**   | `/cmd`       | Backend -> Device | Control commands           |

#### Payload Formats

**Telemetry** (`/telemetry`)

Sent as a JSON Array containing batch of sensor readings.

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

**Status** (`/status`)

Sent as a JSON Object indicating the device's connection state.

```json
{
  "state": "online"
}
```

*Possible values: "online", "offline"*

**Command** (`/cmd`)

Sent as a JSON Object. The `payload` field content depends on the command `type`.

```json
{
  "type": "COMMAND_TYPE",
  "payload": {
    // Command-specific parameters
  }
}
```

#### Workflow

1. **Telemetry**: Devices buffer sensor readings and publish them in batches to the `/telemetry` topic. The backend
   saves these to the database.
2. **Status**: Devices publish a retained message to `/status` upon connection (`online`) and use LWT (Last Will and
   Testament) for disconnection (`offline`).
3. **Commands**: The backend publishes commands to `/cmd`. Devices subscribe to this topic to receive instructions (
   e.g., change LED color).
