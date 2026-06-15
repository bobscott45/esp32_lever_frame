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

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "lvgl.h"
#include <stdint.h>

/**
 * @brief Load state from NVS. If the current_config_hash matches the saved hash, apply the states to the UI.
 * @param system_wrapper The root wrapper object of the lever system
 * @param current_config_hash The hash of the active configuration
 */
void state_manager_load_and_apply(lv_obj_t *system_wrapper, uint32_t current_config_hash);

/**
 * @brief Save the current states from the UI to NVS along with the config hash.
 * @param system_wrapper The root wrapper object of the lever system
 * @param current_config_hash The hash of the active configuration
 */
void state_manager_save(lv_obj_t *system_wrapper, uint32_t current_config_hash);

#endif // STATE_MANAGER_H
