#ifndef APP_COMMON_H
#define APP_COMMON_H

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"

extern EventGroupHandle_t s_app_event_group;

#define WIFI_CONNECTED_BIT BIT0
#define TIME_SYNCED_BIT BIT1
#define BLE_STACK_READY_BIT BIT2

#endif
