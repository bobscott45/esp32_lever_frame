//
// Created by robert on 13/06/2026.
//

#ifndef LVGL_TEST_LEVER_H
#define LVGL_TEST_LEVER_H

#define LEVER_CONTAINER_WIDTH 96

#define LEVER_CONTAINER_BG_COLOR 0x1a1a1a
#define LEVER_CONTAINER_PAD_ROW 10

#define LEVER_WIDTH 60

#define LEVER_LABEL_COLOR 0x111111
#define LEVER_LABEL_BG_COLOR 0x8a6327 // Dark Antique Brass
#define LEVER_LABEL_PADDING_Y 14

#define LEVER_COLOR_HOME_SIGNAL    0x8f2727 // Darker Red
#define LEVER_COLOR_DISTANT_SIGNAL 0xb08817 // Darker Muted Yellow
#define LEVER_COLOR_POINTS         0x000000 // Black
#define LEVER_COLOR_FACING_POINTS  0x2b58b5 // Royal Blue
#define LEVER_COLOR_SPARE          0xb8b8b8 // Medium Light Gray

typedef enum {
    LEVER_TYPE_HOME_SIGNAL,
    LEVER_TYPE_DISTANT_SIGNAL,
    LEVER_TYPE_POINTS,
    LEVER_TYPE_FACING_POINTS,
    LEVER_TYPE_SPARE
} lever_type_t;

#define LEVER_STATE_NORMAL_COLOR   0x111111 // Dark Void for the slot

#include "lvgl.h"
lv_obj_t *lever_create(lv_obj_t *parent, const char *label_text, lever_type_t type, uint8_t label_lines, uint8_t label_line_height);
void lever_set_locked(lv_obj_t *wrapper, bool locked);
void lever_set_state_labels(lv_obj_t *wrapper, const char *up_text, const char *down_text);
void lever_frame_update_system_locks(lv_obj_t *frame);

#endif //LVGL_TEST_LEVER_H
