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
 * @file      display_hal.c
 * @brief     Implementation of display_hal.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "display_hal.h"
#include "bsp/board.h"

esp_err_t display_hal_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_touch_handle_t *touch_handle) {
    if (!panel_handle || !touch_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    return waveshare_esp32_s3_rgb_lcd_init(panel_handle, touch_handle);
}

esp_err_t display_hal_backlight_on(void) {
    return waveshare_rgb_lcd_bl_on();
}

esp_err_t display_hal_backlight_off(void) {
    return waveshare_rgb_lcd_bl_off();
}
