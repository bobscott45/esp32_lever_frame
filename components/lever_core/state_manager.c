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
 * @file      state_manager.c
 * @brief     Implementation of state_manager.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "state_manager.h"
#include "nvs.h"
#include "esp_log.h"
#include "controller.h"
#include <stdlib.h>

static const char *TAG = "StateManager";

void state_manager_load_and_apply(uint32_t current_config_hash) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No NVS or no saved state found. Booting with default safe state.");
        return;
    }
    
    uint32_t saved_hash = 0;
    err = nvs_get_u32(my_handle, "cfg_hash", &saved_hash);
    if (err != ESP_OK || saved_hash != current_config_hash) {
        ESP_LOGW(TAG, "Config hash mismatch (saved: %lu, current: %lu). Layout changed! Wiping state and booting to safe defaults.", saved_hash, current_config_hash);
        nvs_close(my_handle);
        return;
    }
    
    size_t required_size = 0;
    err = nvs_get_blob(my_handle, "lever_st_v2", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
        // Ensure the size matches what the controller expects
        if (required_size == controller_get_serialized_size()) {
            uint8_t *states = calloc(1, required_size);
            if (states) {
                nvs_get_blob(my_handle, "lever_st_v2", states, &required_size);
                ESP_LOGI(TAG, "Restoring %d bytes of lever state from NVS...", required_size);
                controller_apply_serialized_state(states, required_size);
                free(states);
            }
        } else {
            ESP_LOGW(TAG, "Serialized state size mismatch. Wiping state and booting to safe defaults.");
        }
    }
    nvs_close(my_handle);
}

void state_manager_save(uint32_t current_config_hash) {
    size_t bytes_needed = controller_get_serialized_size();
    if (bytes_needed == 0) return;
    
    uint8_t *states = calloc(1, bytes_needed);
    if (!states) return;
    
    if (controller_get_serialized_state(states, bytes_needed)) {
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
        if (err == ESP_OK) {
            nvs_set_u32(my_handle, "cfg_hash", current_config_hash);
            nvs_set_blob(my_handle, "lever_st_v2", states, bytes_needed);
            nvs_commit(my_handle);
            nvs_close(my_handle);
            ESP_LOGD(TAG, "Lever state automatically saved to NVS.");
        }
    }
    
    free(states);
}
