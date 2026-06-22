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
