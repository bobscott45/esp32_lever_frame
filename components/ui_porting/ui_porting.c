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
 * @file      ui_porting.c
 * @brief     Implementation of ui_porting.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "ui_porting.h"
#include "sdkconfig.h"
#include "esp_log.h"

#ifdef CONFIG_IDF_TARGET_ESP32P4
#include "bsp/esp32_p4_wifi6_touch_lcd_4_3.h"
#include "esp_lv_adapter.h"

esp_err_t ui_port_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle) {
    if (!panel_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // In display_hal.c we initialized the panel and touch.
    // Now we register them with LVGL using esp_lv_adapter.
    
    esp_lv_adapter_config_t adapter_cfg = ESP_LV_ADAPTER_DEFAULT_CONFIG();
    esp_lv_adapter_init(&adapter_cfg);

    esp_lv_adapter_display_config_t disp_cfg = {
        .panel = panel_handle,
        .profile = {
            .interface = ESP_LV_ADAPTER_PANEL_IF_MIPI_DSI,
            .rotation = ESP_LV_ADAPTER_ROTATE_90,
            .hor_res = 480,
            .ver_res = 800,
            .buffer_height = 50,
            .use_psram = false,
            .enable_ppa_accel = false,
            .require_double_buffer = false,
        },
        .tear_avoid_mode = ESP_LV_ADAPTER_TEAR_AVOID_MODE_TRIPLE_PARTIAL,
    };
    
    lv_display_t *disp = esp_lv_adapter_register_display(&disp_cfg);
    if (!disp) {
        return ESP_FAIL;
    }
    
    if (touch_handle) {
        esp_lv_adapter_touch_config_t touch_cfg = ESP_LV_ADAPTER_TOUCH_DEFAULT_CONFIG(disp, touch_handle);
        lv_indev_t *indev = esp_lv_adapter_register_touch(&touch_cfg);
        if (!indev) {
            ESP_LOGE("ui_port", "Failed to register touch");
        }
    }
    
    // Start the LVGL task so LVGL actually runs!
    ESP_ERROR_CHECK(esp_lv_adapter_start());
    
    return ESP_OK;
}

bool ui_port_lock(int timeout_ms) {
    return esp_lv_adapter_lock(timeout_ms) == ESP_OK;
}

void ui_port_unlock(void) {
    esp_lv_adapter_unlock();
}
#else
#include "bsp/lvgl_port.h"

esp_err_t ui_port_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle) {
    if (!panel_handle || !touch_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    return lvgl_port_init(panel_handle, touch_handle);
}

bool ui_port_lock(int timeout_ms) {
    return lvgl_port_lock(timeout_ms);
}

void ui_port_unlock(void) {
    lvgl_port_unlock();
}
#endif
