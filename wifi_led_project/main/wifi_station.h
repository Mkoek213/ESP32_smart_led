#ifndef WIFI_STATION_H
#define WIFI_STATION_H

#include "led_controller.h"

void wifi_station_start(LEDController* led);
bool wifi_station_is_connected();

#endif

