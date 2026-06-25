if(CONFIG_HARDWARE_BOARD_WAVESHARE_P4)
    set(LCD_COMP esp32_p4_wifi6_touch_lcd_4_3)
elseif(CONFIG_HARDWARE_BOARD_WAVESHARE_S3)
    set(LCD_COMP waveshare_esp32_s3_touch_lcd_4_3)
else()
    if(IDF_TARGET STREQUAL "esp32p4")
        set(LCD_COMP esp32_p4_wifi6_touch_lcd_4_3)
    else()
        set(LCD_COMP waveshare_esp32_s3_touch_lcd_4_3)
    endif()
endif()

idf_component_register(SRCS "display_hal.c"
                    INCLUDE_DIRS "."
                    REQUIRES esp_lcd esp_lcd_touch ${LCD_COMP})
