cmake_minimum_required(VERSION 3.16)

idf_component_register(
        SRCS "lcc_drivers.c"
             "can_drivers.c"
             "i2c_drivers.c"
             "tcp_drivers.c"
        INCLUDE_DIRS "." "../../main"
        REQUIRES lcc esp_wifi nvs_flash esp_event driver esp_driver_gpio esp_driver_twai esp-idf-lib__i2cdev
)