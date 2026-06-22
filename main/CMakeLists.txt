idf_component_register(SRCS "main.c" "web_server.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES lever_core ui_lever_frame openlcb_node esp_wifi esp_event esp_netif esp_http_server waveshare_esp32_s3_touch_lcd_4_3
                    EMBED_FILES "web/index.html.gz")

target_compile_definitions(${COMPONENT_LIB} PRIVATE PROJECT_VER="${PROJECT_VER}")
