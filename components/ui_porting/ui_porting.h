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
 * @brief Initialize the UI porting layer (LVGL)
 */
esp_err_t ui_port_init(esp_lcd_panel_handle_t panel_handle, esp_lcd_touch_handle_t touch_handle);

/**
 * @brief Lock the UI for thread-safe access
 * @param timeout_ms Timeout in milliseconds (-1 for wait forever)
 * @return true if locked successfully, false otherwise
 */
bool ui_port_lock(int timeout_ms);

/**
 * @brief Unlock the UI
 */
void ui_port_unlock(void);

#ifdef __cplusplus
}
#endif

#endif // UI_PORTING_H
