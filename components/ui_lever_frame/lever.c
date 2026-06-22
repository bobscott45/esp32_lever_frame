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
 * @file      lever.c
 * @brief     Implementation of lever.c
 *
 * @author    Robert Scott
 * @date      2026
 */



#include "lever.h"
#include "display/lv_display.h"
#include "lever_frame.h"
#include <string.h>
#include "state_manager.h"
#include "config_manager.h"
#include "controller.h"

/**
 * @brief  Updates the system lock UI for a specific lever switch.
 *
 * Evaluates the interlocking rules and manual lock state to determine whether
 * the given lever switch should be interactable, visually updating the indicator
 * and collar button accordingly.
 *
 * @param[in]  sw  The lever switch object to update.
 */
static void lever_update_system_lock_ui(lv_obj_t *sw);

/**
 * @brief  Callback for the LCC enabled switch toggle.
 *
 * Handles the event when the LCC enabled switch is toggled by the user, updating
 * the corresponding configuration value in the config manager.
 *
 * @param[in]  ev  The event object containing the switch and user data.
 */
static void lcc_enabled_sw_cb(lv_event_t *ev) {
    lv_obj_t *sw = lv_event_get_target(ev);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    int *indices = (int *)lv_event_get_user_data(ev);
    config_manager_update_lever_bool(indices[0], indices[1], "lcc_enabled", is_on);
}

/**
 * @brief  Callback for the manual collar lock button.
 *
 * Processes the toggling of the manual lock collar button, updating the controller
 * lock state and requesting a UI refresh for the associated lever system.
 *
 * @param[in]  e  The event object containing the button target.
 */
static void collar_btn_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * wrapper = lv_obj_get_parent(btn);
    lv_obj_t *container = lv_obj_get_child(wrapper, 0);
    lv_obj_t *switch_group = lv_obj_get_child(container, 1);
    lv_obj_t *sw = lv_obj_get_child(switch_group, 1);
    
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    lv_obj_t *tab = lv_obj_get_parent(frame);
    
    int tab_index = lv_obj_get_index(tab);
    int lever_index = lv_obj_get_index(wrapper);
    bool is_locked = lv_obj_has_state(btn, LV_STATE_CHECKED);
    controller_set_lever_lock(tab_index, lever_index, is_locked);
    
    // Defer all UI state logic to the centralized lock UI updater
    lever_update_system_lock_ui(sw);
    
    state_manager_save(config_manager_get_hash());
}



/**
 * @brief  Checks the interlocking rules for a lever movement.
 *
 * Evaluates whether transitioning the lever to the specified target state is
 * permitted under the current system configuration and states of other levers.
 *
 * @param[in]  sw                   The switch object attempting to move.
 * @param[in]  target_state_thrown  Boolean indicating if the target state is thrown.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static bool interlocking_check(lv_obj_t *sw, bool target_state_thrown) {
    lv_obj_t *switch_group = lv_obj_get_parent(sw);
    lv_obj_t *container = lv_obj_get_parent(switch_group);
    lv_obj_t *wrapper = lv_obj_get_parent(container);
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    lv_obj_t *tab = lv_obj_get_parent(frame);
    
    int tab_index = lv_obj_get_index(tab);
    int lever_index = lv_obj_get_index(wrapper);
    const tab_def_t *tab_def = (const tab_def_t *)lv_obj_get_user_data(frame);
    if (!tab_def) return true; // No config attached, allow movement
    
    const bool *lever_states = controller_get_tab_states(tab_index);
    if (!lever_states) return true;
    
    return lever_evaluate_interlocking(tab_def, lever_states, lever_index, target_state_thrown);
}

/**
 * @brief  Updates the system lock UI for a specific lever switch.
 *
 * Evaluates the interlocking rules and manual lock state to determine whether
 * the given lever switch should be interactable, visually updating the indicator
 * and collar button accordingly.
 *
 * @param[in]  sw  The lever switch object to update.
 */
static void lever_update_system_lock_ui(lv_obj_t *sw) {
    if (!sw) return;
    
    bool is_thrown = lv_obj_has_state(sw, LV_STATE_CHECKED);
    // system_locked is true if the system DOES NOT allow movement
    bool system_locked = !interlocking_check(sw, !is_thrown);
    
    lv_obj_t *switch_group = lv_obj_get_parent(sw);
    lv_obj_t *container = lv_obj_get_parent(switch_group);
    lv_obj_t *wrapper = lv_obj_get_parent(container);
    
    lv_obj_t *indicator = lv_obj_get_child(sw, 0);
    lv_obj_t *collar_btn = lv_obj_get_child(wrapper, 1);
    bool manual_locked = collar_btn && lv_obj_has_state(collar_btn, LV_STATE_CHECKED);
    
    bool current_illegal = false;
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    lv_obj_t *tab = lv_obj_get_parent(frame);
    int tab_index = lv_obj_get_index(tab);
    int lever_index = lv_obj_get_index(wrapper);
    const tab_def_t *tab_def = (const tab_def_t *)lv_obj_get_user_data(frame);
    
    const bool *lever_states = controller_get_tab_states(tab_index);
    if (tab_def && lever_states) {
        current_illegal = lever_is_state_illegal(tab_def, lever_states, lever_index);
    }
    
    // Switch is unclickable if either system locked OR manually locked
    if (system_locked || manual_locked) {
        lv_obj_remove_flag(sw, LV_OBJ_FLAG_CLICKABLE);
    } else {
        lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
    }
    
    // Mechanical Pin shows if system locked OR manually locked
    if (indicator) {
        if (system_locked || manual_locked) {
            lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        } else {
            lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        }
    }
    
    // Collar Button Text and State
    if (collar_btn) {
        lv_obj_t *collar_lbl = lv_obj_get_child(collar_btn, 0);
        if (collar_lbl) {
            const char *current_text = lv_label_get_text(collar_lbl);
            if (current_illegal) {
                if (strcmp(current_text, "ALARM") != 0) lv_label_set_text(collar_lbl, "ALARM");
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xff8800), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xff8800), LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_text_color(collar_lbl, lv_color_hex(0xffffff), 0);
            } else if (manual_locked) {
                if (strcmp(current_text, "LOCKED") != 0) lv_label_set_text(collar_lbl, "LOCKED");
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0x252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xcc3333), LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_text_color(collar_lbl, lv_color_hex(0xffffff), 0);
            } else if (system_locked) {
                if (strcmp(current_text, "INTERLOCK") != 0) lv_label_set_text(collar_lbl, "INTERLOCK");
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0x252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xcc3333), LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_text_color(collar_lbl, lv_color_hex(0xaaaaaa), 0);
            } else {
                if (strcmp(current_text, "UNLOCKED") != 0) lv_label_set_text(collar_lbl, "UNLOCKED");
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0x252525), LV_PART_MAIN | LV_STATE_DEFAULT);
                lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xcc3333), LV_PART_MAIN | LV_STATE_CHECKED);
                lv_obj_set_style_text_color(collar_lbl, lv_color_hex(0xffffff), 0);
            }
            lv_obj_clear_state(collar_btn, LV_STATE_DISABLED);
        }
    }
}

void lever_frame_update_system_locks(lv_obj_t *frame) {
    uint32_t child_cnt = lv_obj_get_child_cnt(frame);
    for (uint32_t i = 0; i < child_cnt; i++) {
        lv_obj_t *wrapper = lv_obj_get_child(frame, i);
        lv_obj_t *container = lv_obj_get_child(wrapper, 0);
        lv_obj_t *switch_group = lv_obj_get_child(container, 1);
        lv_obj_t *sw = lv_obj_get_child(switch_group, 1);
        
        lever_update_system_lock_ui(sw);
    }
}

/**
 * @brief  Refreshes the visual state of the lever switch labels.
 *
 * Updates the text color of the normal and thrown labels to highlight the
 * active state based on whether the switch is currently thrown or not.
 *
 * @param[in]  e  The refresh event containing the switch target.
 */
static void lever_switch_refresh_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    bool is_thrown = lv_obj_has_state(sw, LV_STATE_CHECKED);
    lv_obj_t * switch_group = lv_obj_get_parent(sw);
    lv_obj_t * normal_label = lv_obj_get_child(switch_group, 0);
    lv_obj_t * thrown_label = lv_obj_get_child(switch_group, 2);

    if(is_thrown) {
        lv_obj_set_style_text_color(normal_label, lv_color_hex(0x888888), LV_PART_MAIN);
        lv_obj_set_style_text_color(thrown_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    } else {
        lv_obj_set_style_text_color(normal_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(thrown_label, lv_color_hex(0x888888), LV_PART_MAIN);
    }
}

/**
 * @brief  Handles the value changed event for a lever switch.
 *
 * Intercepts a lever toggle request, validates it against the controller's
 * interlocking logic, and either accepts the move and updates system locks,
 * or rejects it and reverts the visual state.
 *
 * @param[in]  e  The event object containing the switch target.
 */
static void lever_switch_event_cb(lv_event_t * e) {
    lever_switch_refresh_cb(e);
    
    lv_obj_t * sw = lv_event_get_target(e);
    lv_obj_t * switch_group = lv_obj_get_parent(sw);
    lv_obj_t * container = lv_obj_get_parent(switch_group);
    lv_obj_t * wrapper = lv_obj_get_parent(container);
    lv_obj_t * frame = lv_obj_get_parent(wrapper);
    lv_obj_t * tab = lv_obj_get_parent(frame);
    
    int tab_index = lv_obj_get_index(tab);
    int lever_index = lv_obj_get_index(wrapper);
    bool is_thrown = lv_obj_has_state(sw, LV_STATE_CHECKED);
    
    // Request the controller to move the lever
    bool accepted = controller_request_lever_move(tab_index, lever_index, is_thrown);
    
    if (!accepted) {
        // If the controller rejects the move, revert the UI state and abort
        if (is_thrown) {
            lv_obj_clear_state(sw, LV_STATE_CHECKED);
        } else {
            lv_obj_add_state(sw, LV_STATE_CHECKED);
        }
        lever_switch_refresh_cb(e); // Fix the labels back
        return;
    }
    
    // Update all levers in this frame to reflect new dependencies
    lever_frame_update_system_locks(frame);
}

/**
 * @brief  Creates the main container for a lever.
 *
 * Initializes the flexible column layout and styling that holds all visual
 * components of a single lever, simulating a physical metal plate.
 *
 * @param[in]  parent  The parent object to attach the container to.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *container_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_width(obj, LEVER_CONTAINER_WIDTH);
    lv_obj_set_flex_grow(obj, 1); // Expand to fill available vertical space
    lv_obj_set_style_bg_color(obj, lv_color_hex(LEVER_CONTAINER_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_top(obj, 10, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(obj, 10, LV_PART_MAIN);
    // Set minimal side padding so contents can expand
    lv_obj_set_style_pad_left(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_right(obj, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_row(obj, LEVER_CONTAINER_PAD_ROW, LV_PART_MAIN);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

    // Bezel effect to simulate physical metal plate
    lv_obj_set_style_border_color(obj, lv_color_hex(0x333333), LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN);
    lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_TOP | LV_BORDER_SIDE_LEFT, LV_PART_MAIN);
    
    return obj;
}

/**
 * @brief  Creates the header container for a lever.
 *
 * Groups the top label and the color indicator bar together within the lever's
 * main container, maintaining proper alignment and spacing.
 *
 * @param[in]  parent  The parent container object.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *header_container_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, lv_pct(100), LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(obj, 6, LV_PART_MAIN); // 6px gap between plate and indicator bar
    
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    return obj;
}

/**
 * @brief  Creates the switch container for a lever.
 *
 * Houses the lever switch and its corresponding state labels, ensuring they
 * stretch appropriately and align vertically within the lever assembly.
 *
 * @param[in]  parent  The parent container object.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *switch_container_create(lv_obj_t *parent) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_width(obj, lv_pct(100));
    lv_obj_set_flex_grow(obj, 1); // Stretch to fill the black plate
    lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(obj, 0, LV_PART_MAIN);
    
    lv_obj_set_layout(obj, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(obj, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(obj, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(obj, 8, LV_PART_MAIN); // Brings the labels closer to the switch
    
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    return obj;
}

static lv_obj_t *lever_info_drawer = NULL;
static lv_obj_t *lever_info_dimmer = NULL;



void lever_close_all_drawers(void) {
    if (lever_info_drawer) {
        lv_obj_del(lever_info_drawer);
        lever_info_drawer = NULL;
    }
    if (lever_info_dimmer) {
        lv_obj_del(lever_info_dimmer);
        lever_info_dimmer = NULL;
    }
}

/**
 * @brief  Handles click events to dismiss the lever info drawer.
 *
 * Detects clicks or gestures on the drawer or its background dimmer, and
 * closes the drawer by deleting its associated UI objects.
 *
 * @param[in]  e  The click or gesture event object.
 */
static void lever_drawer_click_cb(lv_event_t * e) {
    if (lever_info_drawer) {
        lv_obj_del(lever_info_drawer);
        lever_info_drawer = NULL;
    }
    if (lever_info_dimmer) {
        lv_obj_del(lever_info_dimmer);
        lever_info_dimmer = NULL;
    }
}

/**
 * @brief  Adds a key-value row to the lever info drawer.
 *
 * Creates a flexible row containing a customized key label and value label,
 * used for displaying detailed configuration information about a lever.
 *
 * @param[in]  parent  The parent table or container object.
 * @param[in]  key     The string representing the key label.
 * @param[in]  val     The string representing the value label.
 * @param[in]  color   The hex color code for the key text.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t* ui_add_drawer_row(lv_obj_t *parent, const char *key, const char *val, uint32_t color) {
    lv_obj_t *row = lv_obj_create(parent);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_key = lv_label_create(row);
    lv_obj_set_width(lbl_key, 160);
    lv_label_set_text(lbl_key, key);
    lv_obj_set_style_text_color(lbl_key, lv_color_hex(color), 0);
    
    lv_obj_t *lbl_val = lv_label_create(row);
    lv_label_set_text(lbl_val, val);
    lv_obj_set_style_text_color(lbl_val, lv_color_hex(0xFFFFFF), 0);
    return row;
}

/**
 * @brief  Handles click events on the lever's brass plate header.
 *
 * Opens an informational drawer displaying detailed settings and interlocking
 * rules for the specific lever when its header area is interacted with.
 *
 * @param[in]  e  The click event containing the lever definition user data.
 */
static void brass_plate_click_cb(lv_event_t * e) {
    const lever_def_t *lever_def = lv_event_get_user_data(e);
    if (!lever_def || lever_info_drawer || lever_info_dimmer) return;
    
    lever_info_dimmer = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(lever_info_dimmer);
    lv_obj_set_size(lever_info_dimmer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(lever_info_dimmer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lever_info_dimmer, LV_OPA_COVER, 0);
    lv_obj_add_flag(lever_info_dimmer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lever_info_dimmer, lever_drawer_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(lever_info_dimmer, lever_drawer_click_cb, LV_EVENT_GESTURE, NULL);
    
    lever_info_drawer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(lever_info_drawer, LV_PCT(100), LV_PCT(85));
    lv_obj_align(lever_info_drawer, LV_ALIGN_TOP_MID, 0, 0);
    
    lv_obj_set_style_bg_color(lever_info_drawer, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(lever_info_drawer, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(lever_info_drawer, 0, 0);
    lv_obj_set_style_radius(lever_info_drawer, 0, 0);
    lv_obj_set_style_border_side(lever_info_drawer, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(lever_info_drawer, 4, 0);
    lv_obj_set_style_border_color(lever_info_drawer, lv_color_hex(0x8a6327), 0);
    
    const char *lcc_norm = (lever_def->lcc_event_normal[0] != '\0') ? lever_def->lcc_event_normal : "None";
    const char *lcc_rev = (lever_def->lcc_event_reversed[0] != '\0') ? lever_def->lcc_event_reversed : "None";
    
    lv_obj_t *title = lv_label_create(lever_info_drawer);
    lv_label_set_text(title, "Lever Settings");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    lv_obj_align(title, LV_ALIGN_TOP_MID, 0, 15);
    
    lv_obj_t *table = lv_obj_create(lever_info_drawer);
    lv_obj_set_size(table, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_align(table, LV_ALIGN_TOP_MID, 0, 45);
    lv_obj_set_style_bg_opa(table, 0, 0);
    lv_obj_set_style_border_width(table, 0, 0);
    lv_obj_set_style_pad_all(table, 0, 0);
    lv_obj_set_layout(table, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(table, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    
    // Store context for the callback and rule generation
    lv_obj_t *header = lv_event_get_current_target(e);
    lv_obj_t *container = lv_obj_get_parent(header);
    lv_obj_t *wrapper = lv_obj_get_parent(container);
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    lv_obj_t *tab = lv_obj_get_parent(frame);
    const tab_def_t *tab_def = (const tab_def_t *)lv_obj_get_user_data(frame);
    
    ui_add_drawer_row(table, "Label:", lever_def->label, 0x8a6327);
    
    bool has_rules = false;
    for (int i = 0; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        int target_idx = lever_def->conditions[i].target_lever_index;
        if (target_idx >= 0 && tab_def && target_idx < tab_def->lever_count) {
            char rule_buf[128];
            int alt_target = lever_def->conditions[i].alt_target_lever_index;
            if (alt_target >= 0 && tab_def && alt_target < tab_def->lever_count) {
                snprintf(rule_buf, sizeof(rule_buf), "Lever %d %s OR Lever %d %s", 
                    target_idx + 1, lever_def->conditions[i].required_state ? "REVERSED" : "NORMAL",
                    alt_target + 1, lever_def->conditions[i].alt_required_state ? "REVERSED" : "NORMAL");
            } else {
                snprintf(rule_buf, sizeof(rule_buf), "Lever %d %s", target_idx + 1, lever_def->conditions[i].required_state ? "REVERSED" : "NORMAL");
            }
            lv_obj_t *r = NULL;
            if (!has_rules) {
                r = ui_add_drawer_row(table, "Requires:", rule_buf, 0x8a6327);
                has_rules = true;
            } else {
                r = ui_add_drawer_row(table, "And:", rule_buf, 0x8a6327);
            }
            if (r) {
                lv_obj_set_style_pad_top(r, 0, 0);
                lv_obj_set_style_pad_bottom(r, 0, 0);
            }
        }
    }
    if (!has_rules) {
        ui_add_drawer_row(table, "Requires:", "None", 0x8a6327);
    }
    
    lv_obj_t *spacer = lv_obj_create(table);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_size(spacer, 10, 15);
    
    // Interactive LCC Enabled Switch
    lv_obj_t *row = lv_obj_create(table);
    lv_obj_set_size(row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(row, 0, 0);
    lv_obj_set_style_border_width(row, 0, 0);
    lv_obj_set_style_pad_all(row, 4, 0);
    lv_obj_set_layout(row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(row, LV_FLEX_FLOW_ROW);
    lv_obj_set_flex_align(row, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_remove_flag(row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lbl_key = lv_label_create(row);
    lv_obj_set_width(lbl_key, 160);
    lv_label_set_text(lbl_key, "LCC Enabled:");
    lv_obj_set_style_text_color(lbl_key, lv_color_hex(0x8a6327), 0);

    lv_obj_t *lcc_sw = lv_switch_create(row);
    lv_obj_set_style_anim_time(lcc_sw, 0, 0);
    if (lever_def->lcc_enabled) {
        lv_obj_add_state(lcc_sw, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(lcc_sw, LV_STATE_CHECKED);
    }
    
    // Context already retrieved above
    
    static int ctx_indices[2];
    ctx_indices[0] = lv_obj_get_index(tab);
    ctx_indices[1] = lv_obj_get_index(wrapper);
    
    lv_obj_add_event_cb(lcc_sw, lcc_enabled_sw_cb, LV_EVENT_VALUE_CHANGED, ctx_indices);
    
    ui_add_drawer_row(table, "Normal Event:", lcc_norm, 0x8a6327);
    ui_add_drawer_row(table, "Reversed Event:", lcc_rev, 0x8a6327);
    
    lv_obj_t *footer = lv_label_create(lever_info_drawer);
    lv_label_set_text(footer, "(Swipe Up or Tap to dismiss)");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x888888), 0);
    lv_obj_align(footer, LV_ALIGN_BOTTOM_MID, 0, -15);
    
    lv_obj_add_flag(lever_info_drawer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lever_info_drawer, lever_drawer_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(lever_info_drawer, lever_drawer_click_cb, LV_EVENT_GESTURE, NULL);
}

/**
 * @brief  Creates the brass plate label for a lever.
 *
 * Generates a styled label mimicking an engraved brass plate, centering the
 * provided text within a fixed-size container above the lever switch.
 *
 * @param[in]  parent        The parent header container.
 * @param[in]  lever_def     Pointer to the lever's definition struct.
 * @param[in]  label_height  The total height allocated for the label plate.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *lever_label_create(lv_obj_t *parent, const lever_def_t *lever_def, uint8_t label_height) {
    // Create the brass plate as a fixed-size container
    lv_obj_t *plate = lv_obj_create(parent);
    lv_obj_set_width(plate, lv_pct(100));
    lv_obj_set_height(plate, label_height); // Uniform height to accommodate multi-line text
    lv_obj_remove_flag(plate, LV_OBJ_FLAG_SCROLLABLE);
    
    lv_obj_remove_flag(plate, LV_OBJ_FLAG_CLICKABLE); // Let clicks pass through to header
    
    // Style the plate
    lv_obj_set_style_bg_color(plate, lv_color_hex(LEVER_LABEL_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(plate, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(plate, 2, LV_PART_MAIN);
    lv_obj_set_style_border_color(plate, lv_color_hex(0x5c421a), LV_PART_MAIN);
    lv_obj_set_style_radius(plate, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(plate, 0, LV_PART_MAIN);
    
    // Create the text label and center it perfectly inside the plate
    lv_obj_t *label = lv_label_create(plate);
    lv_label_set_text(label, lever_def->label);
    lv_obj_align(label, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_text_color(label, lv_color_hex(LEVER_LABEL_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    return plate;
}

/**
 * @brief  Creates a static engraved text label.
 *
 * Generates a non-interactive text label styled to look like an engraving on
 * the metal panel, typically used for the normal and thrown state indicators.
 *
 * @param[in]  parent  The parent switch container.
 * @param[in]  text    The string to display in the label.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *static_label_create(lv_obj_t *parent, const char *text) {
    lv_obj_t *obj = lv_label_create(parent);
    
    // Fill available width to prevent clipping
    lv_obj_set_width(obj, lv_pct(100));
    
    lv_obj_set_style_text_align(obj, LV_TEXT_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
    // Use a slightly darker color to look like an engraving on the panel
    lv_obj_set_style_text_color(obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_label_set_text(obj, text);

    return obj;
}

/**
 * @brief  Creates the visual lever switch component.
 *
 * Instantiates the actual switch object, applying custom styling to resemble
 * a physical lever operating within a dark slot, color-coded by type.
 *
 * @param[in]  parent      The parent switch container.
 * @param[in]  type_color  The hex color code assigned to the lever type.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *lever_switch_create(lv_obj_t *parent, uint32_t type_color) {
    lv_obj_t *obj = lv_switch_create(parent);
    lv_obj_set_style_anim_time(obj, 0, 0);
    lv_obj_set_width(obj, LEVER_WIDTH);
    lv_obj_set_flex_grow(obj, 1); // Stretch vertically between the labels
    lv_obj_set_style_base_dir(obj, LV_BASE_DIR_RTL, 0);
    
    // Deep black track void with a metallic border
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x050505), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_color(obj, lv_color_hex(0x2b2b2b), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_radius(obj, 6, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Keep the switch track a dark void in checked state
    lv_obj_set_style_bg_color(obj, lv_color_hex(0x050505), LV_PART_INDICATOR | LV_STATE_CHECKED);
    lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, LV_PART_INDICATOR | LV_STATE_CHECKED);
    
    // Color code the knob
    lv_obj_set_style_bg_color(obj, lv_color_hex(type_color), LV_PART_KNOB | LV_STATE_DEFAULT);
    
    // Make knob overlap the track width so it looks like a handle sitting on top
    lv_obj_set_style_pad_left(obj, -4, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_right(obj, -4, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_top(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
    lv_obj_set_style_pad_bottom(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
    
    if (type_color == 0x000000) {
        lv_obj_set_style_border_color(obj, lv_color_hex(0x555555), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_border_side(obj, LV_BORDER_SIDE_FULL, LV_PART_KNOB | LV_STATE_DEFAULT);
    } else {
        lv_obj_set_style_border_width(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    }
    
    // Add a mechanical 250ms slide animation to the switch throw
    lv_obj_set_style_anim_duration(obj, 250, 0);
    
    return obj;
}

/**
 * @brief  Creates a colored identification bar for the lever.
 *
 * Adds a small rectangular color bar below the brass plate to quickly indicate
 * the functional type of the lever to the operator.
 *
 * @param[in]  parent  The parent header container.
 * @param[in]  color   The hex color code to fill the bar.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static lv_obj_t *color_bar_create(lv_obj_t *parent, uint32_t color) {
    lv_obj_t *obj = lv_obj_create(parent);
    lv_obj_set_size(obj, lv_pct(100), 16); // Fill available width
    lv_obj_set_style_bg_color(obj, lv_color_hex(color), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // No border by default
    lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // If it's a black points color bar, add a 1px silver border
    if (color == 0x000000) {
        lv_obj_set_style_border_color(obj, lv_color_hex(0x555555), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    return obj;
}

lv_obj_t *lever_create(lv_obj_t *parent, const void *lever_def_ptr, uint8_t label_lines, uint8_t label_line_height, int tab_index, int lever_index) {
    const lever_def_t *lever_def = (const lever_def_t *)lever_def_ptr;
    uint32_t type_color = 0x000000;
    const char *up_text = "";
    const char *down_text = "";
    lever_type_t type = lever_def->type;
    
    // Calculate total height needed for the brass plate
    uint8_t label_height = (label_lines * label_line_height) + LEVER_LABEL_PADDING_Y;

    switch (type) {
        case LEVER_TYPE_HOME_SIGNAL:
            type_color = LEVER_COLOR_HOME_SIGNAL;
            up_text = "ON";
            down_text = "OFF";
            break;
        case LEVER_TYPE_DISTANT_SIGNAL:
            type_color = LEVER_COLOR_DISTANT_SIGNAL;
            up_text = "ON";
            down_text = "OFF";
            break;
        case LEVER_TYPE_POINTS:
            type_color = LEVER_COLOR_POINTS;
            up_text = "NORMAL";
            down_text = "THROWN";
            break;
        case LEVER_TYPE_FACING_POINTS:
            type_color = LEVER_COLOR_FACING_POINTS;
            up_text = "NORMAL";
            down_text = "THROWN";
            break;
        case LEVER_TYPE_BROWN:
            type_color = LEVER_COLOR_BROWN;
            up_text = "NORMAL";
            down_text = "THROWN";
            break;
        case LEVER_TYPE_GREEN:
            type_color = LEVER_COLOR_GREEN;
            up_text = "NORMAL";
            down_text = "THROWN";
            break;
        case LEVER_TYPE_SPARE:
        default:
            type_color = LEVER_COLOR_SPARE;
            up_text = "";
            down_text = "";
            break;
    }

    // Master wrapper that holds the black plate and the external button
    lv_obj_t *wrapper = lv_obj_create(parent);
    lv_obj_set_size(wrapper, LV_SIZE_CONTENT, lv_pct(100)); // Fill available height perfectly
    lv_obj_set_style_bg_opa(wrapper, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(wrapper, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(wrapper, 0, LV_PART_MAIN);
    lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(wrapper, LV_FLEX_ALIGN_START, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_row(wrapper, 4, LV_PART_MAIN); // Tighter gap between plate and external button
    lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(wrapper, LV_SCROLLBAR_MODE_OFF);

    // The main black plate
    lv_obj_t *container = container_create(wrapper);
    
    // Group the top label and the color bar together
    lv_obj_t *header = header_container_create(container);
    lv_obj_add_flag(header, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(header, brass_plate_click_cb, LV_EVENT_CLICKED, (void *)lever_def);
    
    lever_label_create(header, lever_def, label_height);
    color_bar_create(header, type_color);
    
    // Group the switch and its labels tightly together
    lv_obj_t *switch_group = switch_container_create(container);
    
    lv_obj_t *normal_label = static_label_create(switch_group, up_text);
    lv_obj_t *sw = lever_switch_create(switch_group, type_color);
    if (controller_get_lever_state(tab_index, lever_index)) {
        lv_obj_add_state(sw, LV_STATE_CHECKED);
    }
    lv_obj_t *thrown_label = static_label_create(switch_group, down_text);
    
    // Create the lock indicator inside the switch slot
    lv_obj_t *lock_indicator = lv_obj_create(sw);
    lv_obj_set_size(lock_indicator, 24, 12); // Rectangular locking pin
    lv_obj_align(lock_indicator, LV_ALIGN_CENTER, 0, 0);
    lv_obj_set_style_bg_color(lock_indicator, lv_color_hex(0xFF0000), 0); // Bright Red
    lv_obj_set_style_border_color(lock_indicator, lv_color_hex(0xFFFFFF), 0); // White outline so it pops
    lv_obj_set_style_border_width(lock_indicator, 2, 0);
    lv_obj_set_style_radius(lock_indicator, 2, 0);
    lv_obj_clear_flag(lock_indicator, LV_OBJ_FLAG_CLICKABLE); // Pass touches through to switch
    lv_obj_add_flag(lock_indicator, LV_OBJ_FLAG_HIDDEN); // Hidden by default
    
    // Manual Lever Collar (External Red lock button)
    lv_obj_t *collar_btn = lv_btn_create(wrapper);
    lv_obj_set_size(collar_btn, LEVER_CONTAINER_WIDTH - 2, 44); // Automatically scale width to match the black plate
    lv_obj_set_ext_click_area(collar_btn, 10);
    lv_obj_add_flag(collar_btn, LV_OBJ_FLAG_CHECKABLE);
    lv_obj_set_style_radius(collar_btn, 8, LV_PART_MAIN);
    lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0x252525), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xcc3333), LV_PART_MAIN | LV_STATE_CHECKED);
    lv_obj_set_style_border_width(collar_btn, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_shadow_width(collar_btn, 0, LV_PART_MAIN);
    
    // Prevent the button from fading out when system locks it
    lv_obj_set_style_bg_opa(collar_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_add_event_cb(collar_btn, collar_btn_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Label inside the collar button
    lv_obj_t *collar_lbl = lv_label_create(collar_btn);
    lv_obj_align(collar_lbl, LV_ALIGN_CENTER, 0, 0);
    
    // Initialize the lock state visually
    if (controller_get_lever_lock(tab_index, lever_index)) {
        lv_obj_add_state(collar_btn, LV_STATE_CHECKED);
        lv_label_set_text(collar_lbl, "LOCKED");
    } else {
        lv_label_set_text(collar_lbl, "UNLOCKED");
    }
    
    lv_obj_set_style_text_color(collar_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    extern const lv_font_t lv_font_montserrat_12;
    lv_obj_set_style_text_font(collar_lbl, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
    
    lv_obj_set_style_text_opa(collar_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DISABLED);
    
    // Set initial label highlight state (Normal = Bright, Thrown = Dim)
    lv_obj_set_style_text_color(normal_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(thrown_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Add event callback to toggle highlight on switch toggle
    lv_obj_add_event_cb(sw, lever_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    lv_obj_add_event_cb(sw, lever_switch_refresh_cb, LV_EVENT_REFRESH, NULL);
    
    return wrapper;
}

void lever_set_locked(lv_obj_t *wrapper, bool locked) {
    // The container is the first child of the wrapper (index 0)
    lv_obj_t *container = lv_obj_get_child(wrapper, 0);
    if (!container) return;
    
    // The switch_group is the second child of the main container (index 1)
    lv_obj_t *switch_group = lv_obj_get_child(container, 1);
    if (!switch_group) return;
    
    // The switch itself is the second child of the switch_group (index 1)
    lv_obj_t *sw = lv_obj_get_child(switch_group, 1);
    if (!sw) return;
    
    // The lock indicator is the first child of the switch
    lv_obj_t *indicator = lv_obj_get_child(sw, 0);
    
    // The external collar button is the second child of the wrapper (index 1)
    lv_obj_t *collar_btn = lv_obj_get_child(wrapper, 1);
    lv_obj_t *collar_lbl = collar_btn ? lv_obj_get_child(collar_btn, 0) : NULL;
    
    if (locked) {
        lv_obj_remove_flag(sw, LV_OBJ_FLAG_CLICKABLE);
        if (indicator) lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        
        // Force the button to LOCKED and make it unclickable
        if (collar_btn) {
            lv_obj_add_state(collar_btn, LV_STATE_CHECKED | LV_STATE_DISABLED);
            if (collar_lbl) {
                if (strcmp(lv_label_get_text(collar_lbl), "LOCKED") != 0) {
                    lv_label_set_text(collar_lbl, "LOCKED");
                }
            }
        }
    } else {
        lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
        if (indicator) lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        
        // Restore button to UNLOCKED and make it clickable again
        if (collar_btn) {
            lv_obj_clear_state(collar_btn, LV_STATE_CHECKED | LV_STATE_DISABLED);
            if (collar_lbl) {
                if (strcmp(lv_label_get_text(collar_lbl), "UNLOCKED") != 0) {
                    lv_label_set_text(collar_lbl, "UNLOCKED");
                }
            }
        }
    }
}

void lever_set_state_labels(lv_obj_t *wrapper, const char *up_text, const char *down_text) {
    lv_obj_t *container = lv_obj_get_child(wrapper, 0);
    if (!container) return;
    
    lv_obj_t *switch_group = lv_obj_get_child(container, 1);
    if (!switch_group) return;
    
    lv_obj_t *normal_label = lv_obj_get_child(switch_group, 0);
    lv_obj_t *thrown_label = lv_obj_get_child(switch_group, 2);
    
    if (normal_label && up_text) {
        lv_label_set_text(normal_label, up_text);
    }
    if (thrown_label && down_text) {
        lv_label_set_text(thrown_label, down_text);
    }
}
