#include "bsp/board.h"
#include "bsp/lvgl_port.h"
#include "lever.h"
#include "lever_frame.h"
#include "config_manager.h"
#include "web_server.h"
#include "esp_log.h"

static const char *TAG = "main";
static lv_obj_t *system_tabview = NULL;

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

static void rebuild_ui_timer_cb(lv_timer_t *timer)
{
    static int state = 0;
    
    if (state == 0) {
        // Step 1: Turn off backlight and delete old UI to prevent tearing
        ESP_LOGI(TAG, "Config update: Turning off backlight and clearing UI");
        waveshare_rgb_lcd_bl_off();
        
        if (system_tabview) {
            lv_obj_del(system_tabview);
            system_tabview = NULL;
        }
        
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
        lv_timer_del(timer);
    }
}

static void on_configuration_changed(void)
{
    ESP_LOGI(TAG, "Configuration changed! Scheduling UI rebuild...");
    if (lvgl_port_lock(-1)) {
        lv_timer_t *timer = lv_timer_create(rebuild_ui_timer_cb, 50, NULL);
        if (timer) {
            lv_timer_set_repeat_count(timer, -1);
        }
        lvgl_port_unlock();
    }
}

void app_main(void)
{
    esp_lcd_panel_handle_t panel = NULL;
    esp_lcd_touch_handle_t touch = NULL;

    // 1. Initialize configuration manager and load dynamic config
    ESP_ERROR_CHECK(config_manager_init());
    config_manager_set_on_change(on_configuration_changed);

    // 2. Initialize Wi-Fi softAP and start the configuration web server
    ESP_ERROR_CHECK(web_server_start());

    // 3. Initialize hardware drivers independently
    ESP_ERROR_CHECK(waveshare_esp32_s3_rgb_lcd_init(&panel, &touch));

    // 4. Initialize the LVGL port task loop with retrieved handles
    ESP_ERROR_CHECK(lvgl_port_init(panel, touch));

    // 5. Turn on the screen backlight
    ESP_ERROR_CHECK(waveshare_rgb_lcd_bl_on());

    // 6. Lock the port and build your UI from configuration
    if (lvgl_port_lock(-1)) {
        const lever_system_config_t *curr_config = config_manager_get_current();
        system_tabview = lever_system_create(lv_scr_act(), curr_config);
        
        // Restore lever states if configuration matches
        #include "state_manager.h"
        if (curr_config->restore_last_state) {
            state_manager_load_and_apply(system_tabview, config_manager_get_hash());
        }
        
        lvgl_port_unlock();
    }
}
