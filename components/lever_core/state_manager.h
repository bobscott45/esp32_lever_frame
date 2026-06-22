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
 * @file      state_manager.h
 * @brief     Definitions for state_manager.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include <stdint.h>
#include "config_manager.h"

/**
 * @brief  Load state from NVS and apply it.
 *
 * This function loads the previously saved lever states from Non-Volatile Storage (NVS).
 * If the current configuration hash matches the saved hash, it applies those states 
 * to the user interface, restoring the system to its last known state.
 *
 * @param[in]  current_config_hash   The hash of the active configuration.
 */
void state_manager_load_and_apply(uint32_t current_config_hash);

/**
 * @brief  Save current states to NVS.
 *
 * This function saves the current states of all levers from the controller 
 * into Non-Volatile Storage (NVS). It also saves the configuration hash so that
 * on next boot the states can be verified against the active configuration.
 *
 * @param[in]  current_config_hash   The hash of the active configuration.
 */
void state_manager_save(uint32_t current_config_hash);

#endif // STATE_MANAGER_H
