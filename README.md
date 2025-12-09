# ESP32_smart_led

Smart Lighting System with Dual ESP32 – The first ESP32 reads humidity, light, and motion sensors, sending data to the
second ESP32 that controls RGB LEDs. LED color and brightness adapt to sensor data. A mobile app shows stats and lets
users customize lighting.

## Backend

### Mosquitto MQTT Broker

There are two ways to run the mosquitto mqtt broker.

#### Docker

To run the mosquitto mqtt broker, you can use docker.

```bash
docker run -it -p 1883:1883 -p 9001:9001 eclipse-mosquitto
```

#### Homebrew (macOS)

Install mosquitto using brew:

```bash
brew install mosquitto
```

Run mosquitto as a service:

```bash
brew services start mosquitto
```

### Example Mosquitto configuration

An example configuration is provided at `mosquitto.conf.example` in the project root. Use it to run a broker with the project's recommended listeners and options.

- Local mosquitto (macOS/Homebrew): run mosquitto with the example config (stops the service first if running):

```bash
brew services stop mosquitto
mosquitto -c ./mosquitto.conf.example
```

To run the example config as a service, copy it to Homebrew's mosquitto config directory (path may vary) and start the service:

```bash
# adjust target path if Homebrew is installed in /usr/local
cp mosquitto.conf.example /opt/homebrew/etc/mosquitto/mosquitto.conf
brew services start mosquitto
```

Note: verify or adjust the ports and websocket settings inside `mosquitto.conf.example` to match your environment (standard MQTT port 1883 and websocket port 9001 are commonly used).

### Password file (mosquitto.passwd)

To enable username/password authentication, create a password file in the same directory as your `mosquitto.conf` (for example `mosquitto.passwd`). Use `mosquitto_passwd` to create or add users and then ensure your config references the file (`password_file mosquitto.passwd`) and disables anonymous access (`allow_anonymous false`).

Create a new password file and add a user:

```bash
# create new passwd file and add user 'user' (you'll be prompted for a password)
mosquitto_passwd -c mosquitto.passwd user
```

```
password_file mosquitto.passwd
allow_anonymous false
```

### Database

Before running the application, you need to start the PostgreSQL database. The project is configured to use
docker-compose for this.

```bash
docker-compose up -d
```

### Running the service

To run the service you can use the following command:

```bash
./mvnw spring-boot:run
```

### Testing the service

To test the service you can publish mock data to the broker. The subscribed topic for the sensor-data is
`customer/{customerId}/location/{locationId}/device/{deviceId}/sensor-data`.

```bash
mosquitto_pub -h localhost -p 1883 -t "customer/1/location/1/device/1/sensor-data" -m '[{"temperature": 25.5, "humidity": 50.5, "light": 500, "motion": true}]'
```

This will publish a message to the broker with the topic `customer/1/location/1/device/1/sensor-data` and the given
payload. The service
will then receive the message and process it.
