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
#define LEVER_LABEL_BG_COLOR 0xB5A642
#define LEVER_LABEL_LINES 2
#define LEVER_LABEL_LINE_HEIGHT 18
#define LEVER_LABEL_PADDING_Y 14
#define LEVER_LABEL_HEIGHT (LEVER_LABEL_LINES * LEVER_LABEL_LINE_HEIGHT + LEVER_LABEL_PADDING_Y)

#define LEVER_COLOR_HOME_SIGNAL    0xff0000 // Red
#define LEVER_COLOR_DISTANT_SIGNAL 0xffff00 // Yellow
#define LEVER_COLOR_POINTS         0x000000 // Black
#define LEVER_COLOR_FACING_POINTS  0x0000ff // Blue
#define LEVER_COLOR_SPARE          0xffffff // White

typedef enum {
    LEVER_TYPE_HOME_SIGNAL,
    LEVER_TYPE_DISTANT_SIGNAL,
    LEVER_TYPE_POINTS,
    LEVER_TYPE_FACING_POINTS,
    LEVER_TYPE_SPARE
} lever_type_t;

#define LEVER_STATE_NORMAL_COLOR   0x111111 // Dark Void for the slot

#include "lvgl.h"
lv_obj_t *lever_create(lv_obj_t *parent, const char *label_text, lever_type_t type);
void lever_set_locked(lv_obj_t *wrapper, bool locked);
void lever_set_state_labels(lv_obj_t *wrapper, const char *up_text, const char *down_text);

#endif //LVGL_TEST_LEVER_H
