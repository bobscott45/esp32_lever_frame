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
 * @file i2c_drivers.c
 * @brief Hardware driver implementations for I2C Drivers.
 *
 * @author Robert Scott
 * @date 2026
 */

#include "esp_log.h"
#include "i2c_drivers.h"
#include <i2cdev.h>

#define TAG "I2C_DRIVER"

void init_i2c_bus(void) {
    ESP_LOGI(TAG, "Initializing i2cdev master bus manager");
    esp_err_t err = i2cdev_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to initialize i2cdev manager: %d", err);
    }
}
