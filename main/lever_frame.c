#include "lever_frame.h"

lv_obj_t *lever_frame_create(lv_obj_t *parent) {
    lv_obj_t *frame = lv_obj_create(parent);
    
    // Fill the parent (e.g. the tab inside the tabview)
    lv_obj_set_size(frame, lv_pct(100), lv_pct(100));
    
    // Make the frame background transparent and remove borders
    lv_obj_set_style_bg_opa(frame, 0, LV_PART_MAIN);
    lv_obj_set_style_border_width(frame, 0, LV_PART_MAIN);
    
    // Layout levers in a horizontal row
    lv_obj_set_layout(frame, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(frame, LV_FLEX_FLOW_ROW);
    
    // Align levers to the center of the frame vertically and horizontally
    lv_obj_set_flex_align(frame, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    
    // Set a very tight gap between levers, similar to a physical frame
    lv_obj_set_style_pad_column(frame, 4, LV_PART_MAIN);
    
    // Explicitly set a 4px margin on the top and bottom so it doesn't feel squashed
    lv_obj_set_style_pad_top(frame, 4, LV_PART_MAIN);
    lv_obj_set_style_pad_bottom(frame, 4, LV_PART_MAIN);
    
    // Crucially, zero out the parent tab's default padding so the frame can touch the absolute edges!
    lv_obj_set_style_pad_all(parent, 0, LV_PART_MAIN);
    
    // Disable scrolling as requested by the user
    lv_obj_remove_flag(frame, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(frame, LV_SCROLLBAR_MODE_OFF);
    
    return frame;
}

lv_obj_t *lever_frame_add_lever(lv_obj_t *frame, const char *label_text, lever_type_t type) {
    // Simply delegate to the powerful lever factory we built!
    return lever_create(frame, label_text, type);
}

lv_obj_t *lever_system_create(lv_obj_t *parent, const lever_system_config_t *config) {
    if (!config) return NULL;
    
    // Create the tabview
    lv_obj_t *tv = lv_tabview_create(parent);
    
    // Iterate through all defined tabs
    for (size_t t = 0; t < config->tab_count; t++) {
        const tab_def_t *tab_def = &config->tabs[t];
        
        // Add tab to the tabview
        lv_obj_t *tab = lv_tabview_add_tab(tv, tab_def->name);
        
        // Create the lever frame inside this tab
        lv_obj_t *frame = lever_frame_create(tab);
        
        // Add all defined levers to the frame
        for (size_t l = 0; l < tab_def->lever_count; l++) {
            const lever_def_t *lever_def = &tab_def->levers[l];
            lever_frame_add_lever(frame, lever_def->label, lever_def->type);
        }
    }
    
    return tv;
}
