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
 * @file      main.c
 * @brief     Implementation of main.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "display_hal.h"
#include "ui_porting.h"
#include "ui_overlays.h"
#include "lever.h"
#include "lever_frame.h"
#include "config_manager.h"
#include "web_server.h"
#include "esp_log.h"
#include "openlcb_integration.h"
#include "controller.h"
#include "state_manager.h"
#include "system_events.h"
#include "esp_app_desc.h"
#include "esp_event.h"
#include <string.h>

static const char *TAG = "main";
lv_obj_t *system_tabview = NULL;

/**
 * @brief  Handle lever state change events.
 *
 * This function is invoked when a lever state changes. It persists the new state 
 * to NVS and triggers any associated OpenLCB events.
 *
 * @param[in]  handler_args  Pointer to handler arguments (unused).
 * @param[in]  base          Event base.
 * @param[in]  id            Event ID.
 * @param[in]  event_data    Pointer to event data.
 */
static void on_controller_state_changed(void* handler_args, esp_event_base_t base, int32_t id, void* event_data) {
    if (id != EVENT_LEVER_STATE_CHANGED) return;
    
    event_lever_state_t *ev = (event_lever_state_t *)event_data;
    int tab_index = ev->tab_index;
    int lever_index = ev->lever_index;
    bool new_state = ev->new_state;

    // Save state to NVS (synchronized with UI lock)
    if (ui_port_lock(-1)) {
        state_manager_save(config_manager_get_hash());
        ui_port_unlock();
    }
    
    // Fire OpenLCB event
    const lever_system_config_t *system_config = config_manager_get_current();
    if (system_config && system_config->lcc_enabled) {
        if (tab_index >= 0 && tab_index < system_config->tab_count) {
            const tab_def_t *tab_def = &system_config->tabs[tab_index];
            if (lever_index >= 0 && lever_index < tab_def->lever_count) {
                const lever_def_t *lever_def = &tab_def->levers[lever_index];
                if (lever_def->lcc_enabled) {
                    const char *event_str = new_state ? lever_def->lcc_event_reversed : lever_def->lcc_event_normal;
                    if (event_str && strlen(event_str) > 0) {
                        event_id_t eid = lcc_parse_event_id(event_str);
                        if (eid != 0) {
                            openlcb_produce_event(eid);
                        }
                    }
                }
            }
        }
    }
}

static bool display_is_sleeping = false;

/**
 * @brief  Callback for the display sleep timer.
 *
 * Periodically checks LVGL's inactive time and turns off the backlight if it 
 * exceeds the configured timeout. Restores the backlight on activity.
 */
static void display_sleep_timer_cb(lv_timer_t *timer)
{
    const lever_system_config_t *curr_config = config_manager_get_current();
    if (!curr_config) return;
    
    int timeout_ms = curr_config->display_sleep_timeout_ms;
    if (timeout_ms <= 0) {
        // Sleep disabled
        if (display_is_sleeping) {
            display_hal_backlight_on();
            display_is_sleeping = false;
        }
        return;
    }
    
    uint32_t inactive_time = lv_disp_get_inactive_time(NULL);
    
    if (inactive_time > timeout_ms && !display_is_sleeping) {
        ESP_LOGI(TAG, "Display inactive for %lu ms. Sleeping display.", (unsigned long)inactive_time);
        display_hal_backlight_off();
        display_is_sleeping = true;
    } else if (inactive_time < timeout_ms && display_is_sleeping) {
        ESP_LOGI(TAG, "Activity detected. Waking display.");
        display_hal_backlight_on();
        display_is_sleeping = false;
    }
}

static bool ui_rebuild_requested = false;

/**
 * @brief  Callback for the UI rebuild timer.
 *
 * Periodically checks if a UI rebuild was requested due to a configuration change.
 * If requested, it tears down the existing UI, loads the new configuration, 
 * and builds a new UI.
 *
 * @param[in]  timer   Pointer to the LVGL timer.
 */
static void rebuild_ui_timer_cb(lv_timer_t *timer)
{
    static int state = 0;
    
    if (state == 0) {
        if (!ui_rebuild_requested) return;
        ui_rebuild_requested = false;
        
        // Turn off backlight and delete old UI to prevent tearing
        ESP_LOGI(TAG, "Config update: Turning off backlight and clearing UI");
        display_hal_backlight_off();
        
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
        // Create new UI while screen is black
        ESP_LOGI(TAG, "Config update: Creating new UI");
        const lever_system_config_t *curr_config = config_manager_get_current();
        
        controller_init(curr_config);
        
        system_tabview = lever_system_create(lv_scr_act(), curr_config);
        
        state = 2;
    } else {
        // Turn backlight back on
        ESP_LOGI(TAG, "Config update: Restoring backlight");
        display_hal_backlight_on();
        
        state = 0;
    }
}

/**
 * @brief  Handle configuration reload events.
 *
 * This function handles events triggered by a configuration reload, setting a flag
 * to request a UI rebuild asynchronously.
 *
 * @param[in]  handler_args  Pointer to handler arguments (unused).
 * @param[in]  base          Event base.
 * @param[in]  id            Event ID.
 * @param[in]  event_data    Pointer to event data (unused).
 */
static void on_configuration_changed(void* handler_args, esp_event_base_t base, int32_t id, void* event_data)
{
    if (id != EVENT_CONFIG_RELOADED) return;

    ESP_LOGI(TAG, "Configuration changed! Scheduling UI rebuild...");
    ui_rebuild_requested = true;
}

/**
 * @brief  Main application entry point.
 *
 * Initializes the display hardware, UI system, event loops, Wi-Fi, 
 * web server, configuration manager, and OpenLCB stack. Builds the initial UI.
 */
void app_main(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;

    ESP_ERROR_CHECK(esp_event_loop_create_default());

    // Register event handlers
    ESP_ERROR_CHECK(esp_event_handler_instance_register(LEVER_SYSTEM_EVENTS, EVENT_CONFIG_RELOADED, on_configuration_changed, NULL, NULL));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(LEVER_SYSTEM_EVENTS, EVENT_LEVER_STATE_CHANGED, on_controller_state_changed, NULL, NULL));


    // Initialize hardware drivers and LVGL first
    ESP_ERROR_CHECK(display_hal_init(&panel, &touch));
    ESP_ERROR_CHECK(ui_port_init(panel, touch));

    // Show a Loading Splash Screen immediately
    lv_obj_t *loading_scr = NULL;
    if (ui_port_lock(-1)) {
        loading_scr = lv_obj_create(lv_scr_act());
        lv_obj_remove_style_all(loading_scr);
        lv_obj_set_size(loading_scr, LV_PCT(100), LV_PCT(100));
        lv_obj_set_style_bg_color(loading_scr, lv_color_hex(0x121212), 0);
        lv_obj_set_style_bg_opa(loading_scr, LV_OPA_COVER, 0);
        
        lv_obj_t *loading_label = lv_label_create(loading_scr);
        lv_label_set_text(loading_label, "Connecting to Wi-Fi...");
        lv_obj_set_style_text_color(loading_label, lv_color_hex(0xFFFFFF), 0);
        lv_obj_center(loading_label);
        
        // Force layout update so the label is perfectly centered
        lv_obj_update_layout(loading_scr);
        
        ui_port_unlock();
    }
    
    // Delay to allow RGB panel sync signals to stabilize before enabling backlight.
    vTaskDelay(pdMS_TO_TICKS(1500));
    ESP_ERROR_CHECK(display_hal_backlight_on());
    
    // Invalidate screen to prevent initial frame corruption.
    if (ui_port_lock(-1)) {
        lv_obj_invalidate(lv_scr_act());
        ui_port_unlock();
    }

    // Initialize configuration manager and load dynamic config
    ESP_ERROR_CHECK(config_manager_init());

    // Initialize Wi-Fi softAP and start the configuration web server
    // (This triggers connection to Home Wi-Fi)
    ESP_ERROR_CHECK(web_server_start());

    // Initialize OpenLCB stack (This BLOCKS waiting for Wi-Fi)
    openlcb_integration_init();

    // Build final UI from configuration
    if (ui_port_lock(-1)) {
        if (loading_scr) {
            lv_obj_del(loading_scr);
        }
        
        lv_timer_t *rebuild_timer = lv_timer_create(rebuild_ui_timer_cb, 50, NULL);
        lv_timer_set_repeat_count(rebuild_timer, -1);
        
        lv_timer_t *sleep_timer = lv_timer_create(display_sleep_timer_cb, 500, NULL);
        lv_timer_set_repeat_count(sleep_timer, -1);
        
        const lever_system_config_t *curr_config = config_manager_get_current();
        
        controller_init(curr_config);
        
        // Restore lever states if configuration matches
        if (curr_config->restore_last_state) {
            state_manager_load_and_apply(config_manager_get_hash());
        }
        
        system_tabview = lever_system_create(lv_scr_act(), curr_config);
        
        // Set the active tab to what was restored from NVS
        if (system_tabview) {
            lv_obj_t *tv = lv_obj_get_child(system_tabview, 0);
            if (tv) {
                lv_tabview_set_act(tv, controller_get_active_tab(), LV_ANIM_OFF);
            }
        }
        
        // Register gesture on the active screen to pull down the Wi-Fi info overlay
        lv_obj_clear_flag(lv_scr_act(), LV_OBJ_FLAG_SCROLLABLE);
        ui_overlays_register_gestures();
        
        // Force layout update before unlocking
        lv_obj_update_layout(lv_scr_act());
        
        ui_port_unlock();
    }
}
