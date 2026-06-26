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
 * @file      display_hal.h
 * @brief     Definitions for display_hal.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize the hardware display and touch controller.
 *
 * This function initializes the LCD panel and touch controller hardware using the
 * underlying Waveshare BSP. It must be called before any UI operations are performed.
 *
 * @param[out] panel_handle   Pointer to store the resulting panel handle.
 * @param[out] touch_handle   Pointer to store the resulting touch handle.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t display_hal_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_touch_handle_t *touch_handle);

/**
 * @brief  Turn the display backlight on.
 *
 * This function powers on the backlight for the hardware display. It is typically 
 * used to wake up the screen after a sleep or idle period.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t display_hal_backlight_on(void);

/**
 * @brief  Turn the display backlight off.
 *
 * This function powers off the backlight for the hardware display. It is useful 
 * for saving power when the display is not actively being viewed.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t display_hal_backlight_off(void);

/**
 * @brief  Fade the display backlight on or off.
 *
 * Smoothly transitions the backlight brightness. On hardware that does not 
 * support PWM dimming (e.g. S3), this function instantly snaps the brightness
 * on or off.
 * 
 * @param[in] on  True to fade on, false to fade off.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_FAIL on general failure
 */
esp_err_t display_hal_fade_backlight(bool on);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_HAL_H
