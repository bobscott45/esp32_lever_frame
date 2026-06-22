idf_component_register(SRCS "config_manager.c" "controller.c" "interlocking.c" "state_manager.c"
                       INCLUDE_DIRS "."
                       REQUIRES nvs_flash cjson)
