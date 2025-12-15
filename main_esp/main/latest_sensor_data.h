#ifndef LATEST_SENSOR_DATA_H
#define LATEST_SENSOR_DATA_H

/**
 * @brief Thread-safe cache for latest BLE sensor readings
 * 
 * Stores the most recent temperature and humidity from BLE sensor
 * so they're always available for LED color selection, even when
 * the SensorManager queue is empty.
 */
class LatestSensorData {
public:
    /**
     * @brief Initialize the sensor data cache
     */
    static void init();
    
    /**
     * @brief Update with new sensor readings (thread-safe)
     * @param temp Temperature in Celsius
     * @param humid Humidity in percentage
     */
    static void update(float temp, float humid);
    
    /**
     * @brief Get latest temperature (thread-safe)
     * @return Temperature in Celsius, or 22.0 if never updated
     */
    static float get_temperature();
    
    /**
     * @brief Get latest humidity (thread-safe)
     * @return Humidity in percentage, or 50.0 if never updated
     */
    static float get_humidity();
    
    /**
     * @brief Check if data has been received at least once
     * @return true if update() has been called, false otherwise
     */
    static bool has_data();

private:
    static void* mutex;  // SemaphoreHandle_t
    static float temperature;
    static float humidity;
    static bool data_available;
};

#endif // LATEST_SENSOR_DATA_H
