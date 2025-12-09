#include <sys/time.h>
#include "esp_log.h"
#include "esp_sntp.h"

#include "app_sntp.h"
#include "app_common.h"

static const char *TAG = "app_sntp";

static void time_sync_notification_cb(struct timeval *tv)
{
    ESP_LOGI(TAG, "Time synchronized!");
    xEventGroupSetBits(s_app_event_group, TIME_SYNCED_BIT);
}

static void initialize_sntp(void)
{
    ESP_LOGI(TAG, "Initializing SNTP for time sync...");
    esp_sntp_setoperatingmode(SNTP_OPMODE_POLL);
    esp_sntp_setservername(0, "pool.ntp.org");
    esp_sntp_set_time_sync_notification_cb(time_sync_notification_cb);
    esp_sntp_init();
}

void app_sntp_init(void)
{
    initialize_sntp();
}

void app_sntp_stop(void)
{
    esp_sntp_stop();
}
