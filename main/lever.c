//
// Created by robert on 13/06/2026.
//

#include "lever.h"
#include "display/lv_display.h"



static void collar_btn_event_cb(lv_event_t * e) {
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * wrapper = lv_obj_get_parent(btn);
    lv_obj_t * lbl = lv_obj_get_child(btn, 0);
    
    if(lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        if(lbl) lv_label_set_text(lbl, "LOCKED");
    } else {
        if(lbl) lv_label_set_text(lbl, "UNLOCKED");
    }
    
    lv_obj_t *container = lv_obj_get_child(wrapper, 0);
    lv_obj_t *switch_group = lv_obj_get_child(container, 1);
    lv_obj_t *sw = lv_obj_get_child(switch_group, 1);
    lv_obj_t *indicator = lv_obj_get_child(sw, 0);
    
    if (lv_obj_has_state(btn, LV_STATE_CHECKED)) {
        lv_obj_add_state(sw, LV_STATE_DISABLED);
        if(indicator) lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    } else {
        lv_obj_clear_state(sw, LV_STATE_DISABLED);
        if(indicator) lv_obj_add_flag(indicator, LV_OBJ_FLAG_HIDDEN);
    }
}

static void lever_switch_event_cb(lv_event_t * e) {
    lv_obj_t * sw = lv_event_get_target(e);
    lv_obj_t * switch_group = lv_obj_get_parent(sw);
    // Indices are back to 0 and 2 because collar_btn was moved out!
    lv_obj_t * normal_label = lv_obj_get_child(switch_group, 0);
    lv_obj_t * thrown_label = lv_obj_get_child(switch_group, 2);

    if(lv_obj_has_state(sw, LV_STATE_CHECKED)) {
        // Thrown state
        lv_obj_set_style_text_color(normal_label, lv_color_hex(0x888888), LV_PART_MAIN);
        lv_obj_set_style_text_color(thrown_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
    } else {
        // Normal state
        lv_obj_set_style_text_color(normal_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN);
        lv_obj_set_style_text_color(thrown_label, lv_color_hex(0x888888), LV_PART_MAIN);
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

static lv_obj_t *lever_label_create(lv_obj_t *parent, const char *label_text) {
    // Create the brass plate as a fixed-size container
    lv_obj_t *plate = lv_obj_create(parent);
    lv_obj_set_width(plate, lv_pct(100));
    lv_obj_set_height(plate, LEVER_LABEL_HEIGHT); // Uniform height to accommodate multi-line text
    lv_obj_remove_flag(plate, LV_OBJ_FLAG_SCROLLABLE);
    
    // Style the plate
    lv_obj_set_style_bg_color(plate, lv_color_hex(LEVER_LABEL_BG_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(plate, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_border_width(plate, 0, LV_PART_MAIN);
    lv_obj_set_style_radius(plate, 0, LV_PART_MAIN);
    lv_obj_set_style_pad_all(plate, 0, LV_PART_MAIN);
    
    // Create the text label and center it perfectly inside the plate
    lv_obj_t *label = lv_label_create(plate);
    lv_label_set_text(label, label_text);
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
    lv_obj_set_style_bg_color(obj, lv_color_hex(LEVER_STATE_NORMAL_COLOR), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Keep the switch track a dark void in both states
    lv_obj_set_style_bg_color(obj, lv_color_hex(LEVER_STATE_NORMAL_COLOR), LV_PART_INDICATOR | LV_STATE_CHECKED);
    
    // Color code only the "button" (knob) to match the lever type
    lv_obj_set_style_bg_color(obj, lv_color_hex(type_color), LV_PART_KNOB | LV_STATE_DEFAULT);
    
    // Remove default knob outline
    lv_obj_set_style_border_width(obj, 0, LV_PART_KNOB | LV_STATE_DEFAULT);
    
    // If it's a black points lever, add a crisp silver border so it pops against the dark slot
    if (type_color == 0x000000) {
        lv_obj_set_style_border_color(obj, lv_color_hex(0xaaaaaa), LV_PART_KNOB | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 2, LV_PART_KNOB | LV_STATE_DEFAULT);
    }
    
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
        lv_obj_set_style_border_color(obj, lv_color_hex(0xaaaaaa), LV_PART_MAIN | LV_STATE_DEFAULT);
        lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
    }
    
    lv_obj_set_style_radius(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_remove_flag(obj, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
    return obj;
}

lv_obj_t *lever_create(lv_obj_t *parent, const char *label_text, lever_type_t type) {
    uint32_t type_color;
    const char *up_text;
    const char *down_text;
    
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
    lever_label_create(header, label_text);
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
    lv_obj_set_style_radius(collar_btn, LV_RADIUS_CIRCLE, LV_PART_MAIN);
    lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0x333333), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_color(collar_btn, lv_color_hex(0xFF0000), LV_PART_MAIN | LV_STATE_CHECKED);
    
    // Prevent the button from fading out when system locks it
    lv_obj_set_style_bg_opa(collar_btn, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DISABLED);
    lv_obj_add_event_cb(collar_btn, collar_btn_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Label inside the collar button
    lv_obj_t *collar_lbl = lv_label_create(collar_btn);
    lv_obj_align(collar_lbl, LV_ALIGN_CENTER, 0, 0);
    lv_label_set_text(collar_lbl, "UNLOCKED");
    lv_obj_set_style_text_color(collar_lbl, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_opa(collar_lbl, LV_OPA_COVER, LV_PART_MAIN | LV_STATE_DISABLED);
    
    // Set initial label highlight state (Normal = Bright, Thrown = Dim)
    lv_obj_set_style_text_color(normal_label, lv_color_hex(0xFFFFFF), LV_PART_MAIN | LV_STATE_DEFAULT);
    lv_obj_set_style_text_color(thrown_label, lv_color_hex(0x888888), LV_PART_MAIN | LV_STATE_DEFAULT);
    
    // Add event callback to toggle highlight on switch toggle
    lv_obj_add_event_cb(sw, lever_switch_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
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
        lv_obj_add_state(sw, LV_STATE_DISABLED);
        if (indicator) lv_obj_clear_flag(indicator, LV_OBJ_FLAG_HIDDEN);
        
        // Force the button to LOCKED and make it unclickable
        if (collar_btn) {
            lv_obj_add_state(collar_btn, LV_STATE_CHECKED | LV_STATE_DISABLED);
            if (collar_lbl) lv_label_set_text(collar_lbl, "LOCKED");
        }
    } else {
        lv_obj_clear_state(sw, LV_STATE_DISABLED);
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
