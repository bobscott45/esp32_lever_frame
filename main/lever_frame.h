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

#ifndef LVGL_TEST_LEVER_FRAME_H
#define LVGL_TEST_LEVER_FRAME_H

#include <stddef.h>
#include "lvgl.h"
#include "lever.h"
#include "interlocking.h"

/**
 * Creates a horizontal container to hold a row of levers.
 * Designed to be placed inside a tabview or screen.
 */
lv_obj_t *lever_frame_create(lv_obj_t *parent);

/**
 * Adds a new lever to the frame using the lever factory.
 * 
 * @param frame The lever frame container
 * @param label_text The text for the brass plate
 * @param type The type of lever (sets color and text semantics)
 * @return The created lever wrapper object
 */
lv_obj_t *lever_frame_add_lever(lv_obj_t *frame, const lever_def_t *lever_def);

/**
 * Creates a complete lever system (tabview and horizontal lever frames) based on the configuration struct.
 * 
 * @param parent Parent object (usually lv_scr_act())
 * @param config Configuration defining tabs and their levers
 * @return The created tabview object
 */
lv_obj_t *lever_system_create(lv_obj_t *parent, const lever_system_config_t *config);

#endif //LVGL_TEST_LEVER_FRAME_H
