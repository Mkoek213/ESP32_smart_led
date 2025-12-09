idf_component_register(SRCS "main.cpp"
                           "led_controller.cpp"
                           "wifi_station.cpp"
                           "wifi_config.cpp"
                           "ble_provisioning.cpp"
                           "app_common.c"
                           "app_mqtt.c"
                           "app_sntp.c"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio esp_netif esp_timer bt mqtt
                    INCLUDE_DIRS ".")
