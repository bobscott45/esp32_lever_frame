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
 * @file      ui_porting.h
 * @brief     Definitions for ui_porting.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef UI_PORTING_H
#define UI_PORTING_H

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Initialize the UI porting layer (LVGL).
 *
 * This function delegates to the BSP's LVGL porting layer to set up the user
 * interface based on the provided hardware panel and touch handles.
 *
 * @param[in]  panel_handle   Handle to the initialized LCD panel.
 * @param[in]  touch_handle   Handle to the initialized touch controller.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t ui_port_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle);

/**
 * @brief  Lock the UI for thread-safe access.
 *
 * This function acquires a mutex to allow safe access to UI elements from 
 * different tasks. It prevents concurrent modifications to LVGL state.
 *
 * @param[in]  timeout_ms   Timeout in milliseconds (-1 for wait forever).
 * 
 * @return 
 *   - true if locked successfully
 *   - false otherwise
 */
bool ui_port_lock(int timeout_ms);

/**
 * @brief  Unlock the UI.
 *
 * This function releases the mutex acquired by ui_port_lock, allowing other
 * tasks to safely access and modify UI elements.
 */
void ui_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // UI_PORTING_H
