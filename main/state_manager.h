#ifndef STATE_MANAGER_H
#define STATE_MANAGER_H

#include "lvgl.h"
#include <stdint.h>

/**
 * @brief Load state from NVS. If the current_config_hash matches the saved hash, apply the states to the UI.
 * @param system_wrapper The root wrapper object of the lever system
 * @param current_config_hash The hash of the active configuration
 */
void state_manager_load_and_apply(lv_obj_t *system_wrapper, uint32_t current_config_hash);

/**
 * @brief Save the current states from the UI to NVS along with the config hash.
 * @param system_wrapper The root wrapper object of the lever system
 * @param current_config_hash The hash of the active configuration
 */
void state_manager_save(lv_obj_t *system_wrapper, uint32_t current_config_hash);

#endif // STATE_MANAGER_H
