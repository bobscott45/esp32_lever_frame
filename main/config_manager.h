#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <esp_err.h>
#include "lever_frame.h"

typedef void (*config_change_cb_t)(void);

/**
 * @brief Initialize the configuration manager, load saved configuration from NVS,
 *        or fall back to the default config if none is saved.
 * @return ESP_OK on success
 */
esp_err_t config_manager_init(void);

/**
 * @brief Get the currently active lever system configuration.
 * @return Pointer to the active config structure (static or dynamic)
 */
const lever_system_config_t *config_manager_get_current(void);

/**
 * @brief Save a new JSON configuration string to NVS and apply it.
 * @param json_str The raw JSON string
 * @return ESP_OK on success, or error code
 */
esp_err_t config_manager_save_json(const char *json_str);

/**
 * @brief Get the JSON representation of the current configuration.
 * @return Heap-allocated string containing JSON, or NULL. Caller must free().
 */
char *config_manager_get_json_str(void);

/**
 * @brief Set a callback to be triggered when the configuration is updated.
 * @param cb The callback function
 */
void config_manager_set_on_change(config_change_cb_t cb);

/**
 * @brief Clean up allocated configuration resources.
 */
void config_manager_deinit(void);

// Helper conversions
const char *lever_type_to_str(lever_type_t type);
lever_type_t str_to_lever_type(const char *str);

#endif // CONFIG_MANAGER_H
