idf_component_register(SRCS "main.cpp"
                           "led_controller.cpp"
                           "wifi_station.cpp"
                           "wifi_config.cpp"
                           "ble_provisioning.cpp"
                           "app_common.c"
                           "app_mqtt.cpp"
                           "sensor_manager.cpp"
                           "sensor_task.cpp"
                           "app_sntp.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio esp_netif esp_timer bt mqtt esp_pm
                    INCLUDE_DIRS ".")
