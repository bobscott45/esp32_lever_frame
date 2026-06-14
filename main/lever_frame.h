#ifndef LVGL_TEST_LEVER_FRAME_H
#define LVGL_TEST_LEVER_FRAME_H

#include <stddef.h>
#include "lvgl.h"
#include "lever.h"

#define MAX_INTERLOCKING_CONDITIONS 4

typedef struct {
    int target_lever_index; // -1 if unused
    bool required_state; // true = THROWN, false = NORMAL
} interlocking_condition_t;

typedef enum {
    INTERLOCK_POLICY_STRICT_LOCAL = 0, // Reject external changes that violate local locking
    INTERLOCK_POLICY_NETWORK_OVERRIDE = 1, // Accept external changes, ignore local locking
    INTERLOCK_POLICY_ALARM = 2 // Accept external changes, but flag an error
} interlocking_conflict_policy_t;

typedef struct {
    const char *label;
    lever_type_t type;
    interlocking_condition_t conditions[MAX_INTERLOCKING_CONDITIONS];
    char lcc_event_normal[64];
    char lcc_event_reversed[64];
    bool lcc_enabled;
} lever_def_t;

typedef struct {
    const char *name;
    const lever_def_t *levers;
    size_t lever_count;
    uint8_t label_lines;
    uint8_t label_line_height;
} tab_def_t;

typedef struct {
    const tab_def_t *tabs;
    size_t tab_count;
    const char *wifi_password;
    bool restore_last_state;
    interlocking_conflict_policy_t conflict_policy;
    bool lcc_enabled;
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
