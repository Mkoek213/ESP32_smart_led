#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include <unistd.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_log.h"
#include "driver/i2c_master.h"

static const char *TAG = "I2C";


#define I2C_MASTER_SCL_IO           22  // gpio number of SCL
#define I2C_MASTER_SDA_IO           21  // gpio number of SDA

// I2C port number, avaiable: 0, 1. We have one device so doesnt matter
#define I2C_MASTER_NUM              I2C_NUM_1
#define I2C_MASTER_FREQ_HZ          100000  // I2C clock frequency, 100kHz - safe (up to 3.4MHz)
#define I2C_MASTER_TIMEOUT_MS       0x3000  // 12 sec timeout, if device is not responding throw error

// BMP280 register addresses
#define BMP280_I2C_ADDRESS          0x76     // BMP280 address with SD0 connected to GND

// P. 24 memory map
// data registers
#define BMP280_REG_TEMP_MSB   		0xFA
#define BMP280_REG_TEMP_LSB   		0xFB
#define BMP280_REG_TEMP_xLSB  		0xFC

#define BMP280_REG_PRESS_MSB        0xF7
#define BMP280_REG_PRESS_LSB        0xF8
#define BMP280_REG_PRESS_xLSB       0xF9

// control registers
#define BMP280_REG_CTRL_MEAS  		0xF4 // mode and oversampling settings
#define BMP280_REG_CONFIG     		0xF5 // standby time
#define BMP280_REG_ID         		0xD0 // can check if the device is connected

// number of bytes to read (helper)
#define BMP280_TEMP_REG_SIZE  		3
#define BMP280_PRESS_REG_SIZE       3

// P. 21 
// calibration values
#define dig_T1_R					0x88
#define dig_T2_R					0x8A
#define dig_T3_R					0x8C

#define dig_P1_R                    0x8E
#define dig_P2_R                    0x90
#define dig_P3_R                    0x92
#define dig_P4_R                    0x94
#define dig_P5_R                    0x96
#define dig_P6_R                    0x98
#define dig_P7_R                    0x9A
#define dig_P8_R                    0x9C
#define dig_P9_R                    0x9E

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

int32_t t_fine; // fine temperature used in pressure calculation

static void i2c_master_init(i2c_master_bus_handle_t *bus_handle, i2c_master_dev_handle_t *dev_handle)
{
    i2c_master_bus_config_t bus_config = {
        .i2c_port = I2C_MASTER_NUM, // I2C port number
        .sda_io_num = I2C_MASTER_SDA_IO, // SDA gpio number
        .scl_io_num = I2C_MASTER_SCL_IO, // SCL gpio number
        .clk_source = I2C_CLK_SRC_DEFAULT, // I2C clock source
        .glitch_ignore_cnt = 7, // glitch ignore count
        .flags.enable_internal_pullup = true, // enable internal pull-up
    };
    // initialize I2C master bus
    ESP_ERROR_CHECK(i2c_new_master_bus(&bus_config, bus_handle));

    i2c_device_config_t dev_config = {
        .dev_addr_length = I2C_ADDR_BIT_LEN_7,
        .device_address = BMP280_I2C_ADDRESS,
        .scl_speed_hz = I2C_MASTER_FREQ_HZ,
    };
    // add I2C master device
    ESP_ERROR_CHECK(i2c_master_bus_add_device(*bus_handle, &dev_config, dev_handle));
}

static esp_err_t bmp280_register_read(i2c_master_dev_handle_t dev_handle,
    uint8_t reg_addr,
    uint8_t *data,
    size_t len)
{
    return i2c_master_transmit_receive(dev_handle,
        &reg_addr, // register address
        1, // number of bytes to read
        data, // data buffer
        len, // number of bytes to read
        I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS); // timeout
}

static esp_err_t bmp280_register_write_byte(i2c_master_dev_handle_t dev_handle,
     uint8_t reg_addr,
     uint8_t data)
{
    uint8_t write_buf[2] = {reg_addr, data};
    return i2c_master_transmit(dev_handle,
         write_buf,
         sizeof(write_buf),
         I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS); // timeout
}

// P. 22 calculating pressure and temperature
double bmp280_compensate_T_double(int32_t adc_T) {
    double var1, var2, T;
    var1 = (((double) adc_T) / 16384.0 - ((double) dig_T1) / 1024.0)
            * ((double) dig_T2);
    var2 = ((((double) adc_T) / 131072.0 - ((double) dig_T1) / 8192.0)
            * (((double) adc_T) / 131072.0 - ((double) dig_T1) / 8192.0))
            * ((double) dig_T3);
    t_fine = (int32_t)(var1 + var2);
    T = (var1 + var2) / 5120.0;
    return T;
}

double bmp280_compensate_P_double(int32_t adc_P) {
    double var1, var2, p;
    var1 = ((double)t_fine/2.0) - 64000.0;
    var2 = var1 * var1 * ((double)dig_P6) / 32768.0;
    var2 = var2 + var1 * ((double)dig_P5) * 2.0;
    var2 = (var2/4.0)+(((double)dig_P4) * 65536.0);
    var1 = (((double)dig_P3) * var1 * var1 / 524288.0 + ((double)dig_P2) * var1) / 524288.0;
    var1 = (1.0 + var1 / 32768.0)*((double)dig_P1);
    p = 1048576.0 - (double)adc_P;
    p = (p - (var2 / 4096.0)) * 6250.0 / var1;
    var1 = ((double)dig_P9) * p * p / 2147483648.0;
    var2 = p * ((double)dig_P8) / 32768.0;
    p = p + (var1 + var2 + ((double)dig_P7)) / 16.0;
    return p;
}

void app_main(void)
{
	uint8_t buffer[BMP280_TEMP_REG_SIZE]; // data buffer for reading 3 bytes
	uint32_t temp_raw; // temperature raw data (3 bytes combined from buffer)
    uint32_t press_raw; // pressure raw data (3 bytes combined from buffer)

	// setting up I2C devices
	i2c_master_bus_handle_t bus_handle;
    i2c_master_dev_handle_t dev_handle;
    i2c_master_init(&bus_handle, &dev_handle);
    ESP_LOGI(TAG, "I2C init successful");

	// Configuration specific for BMP280
    // ctrl meas
    uint8_t osrs_t = 1; // temperature oversampling x 1 (fast, low acc) p. 26
    uint8_t osrs_p = 1; // pressure oversampling x 1 (fast, low acc) p. 25
    uint8_t mode = 3; // normal mode (0 - sleep, 1 - forced, 3 - normal)

    // config
    uint8_t t_sb = 5; // tstandby 1000ms
    uint8_t filter = 0; // filter off
    uint8_t spi3w_en = 0; // 3-wire SPI Disable

    uint8_t ctrl_meas_reg = (osrs_t << 5) | (osrs_p << 2) | mode;
    uint8_t config_reg = (t_sb << 5) | (filter << 2) | spi3w_en;
    
    // write to registers
    ESP_ERROR_CHECK(bmp280_register_write_byte(dev_handle, BMP280_REG_CTRL_MEAS, ctrl_meas_reg));
    ESP_ERROR_CHECK(bmp280_register_write_byte(dev_handle, BMP280_REG_CONFIG, config_reg));

    // read calibration data for temp
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_T1_R, (uint8_t *)(&dig_T1), sizeof(dig_T1)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_T2_R, (uint8_t *)(&dig_T2), sizeof(dig_T2)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_T3_R, (uint8_t *)(&dig_T3), sizeof(dig_T3)));
    ESP_LOGI(TAG, "Calibration data temperature: \n%d\n%d\n%d\n", dig_T1, dig_T2, dig_T3);

    // read calibration data for pressure
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P1_R, (uint8_t *)(&dig_P1), sizeof(dig_P1)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P2_R, (uint8_t *)(&dig_P2), sizeof(dig_P2)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P3_R, (uint8_t *)(&dig_P3), sizeof(dig_P3)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P4_R, (uint8_t *)(&dig_P4), sizeof(dig_P4)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P5_R, (uint8_t *)(&dig_P5), sizeof(dig_P5)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P6_R, (uint8_t *)(&dig_P6), sizeof(dig_P6)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P7_R, (uint8_t *)(&dig_P7), sizeof(dig_P7)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P8_R, (uint8_t *)(&dig_P8), sizeof(dig_P8)));
    ESP_ERROR_CHECK(bmp280_register_read(dev_handle, dig_P9_R, (uint8_t *)(&dig_P9), sizeof(dig_P9)));
    ESP_LOGI(TAG, "Calibration data pressure: \n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n%d\n", dig_P1, dig_P2, dig_P3, dig_P4, dig_P5, dig_P6, dig_P7, dig_P8, dig_P9);

    while (true) {
        // reading temp
		ESP_ERROR_CHECK(bmp280_register_read( dev_handle, BMP280_REG_TEMP_MSB, buffer, BMP280_TEMP_REG_SIZE));
        // combining 3 bytes into one int
		temp_raw = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);
        
        // reading pressure
        ESP_ERROR_CHECK(bmp280_register_read(dev_handle, BMP280_REG_PRESS_MSB, buffer, BMP280_PRESS_REG_SIZE));
        // combining 3 bytes into one int
        press_raw = (buffer[0] << 12) | (buffer[1] << 4) | (buffer[2] >> 4);

        // calculating actual values
        double temperature_actual = bmp280_compensate_T_double(temp_raw);
        double pressure_actual = bmp280_compensate_P_double(press_raw);

        ESP_LOGI(TAG, "Temperature: %.2f (+/- 0.1) C | Pressure: %.2f (+/- 1) hPa", temperature_actual, pressure_actual/100.0);
		sleep(1);
    }
}
