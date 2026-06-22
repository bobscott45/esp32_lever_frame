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
 * @file      lever.h
 * @brief     Definitions for lever.h
 *
 * @author    Robert Scott
 * @date      2026
 */



#ifndef LVGL_TEST_LEVER_H
#define LVGL_TEST_LEVER_H

#define LEVER_CONTAINER_WIDTH 96

#define LEVER_CONTAINER_BG_COLOR 0x1a1a1a
#define LEVER_CONTAINER_PAD_ROW 10

#define LEVER_WIDTH 60

#define LEVER_LABEL_COLOR 0xd4af37
#define LEVER_LABEL_BG_COLOR 0x1a1a1a // Industrial Black
#define LEVER_LABEL_BORDER_COLOR 0xb8860b
#define LEVER_LABEL_PADDING_Y 14

#define LEVER_COLOR_HOME_SIGNAL    0x8f2727 // Darker Red
#define LEVER_COLOR_DISTANT_SIGNAL 0xb08817 // Darker Muted Yellow
#define LEVER_COLOR_POINTS         0x000000 // Black
#define LEVER_COLOR_FACING_POINTS  0x2b58b5 // Royal Blue
#define LEVER_COLOR_SPARE          0xb8b8b8 // Medium Light Gray
#define LEVER_COLOR_BROWN          0x5C4033 // Dark Brown
#define LEVER_COLOR_GREEN          0x228B22 // Forest Green

#include "interlocking.h"

#define LEVER_STATE_NORMAL_COLOR   0x111111 // Dark Void for the slot

#include "lvgl.h"
/**
 * @brief  Creates a new lever object within the UI.
 *
 * This function initializes a graphical representation of a lever based on the
 * provided definition and places it in the specified container, accounting for
 * the tab and lever index within the broader lever frame system.
 *
 * @param[in]  parent               Parent object to attach the lever to.
 * @param[in]  lever_def            Pointer to the definition struct of the lever.
 * @param[in]  label_lines          Number of lines allocated for the lever's label.
 * @param[in]  label_line_height    Height of each label line.
 * @param[in]  tab_index            Index of the tab this lever belongs to.
 * @param[in]  lever_index          Index of the lever within its frame.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
lv_obj_t *lever_create(lv_obj_t *parent, const void *lever_def, uint8_t label_lines, uint8_t label_line_height, int tab_index, int lever_index);
/**
 * @brief  Sets the locked state of a specific lever.
 *
 * Visually updates the lever to indicate whether it is locked or unlocked,
 * modifying its interactive capabilities accordingly to prevent or allow user
 * input based on interlocking rules.
 *
 * @param[in]  wrapper  The wrapper object containing the lever to be modified.
 * @param[in]  locked   Boolean value indicating the desired lock state.
 */
void lever_set_locked(lv_obj_t *wrapper, bool locked);
/**
 * @brief  Sets the textual labels indicating the lever's states.
 *
 * Configures the text displayed above or below the lever to represent
 * its functional meaning when in the normal or reversed state.
 *
 * @param[in]  wrapper    The wrapper object of the lever to update.
 * @param[in]  up_text    String to display for the normal state.
 * @param[in]  down_text  String to display for the reversed state.
 */
void lever_set_state_labels(lv_obj_t *wrapper, const char *up_text, const char *down_text);
/**
 * @brief  Updates the locking status for all levers in the system.
 *
 * Iterates through the provided frame to re-evaluate and apply the current
 * interlocking rules, locking or unlocking individual levers based on the
 * overall system state.
 *
 * @param[in]  frame  The parent frame object containing the levers.
 */
void lever_frame_update_system_locks(lv_obj_t *frame);
/**
 * @brief  Closes all open drawers or auxiliary UI components.
 *
 * Ensures that any expanded state interfaces, such as configuration drawers
 * or detailed info panels associated with the levers, are collapsed back to
 * their hidden state.
 */
void lever_close_all_drawers(void);

#endif //LVGL_TEST_LEVER_H
