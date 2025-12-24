#ifndef SENSOR_TASK_H
#define SENSOR_TASK_H

#include "bmp280.h"

void sensor_reading_task_start(bmp280_handle_t bmp_handle);

#endif // SENSOR_TASK_H
