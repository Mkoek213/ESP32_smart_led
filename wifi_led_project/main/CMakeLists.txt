idf_component_register(SRCS "main.cpp"
                           "led_controller.cpp"
                           "wifi_station.cpp"
                           "http_client.cpp"
                    PRIV_REQUIRES esp_wifi nvs_flash esp_driver_gpio esp_netif
                    INCLUDE_DIRS ".")
