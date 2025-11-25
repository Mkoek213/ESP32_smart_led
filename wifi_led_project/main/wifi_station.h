#ifndef WIFI_STATION_H
#define WIFI_STATION_H

#include "led_controller.h"
#include "wifi_config.h"

void wifi_station_start(LEDController* led, const WifiConfig* cfg);
bool wifi_station_is_connected();
void wifi_station_stop();

#endif

