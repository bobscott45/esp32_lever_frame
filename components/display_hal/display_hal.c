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
#include "sdkconfig.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#ifdef CONFIG_HARDWARE_BOARD_WAVESHARE_P4
#include "bsp/esp32_p4_wifi6_touch_lcd_4_3.h"
#include "bsp/display.h"
#include "bsp/touch.h"

esp_err_t display_hal_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_touch_handle_t *touch_handle) {
    if (!panel_handle || !touch_handle) {
        return ESP_ERR_INVALID_ARG;
    }
    bsp_display_config_t disp_cfg = {0};
    bsp_lcd_handles_t lcd_handles = {0};
    
    esp_err_t err = bsp_display_new_with_handles(&disp_cfg, &lcd_handles);
    if (err != ESP_OK) {
        return err;
    }
    *panel_handle = lcd_handles.panel;
    
    bsp_display_cfg_t touch_cfg = {
        .touch_flags = {
            .swap_xy = 1,
            .mirror_x = 1,
            .mirror_y = 0,
        }
    };
    err = bsp_touch_new(&touch_cfg, touch_handle);
    if (err != ESP_OK) {
        ESP_LOGE("display_hal", "Touch init failed (%s), continuing without touch", esp_err_to_name(err));
        *touch_handle = NULL;
    }
    
    return bsp_display_brightness_init();
}

esp_err_t display_hal_backlight_on(void) {
    return bsp_display_backlight_on();
}

esp_err_t display_hal_backlight_off(void) {
    return bsp_display_backlight_off();
}
esp_err_t display_hal_fade_backlight(bool on) {
    if (on) {
        for (int i = 0; i <= 100; i += 5) {
            bsp_display_brightness_set(i);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        return bsp_display_brightness_set(100);
    } else {
        for (int i = 100; i >= 0; i -= 5) {
            bsp_display_brightness_set(i);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
        return bsp_display_brightness_set(0);
    }
}
#elif defined(CONFIG_HARDWARE_BOARD_WAVESHARE_S3)
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

esp_err_t display_hal_fade_backlight(bool on) {
    // S3 hardware cannot dim, so snap instantly
    if (on) {
        return display_hal_backlight_on();
    } else {
        return display_hal_backlight_off();
    }
}
#else
#error "No supported hardware board selected in Kconfig!"
#endif
