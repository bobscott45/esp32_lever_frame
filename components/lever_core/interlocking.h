#ifndef ESP32_LEVER_FRAME_INTERLOCKING_H
#define ESP32_LEVER_FRAME_INTERLOCKING_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define MAX_INTERLOCKING_CONDITIONS 4

typedef enum {
    LEVER_TYPE_HOME_SIGNAL = 0,
    LEVER_TYPE_DISTANT_SIGNAL,
    LEVER_TYPE_POINTS,
    LEVER_TYPE_FACING_POINTS,
    LEVER_TYPE_SPARE,
    LEVER_TYPE_BROWN,
    LEVER_TYPE_GREEN
} lever_type_t;

typedef struct {
    int target_lever_index; // -1 if unused
    bool required_state; // true = THROWN, false = NORMAL
    int alt_target_lever_index; // -1 if unused
    bool alt_required_state; // true = THROWN, false = NORMAL
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
    const char *wifi_ssid;
    const char *wifi_station_password;
    bool restore_last_state;
    interlocking_conflict_policy_t conflict_policy;
    bool lcc_enabled;
} lever_system_config_t;

/**
 * Evaluates if a given lever movement is allowed based on the rules and current lever states.
 * This pure logic function is decoupled from LVGL graphics and can be unit tested.
 * 
 * @param tab_def The configuration rules for the current frame
 * @param lever_states A boolean array representing the current state of all levers (true=THROWN, false=NORMAL)
 * @param lever_index_to_move The index of the lever being evaluated
 * @param target_state_thrown The desired state of the lever (true=THROWN, false=NORMAL)
 * @return true if the movement is permitted, false if blocked by interlocking
 */
bool lever_evaluate_interlocking(const tab_def_t *tab_def, const bool *lever_states, int lever_index_to_move, bool target_state_thrown);

/**
 * Checks if the current state of a specific lever is in violation of the interlocking rules.
 */
bool lever_is_state_illegal(const tab_def_t *tab_def, const bool *lever_states, int lever_index_to_check);

#endif // ESP32_LEVER_FRAME_INTERLOCKING_H
