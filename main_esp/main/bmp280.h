/*
 * BMP280 Driver for ESP-IDF
 *
 * This component provides an interface to communicate with the BMP280 sensor
 * using the ESP-IDF I2C master driver.
 */

#pragma once

#include "esp_err.h"
#include "driver/i2c_master.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief BMP280 device handle
 */
typedef struct bmp280_dev_t *bmp280_handle_t;

/**
 * @brief BMP280 configuration structure
 */
typedef struct {
    uint8_t i2c_address;
    uint32_t scl_speed_hz;
} bmp280_config_t;

/**
 * @brief Default configuration for BMP280
 */
#define BMP280_I2C_ADDRESS_DEFAULT  0x76
#define BMP280_SCL_SPEED_HZ_DEFAULT 100000

#define BMP280_CONFIG_DEFAULT() { \
    .i2c_address = BMP280_I2C_ADDRESS_DEFAULT, \
    .scl_speed_hz = BMP280_SCL_SPEED_HZ_DEFAULT, \
}

/**
 * @brief Initialize the BMP280 device and add it to the I2C bus
 *
 * @param[in] bus_handle Handle to the I2C master bus
 * @param[in] config Pointer to the configuration structure
 * @param[out] ret_handle Pointer to store the created device handle
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - ESP_ERR_NO_MEM: Memory allocation failed
 *     - Other error codes from I2C driver
 */
esp_err_t bmp280_init(i2c_master_bus_handle_t bus_handle, const bmp280_config_t *config, bmp280_handle_t *ret_handle);

/**
 * @brief Read temperature and pressure from the BMP280 sensor
 *
 * @param[in] handle Handle to the BMP280 device
 * @param[out] temp Pointer to store the temperature in degrees Celsius
 * @param[out] press Pointer to store the pressure in hPa
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - Other error codes from I2C driver
 */
esp_err_t bmp280_read_temp_pressure(bmp280_handle_t handle, float *temp, float *press);

/**
 * @brief Read only temperature from the BMP280 sensor
 *
 * @param[in] handle Handle to the BMP280 device
 * @param[out] temp Pointer to store the temperature in degrees Celsius
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - Other error codes from I2C driver
 */
esp_err_t bmp280_read_temperature(bmp280_handle_t handle, float *temp);

/**
 * @brief Read only pressure from the BMP280 sensor
 *
 * @param[in] handle Handle to the BMP280 device
 * @param[out] press Pointer to store the pressure in hPa
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 *     - Other error codes from I2C driver
 */
esp_err_t bmp280_read_pressure(bmp280_handle_t handle, float *press);

/**
 * @brief Deinitialize the BMP280 device and free resources
 *
 * @param[in] handle Handle to the BMP280 device
 * @return
 *     - ESP_OK: Success
 *     - ESP_ERR_INVALID_ARG: Invalid arguments
 */
esp_err_t bmp280_delete(bmp280_handle_t handle);

#ifdef __cplusplus
}
#endif

