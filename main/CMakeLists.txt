idf_component_register(SRCS "main.c"
                    "lever.c"
                    "lever_frame.c"
                    "config_manager.c"
                    "web_server.c"
                    INCLUDE_DIRS "."
                    PRIV_REQUIRES cjson nvs_flash esp_wifi esp_event esp_netif esp_http_server
                    EMBED_TXTFILES "web/index.html")
