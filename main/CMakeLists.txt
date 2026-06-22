idf_component_register(SRCS "main.c" "web_server.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES lever_core ui_lever_frame openlcb_node esp_wifi esp_event esp_netif esp_http_server display_hal
                    EMBED_FILES "web/index.html.gz")

target_compile_definitions(${COMPONENT_LIB} PRIVATE PROJECT_VER="${PROJECT_VER}")
