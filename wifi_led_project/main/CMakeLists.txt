idf_component_register(SRCS "main.cpp"
                           "led_controller.cpp"
                           "wifi_station.cpp"
                           "wifi_config.cpp"
                           "ble_provisioning.cpp"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio esp_netif esp_timer bt
                    INCLUDE_DIRS ".")
