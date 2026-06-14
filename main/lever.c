//
// Created by robert on 13/06/2026.
//

#include "lever.h"
#include "display/lv_display.h"
#include "lever_frame.h"
#include <string.h>
#include "state_manager.h"
#include "config_manager.h"
#include "openlcb_integration.h"



static void lever_update_system_lock_ui(lv_obj_t *sw);

static void lcc_enabled_sw_cb(lv_event_t *ev) {
    lv_obj_t *sw = lv_event_get_target(ev);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    int *indices = (int *)lv_event_get_user_data(ev);
    config_manager_update_lever_bool(indices[0], indices[1], "lcc_enabled", is_on);
}

static void collar_btn_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * wrapper = lv_obj_get_parent(btn);
    lv_obj_t *container = lv_obj_get_child(wrapper, 0);
    lv_obj_t *switch_group = lv_obj_get_child(container, 1);
    lv_obj_t *sw = lv_obj_get_child(switch_group, 1);
    
    // Defer all UI state logic to the centralized lock UI updater
    lever_update_system_lock_ui(sw);
    
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    lv_obj_t *tab = lv_obj_get_parent(frame);
    lv_obj_t *content = lv_obj_get_parent(tab);
    lv_obj_t *tv = lv_obj_get_parent(content);
    lv_obj_t *system_wrapper = lv_obj_get_parent(tv);
    
    state_manager_save(system_wrapper, config_manager_get_hash());
}

static bool interlocking_check(lv_obj_t *sw, bool target_state_thrown) {
    lv_obj_t *switch_group = lv_obj_get_parent(sw);
    lv_obj_t *container = lv_obj_get_parent(switch_group);
    lv_obj_t *wrapper = lv_obj_get_parent(container);
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    
    int lever_index = lv_obj_get_index(wrapper);
    const tab_def_t *tab_def = (const tab_def_t *)lv_obj_get_user_data(frame);
    if (!tab_def) return true; // No config attached, allow movement
    
    if (target_state_thrown) {
        // We are trying to throw this lever. Check if our own dependencies are met.
        const lever_def_t *my_def = &tab_def->levers[lever_index];
        for (int i = 0; i < MAX_INTERLOCKING_CONDITIONS; i++) {
            int target_idx = my_def->conditions[i].target_lever_index;
            if (target_idx < 0) break; // End of list
            if (target_idx >= tab_def->lever_count) continue; // Invalid
            
            lv_obj_t *target_wrapper = lv_obj_get_child(frame, target_idx);
            lv_obj_t *target_container = lv_obj_get_child(target_wrapper, 0);
            lv_obj_t *target_switch_group = lv_obj_get_child(target_container, 1);
            lv_obj_t *target_sw = lv_obj_get_child(target_switch_group, 1);
            
            bool is_target_thrown = lv_obj_has_state(target_sw, LV_STATE_CHECKED);
            if (is_target_thrown != my_def->conditions[i].required_state) {
                return false; // Dependency failed
            }
        }
    } else {
        // We are trying to normalize this lever. Check if any OTHER thrown lever requires us to be thrown!
        for (int i = 0; i < tab_def->lever_count; i++) {
            if (i == lever_index) continue;
            
            lv_obj_t *other_wrapper = lv_obj_get_child(frame, i);
            lv_obj_t *other_container = lv_obj_get_child(other_wrapper, 0);
            lv_obj_t *other_switch_group = lv_obj_get_child(other_container, 1);
            lv_obj_t *other_sw = lv_obj_get_child(other_switch_group, 1);
            
            if (lv_obj_has_state(other_sw, LV_STATE_CHECKED)) {
                const lever_def_t *other_def = &tab_def->levers[i];
                for (int c = 0; c < MAX_INTERLOCKING_CONDITIONS; c++) {
                    int target_idx = other_def->conditions[c].target_lever_index;
                    if (target_idx < 0) break;
                    
                    if (target_idx == lever_index && other_def->conditions[c].required_state == true) {
                        return false; // Another thrown lever requires us to remain thrown
                    }
                }
            }
        }
    }
    return true;
}

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
            if (manual_locked) {
                lv_label_set_text(collar_lbl, "LOCKED");
                lv_obj_clear_state(collar_btn, LV_STATE_DISABLED);
            } else if (system_locked) {
                lv_label_set_text(collar_lbl, "INTERLOCK");
                lv_obj_clear_state(collar_btn, LV_STATE_DISABLED);
            } else {
                lv_label_set_text(collar_lbl, "UNLOCKED");
                lv_obj_clear_state(collar_btn, LV_STATE_DISABLED);
            }
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

static void lever_switch_event_cb(lv_event_t * e) {
    lever_switch_refresh_cb(e);
    
    lv_obj_t * sw = lv_event_get_target(e);
    lv_obj_t * switch_group = lv_obj_get_parent(sw);
    lv_obj_t * container = lv_obj_get_parent(switch_group);
    lv_obj_t * wrapper = lv_obj_get_parent(container);
    lv_obj_t * frame = lv_obj_get_parent(wrapper);
    
    // Update all levers in this frame to reflect new dependencies
    lever_frame_update_system_locks(frame);
    
    // Save state to NVS
    lv_obj_t *tab = lv_obj_get_parent(frame);
    lv_obj_t *content = lv_obj_get_parent(tab);
    lv_obj_t *tv = lv_obj_get_parent(content);
    lv_obj_t *system_wrapper = lv_obj_get_parent(tv);
    
    state_manager_save(system_wrapper, config_manager_get_hash());

    // Produce LCC Event
    int lever_index = lv_obj_get_index(wrapper);
    const tab_def_t *tab_def = (const tab_def_t *)lv_obj_get_user_data(frame);
    if (tab_def && lever_index >= 0 && lever_index < tab_def->lever_count) {
        const lever_def_t *lever_def = &tab_def->levers[lever_index];
        const lever_system_config_t *system_config = config_manager_get_current();
        if (system_config && system_config->lcc_enabled && lever_def->lcc_enabled) {
            bool is_thrown = lv_obj_has_state(sw, LV_STATE_CHECKED);
            const char *event_str = is_thrown ? lever_def->lcc_event_reversed : lever_def->lcc_event_normal;
            if (strlen(event_str) > 0) {
                event_id_t eid = lcc_parse_event_id(event_str);
                if (eid != 0) {
                    openlcb_produce_event(eid);
                }
            }
        }
    }
}

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

static void lever_drawer_anim_ready_cb(lv_anim_t * a) {
    if (lever_info_drawer) {
        lv_obj_del(lever_info_drawer);
        lever_info_drawer = NULL;
    }
    if (lever_info_dimmer) {
        lv_obj_del(lever_info_dimmer);
        lever_info_dimmer = NULL;
    }
}

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

static void lever_drawer_click_cb(lv_event_t * e) {
    if (lever_info_drawer) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, lever_info_drawer);
        lv_anim_set_values(&a, 0, -lv_obj_get_height(lever_info_drawer));
        lv_anim_set_time(&a, 300);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
        lv_anim_set_ready_cb(&a, lever_drawer_anim_ready_cb);
        lv_anim_start(&a);
        
        lv_obj_remove_flag(lever_info_drawer, LV_OBJ_FLAG_CLICKABLE);
        if (lever_info_dimmer) lv_obj_remove_flag(lever_info_dimmer, LV_OBJ_FLAG_CLICKABLE);
    }
}

static void ui_add_drawer_row(lv_obj_t *parent, const char *key, const char *val, uint32_t color) {
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
}

static void brass_plate_click_cb(lv_event_t * e) {
    const lever_def_t *lever_def = lv_event_get_user_data(e);
    if (!lever_def || lever_info_drawer || lever_info_dimmer) return;
    
    lever_info_dimmer = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(lever_info_dimmer);
    lv_obj_set_size(lever_info_dimmer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(lever_info_dimmer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(lever_info_dimmer, LV_OPA_60, 0);
    lv_obj_add_flag(lever_info_dimmer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(lever_info_dimmer, lever_drawer_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(lever_info_dimmer, lever_drawer_click_cb, LV_EVENT_GESTURE, NULL);
    
    lever_info_drawer = lv_obj_create(lv_scr_act());
    lv_obj_set_size(lever_info_drawer, LV_PCT(100), LV_PCT(70));
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
    lv_obj_center(table);
    lv_obj_set_style_bg_opa(table, 0, 0);
    lv_obj_set_style_border_width(table, 0, 0);
    lv_obj_set_style_pad_all(table, 0, 0);
    lv_obj_set_layout(table, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(table, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_add_drawer_row(table, "Label:", lever_def->label, 0x8a6327);
    
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
    if (lever_def->lcc_enabled) {
        lv_obj_add_state(lcc_sw, LV_STATE_CHECKED);
    } else {
        lv_obj_clear_state(lcc_sw, LV_STATE_CHECKED);
    }
    
    // Store context for the callback
    lv_obj_t *plate = lv_event_get_target(e);
    lv_obj_t *header = lv_obj_get_parent(plate);
    lv_obj_t *container = lv_obj_get_parent(header);
    lv_obj_t *wrapper = lv_obj_get_parent(container);
    lv_obj_t *frame = lv_obj_get_parent(wrapper);
    lv_obj_t *tab = lv_obj_get_parent(frame);
    
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
    
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, lever_info_drawer);
    lv_obj_update_layout(lever_info_drawer);
    lv_coord_t h = lv_obj_get_height(lever_info_drawer);
    lv_obj_set_y(lever_info_drawer, -h);
    lv_anim_set_values(&a, -h, 0);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static lv_obj_t *lever_label_create(lv_obj_t *parent, const lever_def_t *lever_def, uint8_t label_height) {
    // Create the brass plate as a fixed-size container
    lv_obj_t *plate = lv_obj_create(parent);
    lv_obj_set_width(plate, lv_pct(100));
    lv_obj_set_height(plate, label_height); // Uniform height to accommodate multi-line text
    lv_obj_remove_flag(plate, LV_OBJ_FLAG_SCROLLABLE);
    
    // Make plate clickable
    lv_obj_add_flag(plate, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(plate, brass_plate_click_cb, LV_EVENT_CLICKED, (void *)lever_def);
    
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

static lv_obj_t *lever_switch_create(lv_obj_t *parent, uint32_t type_color) {
    lv_obj_t *obj = lv_switch_create(parent);
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

lv_obj_t *lever_create(lv_obj_t *parent, const void *lever_def_ptr, uint8_t label_lines, uint8_t label_line_height) {
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
    lever_label_create(header, lever_def, label_height);
    color_bar_create(header, type_color);
    
    // Group the switch and its labels tightly together
    lv_obj_t *switch_group = switch_container_create(container);
    
    lv_obj_t *normal_label = static_label_create(switch_group, up_text);
    lv_obj_t *sw = lever_switch_create(switch_group, type_color);
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
    lv_label_set_text(collar_lbl, "UNLOCKED");
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
            if (collar_lbl) lv_label_set_text(collar_lbl, "LOCKED");
        }
    } else {
        lv_obj_add_flag(sw, LV_OBJ_FLAG_CLICKABLE);
        if (indicator) lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        
        // Restore button to UNLOCKED and make it clickable again
        if (collar_btn) {
            lv_obj_clear_state(collar_btn, LV_STATE_CHECKED | LV_STATE_DISABLED);
            if (collar_lbl) lv_label_set_text(collar_lbl, "UNLOCKED");
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
