#ifndef APP_MQTT_H
#define APP_MQTT_H

#ifdef __cplusplus
extern "C" {
#endif

void app_mqtt_init(void);

void app_mqtt_start(void);

void app_mqtt_stop(void);

void app_mqtt_start_publishing_task(void);

#ifdef __cplusplus
}
#endif

#endif
