idf_component_register(SRCS "openlcb_integration.c" "openlcb_user_config.c"
                       INCLUDE_DIRS "."
                       REQUIRES lever_core lcc lcc_drivers)
