#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include "esp_err.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Current firmware version (update this with each release)
#define FIRMWARE_VERSION "25.0"

// OTA Configuration
#define OTA_CHECK_INTERVAL_MS (60000) // Check every 1 minute (dev)
#define OTA_RECV_TIMEOUT_MS (5000)    // HTTP receive timeout

// Initialize OTA update system
esp_err_t ota_update_init(void);

// Check for available firmware updates from backend
// Returns true if update is available and downloaded successfully
bool ota_check_and_update(const char *backend_url, const char *device_mac);

// Perform OTA update from a given URL
esp_err_t ota_update_from_url(const char *url);

// Get current firmware version
const char *ota_get_current_version(void);

// Start background task that periodically checks for updates
void ota_start_update_task(const char *backend_url);

// Mark current boot as valid (call after successful boot)
void ota_mark_boot_valid(void);

#ifdef __cplusplus
}
#endif

#endif // OTA_UPDATE_H
