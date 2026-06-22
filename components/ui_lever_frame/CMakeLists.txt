idf_component_register(SRCS "lever.c" "lever_frame.c" "ui_overlays.c"
                       INCLUDE_DIRS "."
                       REQUIRES lever_core lvgl ui_porting esp_netif esp_app_format)
