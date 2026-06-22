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
 * @file      controller.h
 * @brief     Definitions for controller.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include "config_manager.h"

/**
 * @brief  Initialize the controller with a configuration.
 *
 * This function allocates necessary memory and sets up internal arrays 
 * to hold the states and locks for all tabs and levers based on the provided configuration.
 *
 * @param[in]  config   Pointer to the lever system configuration.
 */
void controller_init(const lever_system_config_t *config);

/**
 * @brief  Free controller resources.
 *
 * This function releases all memory allocated for tab and lever states 
 * and locks, and resets internal counters.
 */
void controller_free(void);

/**
 * @brief  Get the current state of a specific lever.
 *
 * Retrieves whether the lever at the given tab and lever index is thrown.
 *
 * @param[in]  tab_index     Index of the tab.
 * @param[in]  lever_index   Index of the lever within the tab.
 * 
 * @return 
 *   - true if the lever is thrown
 *   - false if the lever is normal or indices are invalid
 */
bool controller_get_lever_state(int tab_index, int lever_index);

/**
 * @brief  Set the state of a specific lever.
 *
 * Directly updates the state of the specified lever. This does not check interlocking rules.
 *
 * @param[in]  tab_index     Index of the tab.
 * @param[in]  lever_index   Index of the lever within the tab.
 * @param[in]  is_thrown     The new state to apply (true for thrown, false for normal).
 */
void controller_set_lever_state(int tab_index, int lever_index, bool is_thrown);

/**
 * @brief  Get the current lock status of a specific lever.
 *
 * Retrieves whether the lever at the given tab and lever index is manually locked.
 *
 * @param[in]  tab_index     Index of the tab.
 * @param[in]  lever_index   Index of the lever within the tab.
 * 
 * @return 
 *   - true if the lever is locked
 *   - false if the lever is unlocked or indices are invalid
 */
bool controller_get_lever_lock(int tab_index, int lever_index);

/**
 * @brief  Set the lock status of a specific lever.
 *
 * Updates the manual lock state for the specified lever, preventing or allowing movement.
 *
 * @param[in]  tab_index     Index of the tab.
 * @param[in]  lever_index   Index of the lever within the tab.
 * @param[in]  is_locked     The new lock state to apply.
 */
void controller_set_lever_lock(int tab_index, int lever_index, bool is_locked);

/**
 * @brief  Get the index of the currently active tab.
 *
 * Returns the 0-based index of the tab currently active in the UI or system.
 *
 * @return 
 *   - The index of the active tab
 */
int controller_get_active_tab(void);

/**
 * @brief  Set the currently active tab.
 *
 * Updates the system to reflect a newly selected tab, verifying the bounds.
 *
 * @param[in]  tab_index   The index of the tab to become active.
 */
void controller_set_active_tab(int tab_index);

/**
 * @brief  Get the required size to serialize controller state.
 *
 * Calculates the number of bytes required to store the current active tab
 * and the states and locks of all levers across all tabs.
 *
 * @return 
 *   - Number of bytes required for serialization
 */
size_t controller_get_serialized_size(void);

/**
 * @brief  Serialize the controller state into a buffer.
 *
 * Writes the active tab, and all lever states and locks into the provided buffer.
 * Each lever uses 2 bits (1 for state, 1 for lock).
 *
 * @param[out] buffer    Buffer to write the serialized data into.
 * @param[in]  max_len   Maximum length of the buffer.
 * 
 * @return 
 *   - true on success
 *   - false on failure (e.g., buffer too small)
 */
bool controller_get_serialized_state(uint8_t *buffer, size_t max_len);

/**
 * @brief  Apply a serialized state to the controller.
 *
 * Reads a previously serialized buffer and updates the active tab,
 * lever states, and lever locks accordingly.
 *
 * @param[in]  buffer   Buffer containing serialized data.
 * @param[in]  len      Length of the buffer data.
 * 
 * @return 
 *   - true on successful application
 *   - false if the buffer is invalid or too small
 */
bool controller_apply_serialized_state(const uint8_t *buffer, size_t len);

/**
 * @brief  Get a pointer to the array of lever states for a tab.
 *
 * Returns direct access to the boolean array representing the state 
 * (thrown or normal) of all levers in the specified tab.
 *
 * @param[in]  tab_index   Index of the tab.
 * 
 * @return 
 *   - Pointer to boolean array of lever states
 *   - NULL if tab_index is invalid
 */
const bool* controller_get_tab_states(int tab_index);

/**
 * @brief  Request a lever movement, evaluating interlocking rules.
 *
 * Validates whether moving the specified lever to the target state is allowed 
 * based on current interlocking definitions. If allowed, applies the state and posts an event.
 *
 * @param[in]  tab_index             Index of the tab.
 * @param[in]  lever_index           Index of the lever within the tab.
 * @param[in]  target_state_thrown   The requested state to move the lever to.
 * 
 * @return 
 *   - true if the move was allowed and applied
 *   - false if blocked by interlocking or invalid indices
 */
bool controller_request_lever_move(int tab_index, int lever_index, bool target_state_thrown);

#endif // CONTROLLER_H
