#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_err.h"

#include "config.h"
#include "led_controller.h"
#include "wifi_station.h"
#include "http_client.h"

extern "C" void app_main(void)
{
    ESP_ERROR_CHECK(nvs_flash_init());

    LEDController led(LED_GPIO, LED_BLINK_PERIOD_MS);
    wifi_station_start(&led);
    bool request_completed = false;
    while (true) {
        if (!wifi_station_is_connected()) {
            request_completed = false;
            vTaskDelay(pdMS_TO_TICKS(500));
            continue;
        }

        if (!request_completed) {
            bool success = false;
            while (!success && wifi_station_is_connected()) {
                success = http_client_get(HTTP_SERVER, HTTP_PORT, HTTP_PATH);
                if (!success) {
                    printf("Brak odpowiedzi od serwera\n");
                    vTaskDelay(pdMS_TO_TICKS(15000));
                }
            }

            if (success) {
                request_completed = true;
            }
            continue;
        }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

