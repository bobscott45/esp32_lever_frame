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

#include "lever_frame.h"
#include "config_manager.h"

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

lv_obj_t *lever_frame_add_lever(lv_obj_t *frame, const lever_def_t *lever_def) {
    const tab_def_t *tab_def = (const tab_def_t *)lv_obj_get_user_data(frame);
    uint8_t lines = (tab_def && tab_def->label_lines > 0) ? tab_def->label_lines : 2;
    uint8_t height = (tab_def && tab_def->label_line_height > 0) ? tab_def->label_line_height : 18;
    
    lv_obj_t *lever = lever_create(frame, lever_def, lines, height);
    return lever;
}

lv_obj_t *lever_system_create(lv_obj_t *parent, const lever_system_config_t *config) {
    if (!config) return NULL;
    
    // Create the master column wrapper container
    lv_obj_t *wrapper = lv_obj_create(parent);
    lv_obj_set_size(wrapper, lv_pct(100), lv_pct(100));
    lv_obj_set_layout(wrapper, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(wrapper, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_style_pad_all(wrapper, 0, 0);
    lv_obj_set_style_pad_row(wrapper, 0, 0);
    lv_obj_set_style_border_width(wrapper, 0, 0);
    lv_obj_set_style_bg_opa(wrapper, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(wrapper, lv_color_hex(0x121212), 0); // deep black main background
    lv_obj_remove_flag(wrapper, LV_OBJ_FLAG_SCROLLABLE);

    // Create the tabview inside the wrapper
    lv_obj_t *tv = lv_tabview_create(wrapper);
    lv_obj_set_flex_grow(tv, 1); // take remaining vertical space
    lv_obj_set_width(tv, lv_pct(100));

    // Style the tabview content area to be dark
    lv_obj_t *content = lv_tabview_get_content(tv);
    lv_obj_set_style_bg_color(content, lv_color_hex(0x121212), 0);
    lv_obj_set_style_bg_opa(content, LV_OPA_COVER, 0);
    
    // Enable slow slide animation (500ms) to spread rendering load over many frames, preventing DMA starvation jitter
    lv_obj_set_style_anim_duration(tv, 500, 0);
    lv_obj_set_style_anim_duration(content, 500, 0);

    // Disable touch swiping/scrolling on the tabview content to prevent screen breakup
    lv_obj_remove_flag(content, LV_OBJ_FLAG_SCROLLABLE);
    lv_obj_set_scrollbar_mode(content, LV_SCROLLBAR_MODE_OFF);

    // Style the Tab Bar container
    lv_obj_t *tab_bar = lv_tabview_get_tab_bar(tv);
    lv_tabview_set_tab_bar_size(tv, 70); // Make tabs taller and much more prominent
    lv_obj_set_style_bg_color(tab_bar, lv_color_hex(0x151515), 0);
    lv_obj_set_style_border_color(tab_bar, lv_color_hex(0x8a6327), 0);
    lv_obj_set_style_border_width(tab_bar, 2, 0);
    lv_obj_set_style_border_side(tab_bar, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_pad_all(tab_bar, 0, 0);

    // Iterate through all defined tabs
    for (size_t t = 0; t < config->tab_count; t++) {
        const tab_def_t *tab_def = &config->tabs[t];
        
        // Add tab to the tabview
        lv_obj_t *tab = lv_tabview_add_tab(tv, tab_def->name);
        
        // Style tab page background
        lv_obj_set_style_bg_color(tab, lv_color_hex(0x121212), 0);
        lv_obj_set_style_bg_opa(tab, LV_OPA_COVER, 0);

        // Disable scrolling on individual tab pages to prevent swiping/gestures
        lv_obj_remove_flag(tab, LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_set_scrollbar_mode(tab, LV_SCROLLBAR_MODE_OFF);
        
        // Create the lever frame inside this tab
        lv_obj_t *frame = lever_frame_create(tab);
        lv_obj_set_user_data(frame, (void *)tab_def);
        
        // Add all defined levers to the frame
        for (size_t l = 0; l < tab_def->lever_count; l++) {
            const lever_def_t *lever_def = &tab_def->levers[l];
            lever_frame_add_lever(frame, lever_def);
        }
        
        // Evaluate initial system locks for all levers in the frame
        lever_frame_update_system_locks(frame);
    }

    // Apply custom styling to the tab buttons (must be done after tabs are added!)
    uint32_t tab_count = lv_tabview_get_tab_count(tv);
    for (uint32_t i = 0; i < tab_count; i++) {
        lv_obj_t *btn = lv_tabview_get_tab_button(tv, i);
        if (btn) {
            // Unselected buttons: lighter gray background, darker text
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x353535), LV_STATE_DEFAULT);
            lv_obj_set_style_text_color(btn, lv_color_hex(0x777777), LV_STATE_DEFAULT); // Darkened so it's not brighter than selected
            lv_obj_set_style_border_color(btn, lv_color_hex(0x555555), LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(btn, 1, LV_STATE_DEFAULT);
            lv_obj_set_style_radius(btn, 4, LV_STATE_DEFAULT);
            lv_obj_set_style_pad_all(btn, 6, LV_STATE_DEFAULT); // Less padding to fit all tabs
            
            extern const lv_font_t lv_font_montserrat_16;
            lv_obj_set_style_text_font(btn, &lv_font_montserrat_16, LV_STATE_DEFAULT);
            
            // Selected buttons: dark background, bright brass text for prominence
            lv_obj_set_style_bg_color(btn, lv_color_hex(0x1a1a1a), LV_STATE_CHECKED);
            lv_obj_set_style_text_color(btn, lv_color_hex(0xd4a34b), LV_STATE_CHECKED); // Bright glowing brass text
            lv_obj_set_style_border_color(btn, lv_color_hex(0x8a6327), LV_STATE_CHECKED); // Deep antique brass border
            lv_obj_set_style_border_width(btn, 2, LV_STATE_CHECKED); // Thicker border
        }
    }
    
    return wrapper;
}
