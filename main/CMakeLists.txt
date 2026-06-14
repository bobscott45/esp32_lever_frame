idf_component_register(SRCS "main.c"
                    "lever.c"
                    "lever_frame.c"
                    "config_manager.c"
                    "web_server.c"
                    "state_manager.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES cjson nvs_flash esp_wifi esp_event esp_netif esp_http_server lcc lcc_drivers
                    EMBED_FILES "web/index.html.gz")
