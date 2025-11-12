idf_component_register(SRCS "app_common.c" "app_sntp.c" "app_mqtt.c" "app_wifi.c" "app_main.c"
                    PRIV_REQUIRES mqtt nvs_flash esp_netif esp_wifi
                    INCLUDE_DIRS ".")
