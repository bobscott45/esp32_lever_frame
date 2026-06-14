//
// Created by robert on 27/05/2026.
//
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
