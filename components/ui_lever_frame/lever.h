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
lv_obj_t *lever_create(lv_obj_t *parent, const void *lever_def, uint8_t label_lines, uint8_t label_line_height, int tab_index, int lever_index);
void lever_set_locked(lv_obj_t *wrapper, bool locked);
void lever_set_state_labels(lv_obj_t *wrapper, const char *up_text, const char *down_text);
void lever_frame_update_system_locks(lv_obj_t *frame);
void lever_close_all_drawers(void);

#endif //LVGL_TEST_LEVER_H
