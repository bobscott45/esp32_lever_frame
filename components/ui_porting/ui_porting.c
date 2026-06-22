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
#include "bsp/lvgl_port.h"

esp_err_t ui_port_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle) {
    if (!panel_handle || !touch_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    
    // For now, this simply delegates to the Waveshare BSP's LVGL porting layer.
    // In the future, this can be swapped to esp_lvgl_port for generic screens.
    
    return lvgl_port_init(panel_handle, touch_handle);
}

bool ui_port_lock(int timeout_ms) {
    return lvgl_port_lock(timeout_ms);
}

void ui_port_unlock(void) {
    lvgl_port_unlock();
}
