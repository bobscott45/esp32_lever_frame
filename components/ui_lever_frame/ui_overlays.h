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
 * @file      ui_overlays.h
 * @brief     Definitions for UI Overlays
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef UI_OVERLAYS_H
#define UI_OVERLAYS_H

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief  Displays a dimmer overlay indicating remote configuration is in progress.
 *
 * Shows an LVGL overlay that prevents interactions and displays a loading text.
 * It automatically disappears after a timeout. Must be called with the UI port lock.
 */
void ui_show_remote_config_overlay(void);

/**
 * @brief  Displays the system information overlay drawer.
 *
 * Creates a slide-down menu overlay displaying current Wi-Fi configuration,
 * system IP, software versions, and global toggle switches for system-wide configuration
 * like startup mode, OpenLCB master, and override policy.
 */
void ui_show_info_overlay(void);

/**
 * @brief  Closes the system information overlay drawer immediately.
 *
 * Deletes the info overlay and dimmer objects if they exist.
 */
void info_overlay_close_immediate(void);

/**
 * @brief  Registers screen-wide gestures for the overlays.
 *
 * Adds LVGL event callbacks to the active screen to catch gestures
 * like swipe down to show the system information overlay.
 */
void ui_overlays_register_gestures(void);

#ifdef __cplusplus
}
#endif

#endif // UI_OVERLAYS_H
