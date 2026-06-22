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
