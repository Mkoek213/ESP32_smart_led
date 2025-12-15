#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/i2c_master.h"
#include "bmp280.h"

static const char *TAG = "BMP280";

// BMP280 register addresses
#define BMP280_REG_TEMP_MSB     0xFA
#define BMP280_REG_TEMP_LSB     0xFB
#define BMP280_REG_TEMP_xLSB    0xFC

#define BMP280_REG_PRESS_MSB    0xF7
#define BMP280_REG_PRESS_LSB    0xF8
#define BMP280_REG_PRESS_xLSB   0xF9

#define BMP280_REG_CTRL_MEAS    0xF4
#define BMP280_REG_CONFIG       0xF5
#define BMP280_REG_ID           0xD0

// Calibration values registers
#define BMP280_REG_DIG_T1       0x88
#define BMP280_REG_DIG_T2       0x8A
#define BMP280_REG_DIG_T3       0x8C

#define BMP280_REG_DIG_P1       0x8E
#define BMP280_REG_DIG_P2       0x90
#define BMP280_REG_DIG_P3       0x92
#define BMP280_REG_DIG_P4       0x94
#define BMP280_REG_DIG_P5       0x96
#define BMP280_REG_DIG_P6       0x98
#define BMP280_REG_DIG_P7       0x9A
#define BMP280_REG_DIG_P8       0x9C
#define BMP280_REG_DIG_P9       0x9E

#define BMP280_TIMEOUT_MS       1000

typedef struct bmp280_dev_t {
    i2c_master_dev_handle_t i2c_dev;
    
    // Calibration data
    uint16_t dig_T1;
    int16_t dig_T2;
    int16_t dig_T3;
    uint16_t dig_P1;
    int16_t dig_P2;
    int16_t dig_P3;
    int16_t dig_P4;
    int16_t dig_P5;
    int16_t dig_P6;
    int16_t dig_P7;
    int16_t dig_P8;
    int16_t dig_P9;

    int32_t t_fine;
} bmp280_dev_t;

static esp_err_t bmp280_read_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t *data, size_t len)
{
    return i2c_master_transmit_receive(dev_handle,
        &reg_addr,
        1,
        data,
        len,
        BMP280_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t bmp280_write_reg(i2c_master_dev_handle_t dev_handle, uint8_t reg_addr, uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle,
         write_buf,
         sizeof(write_buf),
         BMP280_TIMEOUT_MS / portTICK_PERIOD_MS);
}

static esp_err_t bmp280_read_calibration_data(bmp280_handle_t dev)
{
    esp_err_t ret;
    
    // Read temperature calibration
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_T1, (uint8_t *)&dev->dig_T1, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_T2, (uint8_t *)&dev->dig_T2, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_T3, (uint8_t *)&dev->dig_T3, 2);
    if (ret != ESP_OK) return ret;

    // Read pressure calibration
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P1, (uint8_t *)&dev->dig_P1, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P2, (uint8_t *)&dev->dig_P2, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P3, (uint8_t *)&dev->dig_P3, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P4, (uint8_t *)&dev->dig_P4, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P5, (uint8_t *)&dev->dig_P5, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P6, (uint8_t *)&dev->dig_P6, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P7, (uint8_t *)&dev->dig_P7, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P8, (uint8_t *)&dev->dig_P8, 2);
    if (ret != ESP_OK) return ret;
    ret = bmp280_read_reg(dev->i2c_dev, BMP280_REG_DIG_P9, (uint8_t *)&dev->dig_P9, 2);
    if (ret != ESP_OK) return ret;

    ESP_LOGD(TAG, "Calibration data read successfully");
    return ESP_OK;
}

esp_err_t bmp280_init(i2c_master_bus_handle_t bus_handle, const bmp280_config_t *config, bmp280_handle_t *ret_handle)
{
    if (bus_handle == NULL || config == NULL || ret_handle == NULL) {
        return ESP_ERR_INVALID_ARG;
    }

    bmp280_handle_t dev = (bmp280_handle_t)calloc(1, sizeof(bmp280_dev_t));
    if (dev == NULL) {
        return ESP_ERR_NO_MEM;
    }

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = config->i2c_address,
        .scl_speed_hz = config->scl_speed_hz,
    };

    esp_err_t ret = i2c_master_bus_add_device(bus_handle, &dev_config, &dev->i2c_dev);
    if (ret != ESP_OK) {
        free(dev);
        return ret;
    }

    // Initialize with default configuration
    uint8_t osrs_t = 1; 
    uint8_t osrs_p = 1; 
    uint8_t mode = 3; 
    uint8_t ctrl_meas_reg = (osrs_t << 5) | (osrs_p << 2) | mode;

    // config: t_sb=5, filter=0, spi3w_en=0
    uint8_t t_sb = 5; 
    uint8_t filter = 0; 
    uint8_t spi3w_en = 0; 
    uint8_t config_reg = (t_sb << 5) | (filter << 2) | spi3w_en;

    ret = bmp280_write_reg(dev->i2c_dev, BMP280_REG_CTRL_MEAS, ctrl_meas_reg);
    if (ret != ESP_OK) goto err;

    ret = bmp280_write_reg(dev->i2c_dev, BMP280_REG_CONFIG, config_reg);
    if (ret != ESP_OK) goto err;

    ret = bmp280_read_calibration_data(dev);
    if (ret != ESP_OK) goto err;

    *ret_handle = dev;
    return ESP_OK;

err:
    if (dev->i2c_dev) {
        i2c_master_bus_rm_device(dev->i2c_dev);
    }
    free(dev);
    return ret;
}

static double bmp280_compensate_T_double(bmp280_handle_t dev, int32_t adc_T) {
    double var1, var2, T;
    var1 = (((double) adc_T) / 16384.0 - ((double) dev->dig_T1) / 1024.0) * ((double) dev->dig_T2);
    var2 = ((((double) adc_T) / 131072.0 - ((double) dev->dig_T1) / 8192.0) *
            (((double) adc_T) / 131072.0 - ((double) dev->dig_T1) / 8192.0)) * ((double) dev->dig_T3);
    dev->t_fine = (int32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0;
    return T;
}

static double bmp280_compensate_P_double(bmp280_handle_t dev, int32_t adc_P) {
    double var1, var2, p;
    var1 = ((double)dev->t_fine/2.0) - 64000.0;
    var2 = var1 * var1 * ((double)dev->dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)dev->dig_P5) * 2.0;
    var2 = (var2/4.0)+(((double)dev->dig_P4) * 65536.0);
    var1 = (((double)dev->dig_P3) * var1 * var1 / 524288.0 + ((double)dev->dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0)*((double)dev->dig_P1);
    
    if (var1 == 0.0) {
        return 0; // Avoid division by zero
    }

    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)dev->dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)dev->dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)dev->dig_P7)) / 16.0;
    return p;
}

esp_err_t bmp280_read_temp_pressure(bmp280_handle_t handle, float *temp, float *press)
{
    if (handle == NULL) return ESP_ERR_INVALID_ARG;

    esp_err_t ret;
    uint32_t temp_raw, press_raw;

    // Read temp
    uint8_t temp_buf[3];
    ret = bmp280_read_reg(handle->i2c_dev, BMP280_REG_TEMP_MSB, temp_buf, 3);
    if (ret != ESP_OK) return ret;
    temp_raw = (temp_buf[0] << 12) | (temp_buf[1] << 4) | (temp_buf[2] >> 4);

    // Read pressure
    uint8_t press_buf[3];
    ret = bmp280_read_reg(handle->i2c_dev, BMP280_REG_PRESS_MSB, press_buf, 3);
    if (ret != ESP_OK) return ret;
    press_raw = (press_buf[0] << 12) | (press_buf[1] << 4) | (press_buf[2] >> 4);

    double t = bmp280_compensate_T_double(handle, temp_raw);
    double p = bmp280_compensate_P_double(handle, press_raw);

    if (temp) *temp = (float)t;
    if (press) *press = (float)(p / 100.0); // Convert Pa to hPa

    return ESP_OK;
}

esp_err_t bmp280_read_temperature(bmp280_handle_t handle, float *temp)
{
    return bmp280_read_temp_pressure(handle, temp, NULL);
}

esp_err_t bmp280_read_pressure(bmp280_handle_t handle, float *press)
{
    return bmp280_read_temp_pressure(handle, NULL, press);
}

esp_err_t bmp280_delete(bmp280_handle_t handle)
{
    if (handle == NULL) return ESP_ERR_INVALID_ARG;
    
    if (handle->i2c_dev) {
        i2c_master_bus_rm_device(handle->i2c_dev);
    }
    free(handle);
    return ESP_OK;
}

