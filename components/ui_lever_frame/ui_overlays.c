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
 * @file      ui_overlays.c
 * @brief     Implementation of ui_overlays.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "ui_overlays.h"
#include "lvgl.h"
#include "ui_porting.h"
#include "config_manager.h"
#include "esp_netif.h"
#include "esp_app_desc.h"
#include <string.h>

static lv_obj_t *remote_config_overlay = NULL;
static lv_timer_t *remote_config_timer = NULL;

static lv_obj_t *info_overlay = NULL;
static lv_obj_t *info_dimmer = NULL;

/**
 * @brief  Callback for the remote config timeout.
 *
 * Deletes the remote configuration overlay once the 5 second timeout expires.
 *
 * @param[in]  timer   Pointer to the LVGL timer object.
 */
static void remote_config_timeout_cb(lv_timer_t *timer) {
    if (remote_config_overlay) {
        lv_obj_del(remote_config_overlay);
        remote_config_overlay = NULL;
    }
    remote_config_timer = NULL;
}

void ui_show_remote_config_overlay(void) {
    if (ui_port_lock(0)) {
        if (!remote_config_overlay) {
            remote_config_overlay = lv_obj_create(lv_scr_act());
            lv_obj_set_size(remote_config_overlay, LV_PCT(100), LV_PCT(100));
            lv_obj_set_style_bg_color(remote_config_overlay, lv_color_hex(0x000000), 0);
            lv_obj_set_style_bg_opa(remote_config_overlay, LV_OPA_80, 0);
            lv_obj_set_style_border_width(remote_config_overlay, 0, 0);
            lv_obj_set_style_radius(remote_config_overlay, 0, 0);
            
            lv_obj_t *label = lv_label_create(remote_config_overlay);
            lv_label_set_text(label, "Remote Configuration\nIn Progress...");
            lv_obj_set_style_text_color(label, lv_color_hex(0xFFFFFF), 0);
            lv_obj_set_style_text_align(label, LV_TEXT_ALIGN_CENTER, 0);
            lv_obj_center(label);
            
            lv_obj_add_flag(remote_config_overlay, LV_OBJ_FLAG_CLICKABLE);
        }
        
        if (remote_config_timer) {
            lv_timer_del(remote_config_timer);
        }
        remote_config_timer = lv_timer_create(remote_config_timeout_cb, 5000, NULL);
        lv_timer_set_repeat_count(remote_config_timer, 1);
        
        ui_port_unlock();
    }
}

void info_overlay_close_immediate(void) {
    if (info_overlay) {
        lv_obj_del(info_overlay);
        info_overlay = NULL;
    }
    if (info_dimmer) {
        lv_obj_del(info_dimmer);
        info_dimmer = NULL;
    }
}

/**
 * @brief  Callback for the global LCC toggle switch.
 *
 * Updates the global configuration for LCC master when the switch is toggled.
 *
 * @param[in]  e   Pointer to the LVGL event.
 */
static void settings_global_lcc_cb(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    config_manager_update_global_bool("lcc_enabled", is_on);
}

/**
 * @brief  Callback for the interlocking conflict policy dropdown.
 *
 * Updates the global interlocking conflict policy when changed.
 *
 * @param[in]  e   Pointer to the LVGL event.
 */
static void settings_policy_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int policy = lv_dropdown_get_selected(dd);
    config_manager_update_global_int("conflict_policy", policy);
}

/**
 * @brief  Callback for the startup mode dropdown.
 *
 * Updates the global restore_last_state configuration.
 *
 * @param[in]  e   Pointer to the LVGL event.
 */
static void settings_startup_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int selected = lv_dropdown_get_selected(dd);
    config_manager_update_global_bool("restore_last_state", (selected == 0));
}

/**
 * @brief  Callback for closing the info overlay via click.
 *
 * Closes the information drawer when the user taps outside of it or swipes.
 *
 * @param[in]  e   Pointer to the LVGL event.
 */
static void info_overlay_click_cb(lv_event_t * e) {
    if (info_overlay) {
        info_overlay_close_immediate();
        
        if (info_dimmer) {
            lv_obj_remove_flag(info_dimmer, LV_OBJ_FLAG_CLICKABLE);
            lv_obj_del(info_dimmer);
            info_dimmer = NULL;
        }
    }
}

/**
 * @brief  Helper to add a row to the system information drawer.
 *
 * Creates a two-column layout row with a key and a value.
 *
 * @param[in]  parent  Pointer to the parent LVGL object.
 * @param[in]  key     The label for the left column.
 * @param[in]  val     The value for the right column.
 * @param[in]  color   The HEX color for the left column label.
 */
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

void ui_show_info_overlay(void) {
    if (info_overlay || info_dimmer) return;
    
    info_dimmer = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(info_dimmer);
    lv_obj_set_size(info_dimmer, LV_PCT(100), LV_PCT(100));
    lv_obj_set_style_bg_color(info_dimmer, lv_color_hex(0x000000), 0);
    lv_obj_set_style_bg_opa(info_dimmer, LV_OPA_60, 0);
    lv_obj_add_flag(info_dimmer, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(info_dimmer, info_overlay_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(info_dimmer, info_overlay_click_cb, LV_EVENT_GESTURE, NULL);

    info_overlay = lv_obj_create(lv_scr_act());
    lv_obj_set_size(info_overlay, LV_PCT(100), LV_PCT(95));
    lv_obj_align(info_overlay, LV_ALIGN_TOP_MID, 0, 0);
    
    // Theme
    lv_obj_set_style_bg_color(info_overlay, lv_color_hex(0x222222), 0);
    lv_obj_set_style_bg_opa(info_overlay, LV_OPA_COVER, 0);
    lv_obj_set_style_border_width(info_overlay, 0, 0);
    lv_obj_set_style_radius(info_overlay, 0, 0);
    lv_obj_set_style_border_side(info_overlay, LV_BORDER_SIDE_BOTTOM, 0);
    lv_obj_set_style_border_width(info_overlay, 4, 0);
    lv_obj_set_style_border_color(info_overlay, lv_color_hex(0x0078D7), 0);
    
    // Flex Layout
    lv_obj_set_layout(info_overlay, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(info_overlay, LV_FLEX_FLOW_COLUMN);
    lv_obj_set_flex_align(info_overlay, LV_FLEX_ALIGN_SPACE_BETWEEN, LV_FLEX_ALIGN_CENTER, LV_FLEX_ALIGN_CENTER);
    lv_obj_set_style_pad_all(info_overlay, 15, 0);
    
    const lever_system_config_t *curr_config = config_manager_get_current();
    const char *ap_password = (curr_config && curr_config->wifi_password && strlen(curr_config->wifi_password) > 0) ? curr_config->wifi_password : "signalman";

    lv_obj_t *title = lv_label_create(info_overlay);
    lv_label_set_text(title, "System Information");
    lv_obj_set_style_text_color(title, lv_color_hex(0xFFFFFF), 0);
    
    lv_obj_t *table = lv_obj_create(info_overlay);
    lv_obj_set_size(table, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(table, 0, 0);
    lv_obj_set_style_border_width(table, 0, 0);
    lv_obj_set_style_pad_all(table, 0, 0);
    lv_obj_set_layout(table, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(table, LV_FLEX_FLOW_COLUMN);
    lv_obj_remove_flag(table, LV_OBJ_FLAG_SCROLLABLE);
    
    ui_add_drawer_row(table, "Wi-Fi AP:", "Lever-Frame-Config", 0x0078D7);
    ui_add_drawer_row(table, "Password:", ap_password, 0x0078D7);
    char sta_ip[32] = "Not Connected";
    esp_netif_t *sta_netif = esp_netif_get_handle_from_ifkey("WIFI_STA_DEF");
    if (sta_netif) {
        esp_netif_ip_info_t ip_info;
        if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
            if (ip_info.ip.addr != 0) {
                snprintf(sta_ip, sizeof(sta_ip), IPSTR, IP2STR(&ip_info.ip));
            } else {
                strncpy(sta_ip, "Connecting...", sizeof(sta_ip));
            }
        }
    }
    
    ui_add_drawer_row(table, "Wi-Fi AP IP:", "192.168.4.1", 0x0078D7);
    ui_add_drawer_row(table, "Home Wi-Fi IP:", sta_ip, 0x0078D7);
    ui_add_drawer_row(table, "LCC TCP Port:", "12021", 0x0078D7);
    
    const esp_app_desc_t *app_desc = esp_app_get_description();
    ui_add_drawer_row(table, "System Version:", app_desc->version, 0x0078D7);
    
    lv_obj_t *spacer = lv_obj_create(table);
    lv_obj_remove_style_all(spacer);
    lv_obj_set_size(spacer, 10, 5);
    
    // Interactive LCC Master
    lv_obj_t *lcc_row = lv_obj_create(table);
    lv_obj_set_size(lcc_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(lcc_row, 0, 0);
    lv_obj_set_style_border_width(lcc_row, 0, 0);
    lv_obj_set_style_pad_all(lcc_row, 4, 0);
    lv_obj_set_layout(lcc_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(lcc_row, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(lcc_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *lcc_lbl = lv_label_create(lcc_row);
    lv_obj_set_width(lcc_lbl, 160);
    lv_label_set_text(lcc_lbl, "LCC Master:");
    lv_obj_set_style_text_color(lcc_lbl, lv_color_hex(0x0078D7), 0);

    lv_obj_t *lcc_sw = lv_switch_create(lcc_row);
    lv_obj_set_style_anim_time(lcc_sw, 0, 0);
    if (curr_config->lcc_enabled) lv_obj_add_state(lcc_sw, LV_STATE_CHECKED);
    lv_obj_add_event_cb(lcc_sw, settings_global_lcc_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    // Interactive Override Policy
    lv_obj_t *pol_row = lv_obj_create(table);
    lv_obj_set_size(pol_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(pol_row, 0, 0);
    lv_obj_set_style_border_width(pol_row, 0, 0);
    lv_obj_set_style_pad_all(pol_row, 4, 0);
    lv_obj_set_layout(pol_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(pol_row, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(pol_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *pol_lbl = lv_label_create(pol_row);
    lv_obj_set_width(pol_lbl, 160);
    lv_label_set_text(pol_lbl, "Override Policy:");
    lv_obj_set_style_text_color(pol_lbl, lv_color_hex(0x0078D7), 0);

    lv_obj_t *pol_dd = lv_dropdown_create(pol_row);
    lv_obj_set_width(pol_dd, 180);
    lv_dropdown_set_options(pol_dd, "Strict Local\nOverride Allowed\nAccept & Warn");
    lv_dropdown_set_selected(pol_dd, curr_config->conflict_policy);
    lv_obj_add_event_cb(pol_dd, settings_policy_cb, LV_EVENT_VALUE_CHANGED, NULL);

    // Interactive Startup Mode
    lv_obj_t *start_row = lv_obj_create(table);
    lv_obj_set_size(start_row, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
    lv_obj_set_style_bg_opa(start_row, 0, 0);
    lv_obj_set_style_border_width(start_row, 0, 0);
    lv_obj_set_style_pad_all(start_row, 4, 0);
    lv_obj_set_layout(start_row, LV_LAYOUT_FLEX);
    lv_obj_set_flex_flow(start_row, LV_FLEX_FLOW_ROW);
    lv_obj_remove_flag(start_row, LV_OBJ_FLAG_SCROLLABLE);

    lv_obj_t *start_lbl = lv_label_create(start_row);
    lv_obj_set_width(start_lbl, 160);
    lv_label_set_text(start_lbl, "Startup Mode:");
    lv_obj_set_style_text_color(start_lbl, lv_color_hex(0x0078D7), 0);

    lv_obj_t *start_dd = lv_dropdown_create(start_row);
    lv_obj_set_width(start_dd, 180);
    lv_dropdown_set_options(start_dd, "Restore Last\nSafe Default");
    lv_dropdown_set_selected(start_dd, curr_config->restore_last_state ? 0 : 1);
    lv_obj_add_event_cb(start_dd, settings_startup_cb, LV_EVENT_VALUE_CHANGED, NULL);
    
    lv_obj_t *footer = lv_label_create(info_overlay);
    lv_label_set_text(footer, "(Swipe Up or Tap to dismiss)");
    lv_obj_set_style_text_color(footer, lv_color_hex(0x888888), 0);
    
    lv_obj_add_flag(info_overlay, LV_OBJ_FLAG_CLICKABLE);
    lv_obj_add_event_cb(info_overlay, info_overlay_click_cb, LV_EVENT_CLICKED, NULL);
    lv_obj_add_event_cb(info_overlay, info_overlay_click_cb, LV_EVENT_GESTURE, NULL);
}

/**
 * @brief  Callback for screen swipe gestures.
 *
 * Detects downward swipes to open the system information drawer.
 *
 * @param[in]  e   Pointer to the LVGL event.
 */
static void screen_gesture_cb(lv_event_t * e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(dir == LV_DIR_BOTTOM) {
        ui_show_info_overlay();
    }
}

void ui_overlays_register_gestures(void) {
    lv_obj_add_event_cb(lv_scr_act(), screen_gesture_cb, LV_EVENT_GESTURE, NULL);
}
