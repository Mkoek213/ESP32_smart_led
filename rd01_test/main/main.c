#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "rd01_test";

/**
 * Rd-01 Configuration
 * TX Pin: GPIO 16 (Connect to Rd-01 TX)
 * RX Pin: GPIO 17 (Connect to Rd-01 RX) - Note: standard UART nomenclature is crossed, but ESP32 UART2 default is often referred to as RX=16, TX=17. 
 * HOWEVER, the previous chat said:
 * Rd-01 TX -> ESP32 RX (GPIO 16)
 * Rd-01 RX -> ESP32 TX (GPIO 17)
 * 
 * Baud Rate: 256000 (Default)
 */
// Reverting to RX2/TX2 (GPIO 16/17) since user confirmed signal activity there
#define RADAR_TXD_PIN (GPIO_NUM_17)
#define RADAR_RXD_PIN (GPIO_NUM_16)
#define RADAR_UART_PORT_NUM      (UART_NUM_2)
#define RADAR_BAUD_RATE          (115200)
#define BUF_SIZE (1024)

void app_main(void)
{
    ESP_LOGI(TAG, "Starting Rd-01 Radar Test"); 
    ESP_LOGI(TAG, "Configuring UART2: Baud=%d, RX=%d, TX=%d", 
             RADAR_BAUD_RATE, RADAR_RXD_PIN, RADAR_TXD_PIN);

    /* Configure parameters of an UART driver,
     * communication pins and install the driver */
    uart_config_t uart_config = {
        .baud_rate = RADAR_BAUD_RATE,
        .data_bits = UART_DATA_8_BITS,
        .parity    = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
        .source_clk = UART_SCLK_DEFAULT,
    };
    
    // Install UART driver, and get the queue.
    ESP_ERROR_CHECK(uart_driver_install(RADAR_UART_PORT_NUM, BUF_SIZE * 2, 0, 0, NULL, 0));
    ESP_ERROR_CHECK(uart_param_config(RADAR_UART_PORT_NUM, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(RADAR_UART_PORT_NUM, RADAR_TXD_PIN, RADAR_RXD_PIN, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));

    ESP_LOGI(TAG, "UART2 initialized. Listening for data...");
    ESP_LOGI(TAG, "NOTE: Rd-01 'CHIP_EN' pin should be left FLOATING (disconnected).");

    // Convert raw hex to string for printing
    uint8_t *data = (uint8_t *) malloc(BUF_SIZE);
    
    // Buffer for single byte reading
    uint8_t byte;
    
    while (1) {
        // Read 1 byte
        int len = uart_read_bytes(RADAR_UART_PORT_NUM, &byte, 1, 20 / portTICK_PERIOD_MS);
        
        if (len > 0) {
            // Check for common headers based on search results (0xAA or 0xF4)
            // The search mentioned 0xAA for intra-frame data.
            // Let's print everything but highlight AA
            
            if (byte == 0xAA) {
                 printf("\n[HEAD AA] ");
            } else if (byte == 0xF4) {
                 printf("\n[HEAD F4] ");
            }
            
            printf("%02X ", byte);
        } else {
             // Print heartbeat every ~5 seconds (250 * 20ms)
             static int idle_count = 0;
             if (++idle_count > 250) {
                 idle_count = 0;
                 ESP_LOGI(TAG, "Waiting for data...");
             }
        }
        
        // Small yield
        // vTaskDelay(1); 
    }
}
