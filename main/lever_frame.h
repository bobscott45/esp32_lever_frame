#ifndef LVGL_TEST_LEVER_FRAME_H
#define LVGL_TEST_LEVER_FRAME_H

#include <stddef.h>
#include "lvgl.h"
#include "lever.h"

typedef struct {
    const char *label;
    lever_type_t type;
} lever_def_t;

typedef struct {
    const char *name;
    const lever_def_t *levers;
    size_t lever_count;
} tab_def_t;

typedef struct {
    const tab_def_t *tabs;
    size_t tab_count;
} lever_system_config_t;

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
lv_obj_t *lever_frame_add_lever(lv_obj_t *frame, const char *label_text, lever_type_t type);

/**
 * Creates a complete lever system (tabview and horizontal lever frames) based on the configuration struct.
 * 
 * @param parent Parent object (usually lv_scr_act())
 * @param config Configuration defining tabs and their levers
 * @return The created tabview object
 */
lv_obj_t *lever_system_create(lv_obj_t *parent, const lever_system_config_t *config);

#endif //LVGL_TEST_LEVER_FRAME_H
