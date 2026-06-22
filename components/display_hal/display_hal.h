#ifndef DISPLAY_HAL_H
#define DISPLAY_HAL_H

#include "esp_err.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lcd_touch.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Initialize the hardware display and touch controller.
 *
 * @param[out] panel_handle Pointer to store the panel handle
 * @param[out] touch_handle Pointer to store the touch handle
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_hal_init(esp_lcd_panel_handle_t *panel_handle, esp_lcd_touch_handle_t *touch_handle);

/**
 * @brief Turn the display backlight on.
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_hal_backlight_on(void);

/**
 * @brief Turn the display backlight off.
 *
 * @return esp_err_t ESP_OK on success
 */
esp_err_t display_hal_backlight_off(void);

#ifdef __cplusplus
}
#endif

#endif // DISPLAY_HAL_H
