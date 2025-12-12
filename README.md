# ESP32_smart_led

Smart Lighting System with Dual ESP32 – The first ESP32 reads humidity, light, and motion sensors, sending data to the
second ESP32 that controls RGB LEDs. LED color and brightness adapt to sensor data. A mobile app shows stats and lets
users customize lighting.

## Backend

The backend consists of a Spring Boot application, a Mosquitto MQTT broker, and a PostgreSQL database. The entire stack can be easily run using Docker Compose.

### Prerequisites

- Dockers

### Running the Application with Docker Compose

To build and run all the services (backend, MQTT broker, and database), use the following command from the project root directory:

```bash
docker compose up --build
```

This command will:
1.  Build the Docker image for the backend service as defined in the `Dockerfile`.
2.  Start the backend service, Mosquitto broker, and PostgreSQL database containers in the correct order.
3.  The services will be networked together, allowing them to communicate.

To stop all the running services, you can press `Ctrl+C` in the terminal where `docker compose` is running, or run `docker compose down` from another terminal in the project root.

