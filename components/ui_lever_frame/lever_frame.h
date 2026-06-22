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
 * @file      lever_frame.h
 * @brief     Definitions for lever_frame.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef LVGL_TEST_LEVER_FRAME_H
#define LVGL_TEST_LEVER_FRAME_H

#include <stddef.h>
#include "lvgl.h"
#include "lever.h"
#include "interlocking.h"

/**
 * @brief  Creates a horizontal container to hold a row of levers.
 *
 * Designed to be placed inside a tabview or screen, providing the foundational
 * structure and layout properties to arrange levers horizontally.
 *
 * @param[in]  parent  The parent object to attach the frame to.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
lv_obj_t *lever_frame_create(lv_obj_t *parent);

/**
 * @brief  Adds a new lever to the frame using the lever factory.
 *
 * Instantiates a new lever based on the provided definition and appends it to
 * the specified frame, assigning it the correct visual and logical properties.
 *
 * @param[in]  frame        The lever frame container.
 * @param[in]  lever_def    The definition struct for the lever.
 * @param[in]  tab_index    The index of the tab containing the frame.
 * @param[in]  lever_index  The index of the new lever within the frame.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
lv_obj_t *lever_frame_add_lever(lv_obj_t *frame, const lever_def_t *lever_def, int tab_index, int lever_index);

/**
 * @brief  Creates a complete lever system based on the configuration.
 *
 * Initializes the entire UI hierarchy including tabviews and horizontal lever
 * frames, populating them with levers as specified in the configuration struct.
 *
 * @param[in]  parent  Parent object (usually lv_scr_act()).
 * @param[in]  config  Configuration defining tabs and their levers.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
lv_obj_t *lever_system_create(lv_obj_t *parent, const lever_system_config_t *config);

#endif //LVGL_TEST_LEVER_FRAME_H
