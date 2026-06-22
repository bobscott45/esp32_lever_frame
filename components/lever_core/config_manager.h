/*
 * This file is part of esp32_lever_frame.
 *
 * esp32_lever_frame is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * esp32_lever_frame is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with esp32_lever_frame.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file      config_manager.h
 * @brief     Definitions for config_manager.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <esp_err.h>
#include "interlocking.h"

/**
 * @brief  Initialize the configuration manager.
 *
 * This function initializes the Non-Volatile Storage (NVS) and loads 
 * the saved configuration string. If no configuration is saved or if 
 * an error occurs, it falls back to the default compile-time configuration.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_FAIL on general failure
 */
esp_err_t config_manager_init(void);

/**
 * @brief  Get the currently active configuration.
 *
 * This function returns a pointer to the active lever system configuration, 
 * which may be dynamically allocated from NVS or point to the default config.
 * 
 * @return 
 *   - Pointer to the active config structure
 */
const lever_system_config_t *config_manager_get_current(void);

/**
 * @brief  Get the fingerprint hash of the active configuration.
 *
 * Returns a 32-bit hash of the current JSON configuration string, 
 * which can be used to detect changes or mismatches.
 * 
 * @return 
 *   - 32-bit hash value
 */
uint32_t config_manager_get_hash(void);

/**
 * @brief  Save a new JSON configuration to NVS.
 *
 * Parses the provided JSON string, validates it, and saves it to NVS. 
 * If valid, it swaps the active configuration to the newly parsed one 
 * and notifies the system of a configuration reload.
 *
 * @param[in]  json_str   The raw JSON string representing the new configuration.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t config_manager_save_json(const char *json_str);

/**
 * @brief  Get the JSON representation of the current configuration.
 *
 * Reads the active configuration JSON directly from NVS, or generates 
 * it dynamically from the default configuration if using the default.
 * The caller is responsible for freeing the returned string.
 * 
 * @return 
 *   - Heap-allocated string containing JSON
 *   - NULL on failure
 */
char *config_manager_get_json_str(void);

/**
 * @brief  Free any pending configuration memory.
 *
 * Releases memory for any dynamically allocated configuration that 
 * was replaced by a newer configuration and is waiting to be freed.
 */
void config_manager_free_pending(void);

/**
 * @brief  Clean up allocated configuration resources.
 *
 * Frees the currently active dynamic configuration if one is loaded, 
 * and cleans up any pending free configurations.
 */
void config_manager_deinit(void);

/**
 * @brief  Update a boolean setting globally.
 *
 * Edits the current configuration JSON to update a global boolean setting 
 * and saves the changes to NVS without triggering a full system reload notify.
 *
 * @param[in]  key     The JSON key to update.
 * @param[in]  value   The new boolean value.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t config_manager_update_global_bool(const char *key, bool value);

/**
 * @brief  Update an integer setting globally.
 *
 * Edits the current configuration JSON to update a global integer setting 
 * and saves the changes to NVS without triggering a full system reload notify.
 *
 * @param[in]  key     The JSON key to update.
 * @param[in]  value   The new integer value.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t config_manager_update_global_int(const char *key, int value);

/**
 * @brief  Update a boolean setting for a specific lever.
 *
 * Edits the current configuration JSON to update a boolean setting for 
 * a specific lever and saves the changes to NVS.
 *
 * @param[in]  tab_idx     The index of the tab containing the lever.
 * @param[in]  lever_idx   The index of the lever within the tab.
 * @param[in]  key         The JSON key to update.
 * @param[in]  value       The new boolean value.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_ERR_NO_MEM if memory allocation fails
 */
esp_err_t config_manager_update_lever_bool(int tab_idx, int lever_idx, const char *key, bool value);

/**
 * @brief  Convert a lever type enum to its string representation.
 *
 * Returns a human-readable string corresponding to the provided lever type.
 *
 * @param[in]  type   The lever type to convert.
 * 
 * @return 
 *   - String representation of the lever type
 */
const char *lever_type_to_str(lever_type_t type);

/**
 * @brief  Convert a string to its corresponding lever type enum.
 *
 * Parses the provided string and returns the matching lever type enum. 
 * If the string does not match any known type, it returns LEVER_TYPE_SPARE.
 *
 * @param[in]  str   The string to parse.
 * 
 * @return 
 *   - The corresponding lever type enum
 */
lever_type_t str_to_lever_type(const char *str);

#endif // CONFIG_MANAGER_H
