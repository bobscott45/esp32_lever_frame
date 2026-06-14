#include "bsp/board.h"
#include "bsp/lvgl_port.h"
#include "lever.h"
#include "lever_frame.h"
#include "config_manager.h"
#include "web_server.h"
#include "esp_log.h"
#include "openlcb_integration.h"
#include <string.h>

static const char *TAG = "main";
lv_obj_t *system_tabview = NULL;

static lv_obj_t *remote_config_overlay = NULL;
static lv_timer_t *remote_config_timer = NULL;

static void remote_config_timeout_cb(lv_timer_t *timer) {
    if (remote_config_overlay) {
        lv_obj_del(remote_config_overlay);
        remote_config_overlay = NULL;
    }
    remote_config_timer = NULL;
}

void ui_show_remote_config_overlay(void) {
    if (lvgl_port_lock(0)) {
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
        
        lvgl_port_unlock();
    }
}

static lv_obj_t *info_overlay = NULL;
static lv_obj_t *info_dimmer = NULL;

static void info_overlay_close_immediate(void) {
    if (info_overlay) {
        lv_obj_del(info_overlay);
        info_overlay = NULL;
    }
    if (info_dimmer) {
        lv_obj_del(info_dimmer);
        info_dimmer = NULL;
    }
}

static void info_overlay_anim_ready_cb(lv_anim_t * a) {
    info_overlay_close_immediate();
}

static void settings_global_lcc_cb(lv_event_t *e) {
    lv_obj_t *sw = lv_event_get_target(e);
    bool is_on = lv_obj_has_state(sw, LV_STATE_CHECKED);
    config_manager_update_global_bool("lcc_enabled", is_on);
}

static void settings_policy_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int policy = lv_dropdown_get_selected(dd);
    config_manager_update_global_int("conflict_policy", policy);
}

static void settings_startup_cb(lv_event_t *e) {
    lv_obj_t *dd = lv_event_get_target(e);
    int selected = lv_dropdown_get_selected(dd);
    config_manager_update_global_bool("restore_last_state", (selected == 0));
}

static void info_overlay_click_cb(lv_event_t * e) {
    if (info_overlay) {
        lv_anim_t a;
        lv_anim_init(&a);
        lv_anim_set_var(&a, info_overlay);
        lv_anim_set_values(&a, 0, -lv_obj_get_height(info_overlay));
        lv_anim_set_time(&a, 300);
        lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
        lv_anim_set_path_cb(&a, lv_anim_path_ease_in);
        lv_anim_set_ready_cb(&a, info_overlay_anim_ready_cb);
        lv_anim_start(&a);
        
        lv_obj_remove_flag(info_overlay, LV_OBJ_FLAG_CLICKABLE);
        if (info_dimmer) lv_obj_remove_flag(info_dimmer, LV_OBJ_FLAG_CLICKABLE);
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

static void ui_show_info_overlay(void) {
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
    web_server_get_sta_ip(sta_ip, sizeof(sta_ip));
    
    ui_add_drawer_row(table, "Wi-Fi AP IP:", "192.168.4.1", 0x0078D7);
    ui_add_drawer_row(table, "Home Wi-Fi IP:", sta_ip, 0x0078D7);
    ui_add_drawer_row(table, "LCC TCP Port:", "12021", 0x0078D7);
    
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
    lv_dropdown_set_options(pol_dd, "Strict Local\nOverride Allowed");
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
    
    // Slide down animation
    lv_anim_t a;
    lv_anim_init(&a);
    lv_anim_set_var(&a, info_overlay);
    lv_obj_update_layout(info_overlay);
    lv_coord_t h = lv_obj_get_height(info_overlay);
    lv_obj_set_y(info_overlay, -h);
    lv_anim_set_values(&a, -h, 0);
    lv_anim_set_time(&a, 300);
    lv_anim_set_exec_cb(&a, (lv_anim_exec_xcb_t)lv_obj_set_y);
    lv_anim_set_path_cb(&a, lv_anim_path_ease_out);
    lv_anim_start(&a);
}

static void screen_gesture_cb(lv_event_t * e) {
    lv_dir_t dir = lv_indev_get_gesture_dir(lv_indev_get_act());
    if(dir == LV_DIR_BOTTOM) {
        ui_show_info_overlay();
    }
}

static bool ui_rebuild_requested = false;

static void rebuild_ui_timer_cb(lv_timer_t *timer)
{
    static int state = 0;
    
    if (state == 0) {
        if (!ui_rebuild_requested) return;
        ui_rebuild_requested = false;
        
        // Step 1: Turn off backlight and delete old UI to prevent tearing
        ESP_LOGI(TAG, "Config update: Turning off backlight and clearing UI");
        waveshare_rgb_lcd_bl_off();
        
        info_overlay_close_immediate();
        lever_close_all_drawers();
        
        if (system_tabview) {
            lv_obj_del(system_tabview);
            system_tabview = NULL;
        }
        
        // Safe to free old configuration memory now that UI objects using it are deleted
        config_manager_free_pending();
        
        state = 1;
    } else if (state == 1) {
        // Step 2: Create new UI while screen is black
        ESP_LOGI(TAG, "Config update: Creating new UI");
        const lever_system_config_t *curr_config = config_manager_get_current();
        system_tabview = lever_system_create(lv_scr_act(), curr_config);
        
        state = 2;
    } else {
        // Step 3: Turn backlight back on
        ESP_LOGI(TAG, "Config update: Restoring backlight");
        waveshare_rgb_lcd_bl_on();
        
        state = 0;
    }
}

static void on_configuration_changed(void)
{
    ESP_LOGI(TAG, "Configuration changed! Scheduling UI rebuild...");
    ui_rebuild_requested = true;
}

void app_main(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;

    // 1. Initialize hardware drivers and LVGL first
    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init(&panel, &touch));
    ESP_ERROR_CHECK(lvgl_port_init(panel, touch));

    // Show a Loading Splash Screen immediately
    lv_obj_t *loading_scr = NULL;
    if (lvgl_port_lock(-1)) {
        loading_scr = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(loading_scr);
        lv_obj_set_size(loading_scr, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(loading_scr, lv_color_hex(0x121212), 0);
        lv_obj_set_style_bg_opa(loading_scr, LV_OPA_COVER, 0);
        
        lv_obj_t *loading_label = lv_label_create(loading_scr);
        lv_label_set_text(loading_label, "Connecting to Wi-Fi...");
        lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(loading_label);
        lvgl_port_unlock();
    }
    
    // Give the RGB panel time to lock onto the sync signals before turning the backlight on.
    // 500ms ensures the panel sync is perfectly stable, preventing horizontal image wrapping.
    vTaskDelay(pdMS_TO_TICKS(500));
    ESP_ERROR_CHECK(waveshare_rgb_lcd_bl_on());

    // 2. Initialize configuration manager and load dynamic config
    ESP_ERROR_CHECK(config_manager_init());
    config_manager_set_on_change(on_configuration_changed);

    // 3. Initialize Wi-Fi softAP and start the configuration web server
    // (This triggers connection to Home Wi-Fi)
    ESP_ERROR_CHECK(web_server_start());

    // 4. Initialize OpenLCB stack (This BLOCKS waiting for Wi-Fi)
    openlcb_integration_init();

    // 5. Build final UI from configuration
    if (lvgl_port_lock(-1)) {
        if (loading_scr) {
            lv_obj_del(loading_scr);
        }
        
        lv_timer_t *rebuild_timer = lv_timer_create(rebuild_ui_timer_cb, 50, NULL);
        lv_timer_set_repeat_count(rebuild_timer, -1);
        
        const lever_system_config_t *curr_config = config_manager_get_current();
        system_tabview = lever_system_create(lv_scr_act(), curr_config);
        
        // Restore lever states if configuration matches
        #include "state_manager.h"
        if (curr_config->restore_last_state) {
            state_manager_load_and_apply(system_tabview, config_manager_get_hash());
        }
        
        // Register gesture on the active screen to pull down the Wi-Fi info overlay
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
        lv_obj_add_event_cb(lv_scr_act(), screen_gesture_cb, LV_EVENT_GESTURE, NULL);
        
        lvgl_port_unlock();
    }
}
