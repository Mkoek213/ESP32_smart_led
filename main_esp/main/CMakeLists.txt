idf_component_register(SRCS "latest_sensor_data.cpp" "person_counter.cpp" "bmp280.c" "motion_detector.cpp" "led_config.cpp" "main.cpp"
                           "led_controller.cpp"
                           "ws2812b_controller.cpp"
                           "hc_sr04.cpp"
                           "wifi_station.cpp"
                           "wifi_config.cpp"
                           "ble_provisioning.cpp"
                           "app_common.c"
                           "app_mqtt.cpp"
                           "bmp280.c"
                           "sensor_manager.cpp"
                           "sensor_task.cpp"
                           "app_sntp.c"
                    REQUIRES esp_driver_gpio esp_driver_rmt esp_driver_gptimer led_strip esp_driver_i2c
                    PRIV_REQUIRES esp_wifi nvs_flash esp_netif esp_timer bt mqtt esp_pm json esp_adc
                    INCLUDE_DIRS ".")
