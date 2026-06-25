/*
 * This file is part of esp32_lever_frame.
 *
 * esp32_lever_frame is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * esp32_lever_frame is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with esp32_lever_frame.  If not, see <https://www.gnu.org/licenses/>.
 */

/**
 * @file      config_manager.c
 * @brief     Implementation of config_manager.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "default_prototypical_config.h"
#include "cJSON.h"
#include "system_events.h"

static const char *TAG = "ConfigManager";

#define LEVER_DEF(lbl, typ) { \
    .label = lbl, .type = typ, \
    .conditions = { \
        { .target_lever_index = -1, .required_state = false }, \
        { .target_lever_index = -1, .required_state = false }, \
        { .target_lever_index = -1, .required_state = false }, \
        { .target_lever_index = -1, .required_state = false } \
    } \
}

// Fallback default configuration
static const lever_def_t default_frame1_levers[] = {
    LEVER_DEF("UP\nDISTANT", LEVER_TYPE_DISTANT_SIGNAL),
    LEVER_DEF("UP\nHOME", LEVER_TYPE_HOME_SIGNAL),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("FACING\nPOINTS", LEVER_TYPE_FACING_POINTS),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE)
};

static const lever_def_t default_frame2_levers[] = {
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE)
};

static const lever_def_t default_frame3_levers[] = {
    LEVER_DEF("YARD#1", LEVER_TYPE_POINTS),
    LEVER_DEF("YARD#2", LEVER_TYPE_POINTS),
    LEVER_DEF("MAIN LINE", LEVER_TYPE_FACING_POINTS),
    LEVER_DEF("SPARE", LEVER_TYPE_SPARE)
};

static const tab_def_t default_tabs[] = {
    {.name = "North Junction Frame 1", .levers = default_frame1_levers, .lever_count = sizeof(default_frame1_levers) / sizeof(lever_def_t), .label_lines = 2, .label_line_height = 18},
    {.name = "North Junction Frame 2", .levers = default_frame2_levers, .lever_count = sizeof(default_frame2_levers) / sizeof(lever_def_t), .label_lines = 2, .label_line_height = 18},
    {.name = "Sidings", .levers = default_frame3_levers, .lever_count = sizeof(default_frame3_levers) / sizeof(lever_def_t), .label_lines = 2, .label_line_height = 18}
};

static const lever_system_config_t default_config = {
    .tabs = default_tabs,
    .tab_count = sizeof(default_tabs) / sizeof(default_tabs[0]),
    .restore_last_state = true,
    .conflict_policy = INTERLOCK_POLICY_STRICT_LOCAL,
    .lcc_enabled = true,
    .wifi_ssid = "",
    .wifi_password = "signalman",
    .wifi_station_password = "",
    .jmri_ip_address = ""
};

// Currently active dynamic configuration
static lever_system_config_t active_dynamic_config = {0};
static bool is_using_dynamic = false;
static uint32_t active_config_hash = 0;

/**
 * @brief  Calculate a hash for a JSON string.
 *
 * Computes a 32-bit djb2 hash of the provided JSON configuration string, 
 * which is used to quickly detect layout or configuration changes.
 *
 * @param[in]  str   The JSON string to hash.
 * 
 * @return 
 *   - 32-bit hash value
 */
static uint32_t hash_json_string(const char *str) {
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    return hash;
}

uint32_t config_manager_get_hash(void) {
    return active_config_hash;
}

// Configuration change callback

const char *lever_type_to_str(lever_type_t type) {
    switch (type) {
        case LEVER_TYPE_HOME_SIGNAL:    return "HOME_SIGNAL";
        case LEVER_TYPE_DISTANT_SIGNAL: return "DISTANT_SIGNAL";
        case LEVER_TYPE_POINTS:         return "POINTS";
        case LEVER_TYPE_FACING_POINTS:  return "FACING_POINTS";
        case LEVER_TYPE_BROWN:          return "BROWN";
        case LEVER_TYPE_GREEN:          return "GREEN";
        case LEVER_TYPE_SPARE:
        default:                        return "SPARE";
    }
}

lever_type_t str_to_lever_type(const char *str) {
    if (!str) return LEVER_TYPE_SPARE;
    if (strcmp(str, "HOME_SIGNAL") == 0)    return LEVER_TYPE_HOME_SIGNAL;
    if (strcmp(str, "DISTANT_SIGNAL") == 0) return LEVER_TYPE_DISTANT_SIGNAL;
    if (strcmp(str, "POINTS") == 0)         return LEVER_TYPE_POINTS;
    if (strcmp(str, "FACING_POINTS") == 0)  return LEVER_TYPE_FACING_POINTS;
    if (strcmp(str, "BROWN") == 0)          return LEVER_TYPE_BROWN;
    if (strcmp(str, "GREEN") == 0)          return LEVER_TYPE_GREEN;
    return LEVER_TYPE_SPARE;
}

/**
 * @brief  Free dynamically allocated configuration structures.
 *
 * Recursively frees all heap-allocated memory associated with a dynamically 
 * loaded configuration, including tabs, levers, strings, and Wi-Fi credentials.
 *
 * @param[in]  config   Pointer to the configuration structure to free.
 */
static void free_dynamic_config(lever_system_config_t *config) {
    if (!config || config->tab_count == 0) return;
    
    for (size_t t = 0; t < config->tab_count; t++) {
        tab_def_t *tab = (tab_def_t *)&config->tabs[t];
        if (tab->name) {
            free((void *)tab->name);
        }
        if (tab->levers) {
            for (size_t l = 0; l < tab->lever_count; l++) {
                lever_def_t *lever = (lever_def_t *)&tab->levers[l];
                if (lever->label) {
                    free((void *)lever->label);
                }
            }
            free((void *)tab->levers);
        }
    }
    free((void *)config->tabs);
    if (config->wifi_password) {
        free((void *)config->wifi_password);
    }
    if (config->wifi_ssid) {
        free((void *)config->wifi_ssid);
    }
    if (config->wifi_station_password) {
        free((void *)config->wifi_station_password);
    }
    if (config->jmri_ip_address) {
        free((void *)config->jmri_ip_address);
    }
    memset(config, 0, sizeof(lever_system_config_t));
}

/**
 * @brief  Parse a JSON string into a configuration structure.
 *
 * Uses cJSON to parse the provided JSON string and populate the target 
 * configuration structure with dynamically allocated strings and arrays.
 *
 * @param[in]  json_str     The raw JSON string to parse.
 * @param[out] out_config   Pointer to the configuration structure to populate.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_ERR_NO_MEM if memory allocation fails
 */
static esp_err_t parse_json_to_config(const char *json_str, lever_system_config_t *out_config) {
    cJSON *root = cJSON_Parse(json_str);
    if (!root) {
        ESP_LOGE(TAG, "Failed to parse JSON string");
        return ESP_ERR_INVALID_ARG;
    }

    cJSON *tabs_arr = cJSON_GetObjectItem(root, "tabs");
    if (!tabs_arr || !cJSON_IsArray(tabs_arr)) {
        ESP_LOGE(TAG, "JSON missing 'tabs' array");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    int tab_count = cJSON_GetArraySize(tabs_arr);
    if (tab_count <= 0) {
        ESP_LOGE(TAG, "Empty 'tabs' array");
        cJSON_Delete(root);
        return ESP_ERR_INVALID_ARG;
    }

    tab_def_t *dynamic_tabs = calloc(tab_count, sizeof(tab_def_t));
    if (!dynamic_tabs) {
        cJSON_Delete(root);
        return ESP_ERR_NO_MEM;
    }

    for (int t = 0; t < tab_count; t++) {
        cJSON *tab_obj = cJSON_GetArrayItem(tabs_arr, t);
        if (!tab_obj) goto cleanup_error;

        cJSON *name_obj = cJSON_GetObjectItem(tab_obj, "name");
        const char *name_str = name_obj && cJSON_IsString(name_obj) ? name_obj->valuestring : "Frame";
        dynamic_tabs[t].name = strdup(name_str);

        cJSON *lines_obj = cJSON_GetObjectItem(tab_obj, "label_lines");
        dynamic_tabs[t].label_lines = lines_obj && cJSON_IsNumber(lines_obj) ? lines_obj->valueint : 2;

        cJSON *height_obj = cJSON_GetObjectItem(tab_obj, "label_line_height");
        dynamic_tabs[t].label_line_height = height_obj && cJSON_IsNumber(height_obj) ? height_obj->valueint : 18;

        cJSON *levers_arr = cJSON_GetObjectItem(tab_obj, "levers");
        if (!levers_arr || !cJSON_IsArray(levers_arr)) {
            dynamic_tabs[t].levers = NULL;
            dynamic_tabs[t].lever_count = 0;
            continue;
        }

        int lever_count = cJSON_GetArraySize(levers_arr);
        dynamic_tabs[t].lever_count = lever_count;
        if (lever_count > 0) {
            lever_def_t *dynamic_levers = calloc(lever_count, sizeof(lever_def_t));
            if (!dynamic_levers) goto cleanup_error;
            dynamic_tabs[t].levers = dynamic_levers;

            for (int l = 0; l < lever_count; l++) {
                cJSON *lever_obj = cJSON_GetArrayItem(levers_arr, l);
                if (!lever_obj) goto cleanup_error;

                cJSON *label_obj = cJSON_GetObjectItem(lever_obj, "label");
                const char *label_str = label_obj && cJSON_IsString(label_obj) ? label_obj->valuestring : "SPARE";
                dynamic_levers[l].label = strdup(label_str);

                cJSON *type_obj = cJSON_GetObjectItem(lever_obj, "type");
                const char *type_str = type_obj && cJSON_IsString(type_obj) ? type_obj->valuestring : "SPARE";
                dynamic_levers[l].type = str_to_lever_type(type_str);
                
                cJSON *lcc_norm = cJSON_GetObjectItem(lever_obj, "lcc_event_normal");
                if (lcc_norm && cJSON_IsString(lcc_norm)) {
                    strncpy(dynamic_levers[l].lcc_event_normal, lcc_norm->valuestring, sizeof(dynamic_levers[l].lcc_event_normal) - 1);
                } else {
                    dynamic_levers[l].lcc_event_normal[0] = '\0';
                }

                cJSON *lcc_rev = cJSON_GetObjectItem(lever_obj, "lcc_event_reversed");
                if (lcc_rev && cJSON_IsString(lcc_rev)) {
                    strncpy(dynamic_levers[l].lcc_event_reversed, lcc_rev->valuestring, sizeof(dynamic_levers[l].lcc_event_reversed) - 1);
                } else {
                    dynamic_levers[l].lcc_event_reversed[0] = '\0';
                }
                
                cJSON *lcc_en_obj = cJSON_GetObjectItem(lever_obj, "lcc_enabled");
                if (lcc_en_obj && cJSON_IsBool(lcc_en_obj)) {
                    dynamic_levers[l].lcc_enabled = cJSON_IsTrue(lcc_en_obj);
                } else {
                    dynamic_levers[l].lcc_enabled = true; // default enabled
                }
                
                for(int c = 0; c < MAX_INTERLOCKING_CONDITIONS; c++) {
                    dynamic_levers[l].conditions[c].target_lever_index = -1;
                    dynamic_levers[l].conditions[c].required_state = false;
                }
                
                cJSON *interlocking_arr = cJSON_GetObjectItem(lever_obj, "interlocking");
                if (interlocking_arr && cJSON_IsArray(interlocking_arr)) {
                    int c_count = cJSON_GetArraySize(interlocking_arr);
                    if (c_count > MAX_INTERLOCKING_CONDITIONS) c_count = MAX_INTERLOCKING_CONDITIONS;
                    for(int c = 0; c < c_count; c++) {
                        cJSON *cond_obj = cJSON_GetArrayItem(interlocking_arr, c);
                        if (!cond_obj) continue;
                        
                        cJSON *target_obj = cJSON_GetObjectItem(cond_obj, "target");
                        cJSON *state_obj = cJSON_GetObjectItem(cond_obj, "state");
                        
                        if (target_obj && cJSON_IsNumber(target_obj)) {
                            dynamic_levers[l].conditions[c].target_lever_index = target_obj->valueint;
                            if (state_obj && cJSON_IsString(state_obj)) {
                                dynamic_levers[l].conditions[c].required_state = (strcmp(state_obj->valuestring, "REVERSED") == 0);
                            }
                        }
                        
                        cJSON *alt_target_obj = cJSON_GetObjectItem(cond_obj, "alt_target");
                        cJSON *alt_state_obj = cJSON_GetObjectItem(cond_obj, "alt_state");
                        if (alt_target_obj && cJSON_IsNumber(alt_target_obj)) {
                            dynamic_levers[l].conditions[c].alt_target_lever_index = alt_target_obj->valueint;
                            if (alt_state_obj && cJSON_IsString(alt_state_obj)) {
                                dynamic_levers[l].conditions[c].alt_required_state = (strcmp(alt_state_obj->valuestring, "REVERSED") == 0);
                            }
                        } else {
                            dynamic_levers[l].conditions[c].alt_target_lever_index = -1;
                            dynamic_levers[l].conditions[c].alt_required_state = false;
                        }
                    }
                }
            }
        }
    }

    out_config->tabs = dynamic_tabs;
    out_config->tab_count = tab_count;
    
    cJSON *wifi_pw_obj = cJSON_GetObjectItem(root, "wifi_password");
    if (wifi_pw_obj && cJSON_IsString(wifi_pw_obj)) {
        out_config->wifi_password = strdup(wifi_pw_obj->valuestring);
    } else {
        out_config->wifi_password = NULL;
    }
    
    cJSON *wifi_ssid_obj = cJSON_GetObjectItem(root, "wifi_ssid");
    if (wifi_ssid_obj && cJSON_IsString(wifi_ssid_obj)) {
        out_config->wifi_ssid = strdup(wifi_ssid_obj->valuestring);
    } else {
        out_config->wifi_ssid = NULL;
    }

    cJSON *wifi_sta_pw_obj = cJSON_GetObjectItem(root, "wifi_station_password");
    if (wifi_sta_pw_obj && cJSON_IsString(wifi_sta_pw_obj)) {
        out_config->wifi_station_password = strdup(wifi_sta_pw_obj->valuestring);
    } else {
        out_config->wifi_station_password = NULL;
    }

    cJSON *jmri_ip_obj = cJSON_GetObjectItem(root, "jmri_ip_address");
    if (jmri_ip_obj && cJSON_IsString(jmri_ip_obj)) {
        out_config->jmri_ip_address = strdup(jmri_ip_obj->valuestring);
    } else {
        out_config->jmri_ip_address = NULL;
    }
    
    cJSON *restore_obj = cJSON_GetObjectItem(root, "restore_last_state");
    if (restore_obj && cJSON_IsBool(restore_obj)) {
        out_config->restore_last_state = cJSON_IsTrue(restore_obj);
    } else {
        out_config->restore_last_state = true;
    }
    
    cJSON *policy_obj = cJSON_GetObjectItem(root, "conflict_policy");
    if (policy_obj && cJSON_IsNumber(policy_obj)) {
        out_config->conflict_policy = (interlocking_conflict_policy_t)policy_obj->valueint;
    } else {
        out_config->conflict_policy = INTERLOCK_POLICY_STRICT_LOCAL;
    }

    cJSON *sleep_obj = cJSON_GetObjectItem(root, "display_sleep_timeout_ms");
    if (sleep_obj && cJSON_IsNumber(sleep_obj)) {
        out_config->display_sleep_timeout_ms = sleep_obj->valueint;
    } else {
        out_config->display_sleep_timeout_ms = 60000; // 60 seconds default
    }
    
    cJSON *global_lcc_obj = cJSON_GetObjectItem(root, "lcc_enabled");
    if (global_lcc_obj && cJSON_IsBool(global_lcc_obj)) {
        out_config->lcc_enabled = cJSON_IsTrue(global_lcc_obj);
    } else {
        out_config->lcc_enabled = true; // default enabled
    }
    
    cJSON_Delete(root);
    return ESP_OK;

cleanup_error:
    cJSON_Delete(root);
    // Free whatever was allocated
    lever_system_config_t temp_to_free = { .tabs = dynamic_tabs, .tab_count = tab_count };
    free_dynamic_config(&temp_to_free);
    return ESP_ERR_NO_MEM;
}

static lever_system_config_t pending_free_config = {0};
static bool has_pending_free = false;

void config_manager_free_pending(void) {
    if (has_pending_free) {
        free_dynamic_config(&pending_free_config);
        has_pending_free = false;
    }
}

esp_err_t config_manager_init(void) {
    // Initialize NVS
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK(err);

    // Try to load saved config string from NVS
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No 'storage' namespace found in NVS. Using prototypical config.");
        
        lever_system_config_t new_config = {0};
        esp_err_t parse_err = parse_json_to_config(default_prototypical_config_json, &new_config);
        if (parse_err == ESP_OK) {
            config_manager_free_pending();
            if (is_using_dynamic) {
                pending_free_config = active_dynamic_config;
                has_pending_free = true;
            }
            active_dynamic_config = new_config;
            active_config_hash = hash_json_string(default_prototypical_config_json);
            is_using_dynamic = true;
        } else {
            ESP_LOGE(TAG, "Failed to parse default prototypical config. Falling back to compile-time default.");
        }
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_blob(my_handle, "config_json", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No config_json found in NVS. Using prototypical config.");
        nvs_close(my_handle);
        
        lever_system_config_t new_config = {0};
        esp_err_t parse_err = parse_json_to_config(default_prototypical_config_json, &new_config);
        if (parse_err == ESP_OK) {
            config_manager_free_pending();
            if (is_using_dynamic) {
                pending_free_config = active_dynamic_config;
                has_pending_free = true;
            }
            active_dynamic_config = new_config;
            active_config_hash = hash_json_string(default_prototypical_config_json);
            is_using_dynamic = true;
        } else {
            ESP_LOGE(TAG, "Failed to parse default prototypical config. Falling back to compile-time default.");
        }
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error getting config_json size from NVS: %s", esp_err_to_name(err));
        nvs_close(my_handle);
        return err;
    }

    char *json_buf = malloc(required_size);
    if (!json_buf) {
        nvs_close(my_handle);
        return ESP_ERR_NO_MEM;
    }

    err = nvs_get_blob(my_handle, "config_json", json_buf, &required_size);
    nvs_close(my_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error reading config_json from NVS: %s", esp_err_to_name(err));
        free(json_buf);
        return err;
    }

    ESP_LOGI(TAG, "Loading saved config from NVS...");
    lever_system_config_t new_config = {0};
    
    // Calculate hash before parsing and freeing
    uint32_t loaded_hash = hash_json_string(json_buf);
    
    err = parse_json_to_config(json_buf, &new_config);
    free(json_buf);

    if (err == ESP_OK) {
        active_dynamic_config = new_config;
        is_using_dynamic = true;
        active_config_hash = loaded_hash;
        ESP_LOGI(TAG, "Successfully loaded dynamic config from NVS.");
    } else {
        ESP_LOGE(TAG, "Saved JSON config was invalid. Using default config instead.");
    }

    if (!is_using_dynamic) {
        char *json = config_manager_get_json_str();
        active_config_hash = hash_json_string(json);
        free(json);
    }

    return ESP_OK;
}

const lever_system_config_t *config_manager_get_current(void) {
    if (is_using_dynamic) {
        return &active_dynamic_config;
    }
    return &default_config;
}



esp_err_t config_manager_save_json_internal(const char *json_str, bool notify) {
    lever_system_config_t new_config = {0};
    esp_err_t err = parse_json_to_config(json_str, &new_config);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Rejected invalid JSON config.");
        return err;
    }

    // Save to NVS
    nvs_handle_t my_handle;
    err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS storage: %s", esp_err_to_name(err));
        free_dynamic_config(&new_config);
        return err;
    }

    err = nvs_set_blob(my_handle, "config_json", json_str, strlen(json_str) + 1);
    if (err == ESP_OK) {
        err = nvs_commit(my_handle);
    }
    nvs_close(my_handle);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Failed to write config JSON to NVS: %s", esp_err_to_name(err));
        free_dynamic_config(&new_config);
        return err;
    }

    // Swap configurations
    if (is_using_dynamic) {
        if (has_pending_free) {
            free_dynamic_config(&pending_free_config);
        }
        pending_free_config = active_dynamic_config;
        has_pending_free = true;
    }
    active_dynamic_config = new_config;
    is_using_dynamic = true;
    active_config_hash = hash_json_string(json_str);

    ESP_LOGI(TAG, "Dynamic configuration updated and saved to NVS.");

    // Trigger callback if registered
    if (notify) {
        esp_event_post(LEVER_SYSTEM_EVENTS, EVENT_CONFIG_RELOADED, NULL, 0, portMAX_DELAY);
    }

    return ESP_OK;
}

esp_err_t config_manager_save_json(const char *json_str) {
    return config_manager_save_json_internal(json_str, true);
}

char *config_manager_get_json_str(void) {
    if (is_using_dynamic) {
        // Read directly from NVS to avoid cJSON tree allocation overhead
        nvs_handle_t my_handle;
        esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
        if (err == ESP_OK) {
            size_t required_size = 0;
            err = nvs_get_blob(my_handle, "config_json", NULL, &required_size);
            if (err == ESP_OK && required_size > 0) {
                char *json_buf = malloc(required_size);
                if (json_buf) {
                    err = nvs_get_blob(my_handle, "config_json", json_buf, &required_size);
                    nvs_close(my_handle);
                    if (err == ESP_OK) {
                        return json_buf;
                    }
                    free(json_buf);
                }
            }
            nvs_close(my_handle);
        }
    }

    // Fallback: Generate JSON representation of default_config using cJSON
    const lever_system_config_t *curr = &default_config;
    
    cJSON *root = cJSON_CreateObject();
    
    if (curr->wifi_password) {
        cJSON_AddStringToObject(root, "wifi_password", curr->wifi_password);
    } else {
        cJSON_AddStringToObject(root, "wifi_password", "signalman"); // Default
    }
    
    if (curr->wifi_ssid) {
        cJSON_AddStringToObject(root, "wifi_ssid", curr->wifi_ssid);
    } else {
        cJSON_AddStringToObject(root, "wifi_ssid", ""); // Default fallback
    }

    if (curr->wifi_station_password) {
        cJSON_AddStringToObject(root, "wifi_station_password", curr->wifi_station_password);
    } else {
        cJSON_AddStringToObject(root, "wifi_station_password", ""); // Default fallback
    }

    if (curr->jmri_ip_address) {
        cJSON_AddStringToObject(root, "jmri_ip_address", curr->jmri_ip_address);
    } else {
        cJSON_AddStringToObject(root, "jmri_ip_address", "");
    }
    
    cJSON_AddBoolToObject(root, "restore_last_state", curr->restore_last_state);
    cJSON_AddNumberToObject(root, "conflict_policy", curr->conflict_policy);
    cJSON_AddNumberToObject(root, "display_sleep_timeout_ms", curr->display_sleep_timeout_ms);
    cJSON_AddBoolToObject(root, "lcc_enabled", curr->lcc_enabled);
    
    cJSON *tabs_arr = cJSON_CreateArray();
    cJSON_AddItemToObject(root, "tabs", tabs_arr);

    for (size_t t = 0; t < curr->tab_count; t++) {
        cJSON *tab_obj = cJSON_CreateObject();
        cJSON_AddStringToObject(tab_obj, "name", curr->tabs[t].name);
        cJSON_AddNumberToObject(tab_obj, "label_lines", curr->tabs[t].label_lines > 0 ? curr->tabs[t].label_lines : 2);
        cJSON_AddNumberToObject(tab_obj, "label_line_height", curr->tabs[t].label_line_height > 0 ? curr->tabs[t].label_line_height : 18);
        cJSON *levers_arr = cJSON_CreateArray();
        cJSON_AddItemToObject(tab_obj, "levers", levers_arr);

        for (size_t l = 0; l < curr->tabs[t].lever_count; l++) {
            cJSON *lever_obj = cJSON_CreateObject();
            cJSON_AddStringToObject(lever_obj, "label", curr->tabs[t].levers[l].label);
            cJSON_AddStringToObject(lever_obj, "type", lever_type_to_str(curr->tabs[t].levers[l].type));
            
            if (strlen(curr->tabs[t].levers[l].lcc_event_normal) > 0) {
                cJSON_AddStringToObject(lever_obj, "lcc_event_normal", curr->tabs[t].levers[l].lcc_event_normal);
            }
            if (strlen(curr->tabs[t].levers[l].lcc_event_reversed) > 0) {
                cJSON_AddStringToObject(lever_obj, "lcc_event_reversed", curr->tabs[t].levers[l].lcc_event_reversed);
            }
            cJSON_AddBoolToObject(lever_obj, "lcc_enabled", curr->tabs[t].levers[l].lcc_enabled);
            
            cJSON *interlocking_arr = cJSON_CreateArray();
            for(int c = 0; c < MAX_INTERLOCKING_CONDITIONS; c++) {
                if (curr->tabs[t].levers[l].conditions[c].target_lever_index >= 0) {
                    cJSON *cond_obj = cJSON_CreateObject();
                    cJSON_AddNumberToObject(cond_obj, "target", curr->tabs[t].levers[l].conditions[c].target_lever_index);
                    cJSON_AddStringToObject(cond_obj, "state", curr->tabs[t].levers[l].conditions[c].required_state ? "REVERSED" : "NORMAL");
                    cJSON_AddNumberToObject(cond_obj, "alt_target", curr->tabs[t].levers[l].conditions[c].alt_target_lever_index);
                    cJSON_AddStringToObject(cond_obj, "alt_state", curr->tabs[t].levers[l].conditions[c].alt_required_state ? "REVERSED" : "NORMAL");
                    cJSON_AddItemToArray(interlocking_arr, cond_obj);
                }
            }
            if (cJSON_GetArraySize(interlocking_arr) > 0) {
                cJSON_AddItemToObject(lever_obj, "interlocking", interlocking_arr);
            } else {
                cJSON_Delete(interlocking_arr);
            }
            
            cJSON_AddItemToArray(levers_arr, lever_obj);
        }
        cJSON_AddItemToArray(tabs_arr, tab_obj);
    }

    char *json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    return json_str;
}

/**
 * @brief  Update configuration using a cJSON callback.
 *
 * Retrieves the current configuration as a JSON string, parses it into a cJSON 
 * tree, applies the provided callback function to modify the tree, and then 
 * saves the updated JSON back to NVS.
 *
 * @param[in]  update_cb   Callback function to modify the cJSON tree.
 * @param[in]  ctx         Context pointer passed to the callback.
 * @param[in]  notify      Whether to trigger a configuration reload event.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_ERR_NO_MEM if memory allocation fails
 */
static esp_err_t update_config_with_cjson(void (*update_cb)(cJSON *root, void *ctx), void *ctx, bool notify) {
    char *json_str = config_manager_get_json_str();
    if (!json_str) return ESP_ERR_NO_MEM;
    
    cJSON *root = cJSON_Parse(json_str);
    free(json_str);
    if (!root) return ESP_ERR_INVALID_ARG;
    
    update_cb(root, ctx);
    
    char *new_json_str = cJSON_PrintUnformatted(root);
    cJSON_Delete(root);
    if (!new_json_str) return ESP_ERR_NO_MEM;
    
    esp_err_t err = config_manager_save_json_internal(new_json_str, notify);
    free(new_json_str);
    return err;
}

struct global_bool_update_ctx {
    const char *key;
    bool value;
};

/**
 * @brief  cJSON callback to update a global boolean.
 *
 * Modifies the provided cJSON tree to update or add a global boolean value 
 * using the key and value specified in the context structure.
 *
 * @param[in]  root   Pointer to the root of the cJSON tree.
 * @param[in]  ctx    Context containing the key and boolean value.
 */
static void update_global_bool_cb(cJSON *root, void *ctx) {
    struct global_bool_update_ctx *c = ctx;
    cJSON *newitem = cJSON_CreateBool(c->value);
    if (cJSON_HasObjectItem(root, c->key)) {
        cJSON_ReplaceItemInObjectCaseSensitive(root, c->key, newitem);
    } else {
        cJSON_AddItemToObject(root, c->key, newitem);
    }
}

esp_err_t config_manager_update_global_bool(const char *key, bool value) {
    struct global_bool_update_ctx ctx = {key, value};
    return update_config_with_cjson(update_global_bool_cb, &ctx, false);
}

struct global_int_update_ctx {
    const char *key;
    int value;
};

/**
 * @brief  cJSON callback to update a global integer.
 *
 * Modifies the provided cJSON tree to update or add a global integer value 
 * using the key and value specified in the context structure.
 *
 * @param[in]  root   Pointer to the root of the cJSON tree.
 * @param[in]  ctx    Context containing the key and integer value.
 */
static void update_global_int_cb(cJSON *root, void *ctx) {
    struct global_int_update_ctx *c = ctx;
    cJSON *newitem = cJSON_CreateNumber(c->value);
    if (cJSON_HasObjectItem(root, c->key)) {
        cJSON_ReplaceItemInObjectCaseSensitive(root, c->key, newitem);
    } else {
        cJSON_AddItemToObject(root, c->key, newitem);
    }
}

esp_err_t config_manager_update_global_int(const char *key, int value) {
    struct global_int_update_ctx ctx = {key, value};
    return update_config_with_cjson(update_global_int_cb, &ctx, false);
}

struct lever_bool_update_ctx {
    int tab_idx;
    int lever_idx;
    const char *key;
    bool value;
};

/**
 * @brief  cJSON callback to update a lever boolean.
 *
 * Modifies the provided cJSON tree to update or add a boolean value for 
 * a specific lever using the indices, key, and value specified in the context.
 *
 * @param[in]  root   Pointer to the root of the cJSON tree.
 * @param[in]  ctx    Context containing indices, key, and boolean value.
 */
static void update_lever_bool_cb(cJSON *root, void *ctx) {
    struct lever_bool_update_ctx *c = ctx;
    cJSON *tabs = cJSON_GetObjectItem(root, "tabs");
    if (!tabs || !cJSON_IsArray(tabs)) return;
    
    cJSON *tab = cJSON_GetArrayItem(tabs, c->tab_idx);
    if (!tab) return;
    
    cJSON *levers = cJSON_GetObjectItem(tab, "levers");
    if (!levers || !cJSON_IsArray(levers)) return;
    
    cJSON *lever = cJSON_GetArrayItem(levers, c->lever_idx);
    if (!lever) return;
    
    cJSON *newitem = cJSON_CreateBool(c->value);
    if (cJSON_HasObjectItem(lever, c->key)) {
        cJSON_ReplaceItemInObjectCaseSensitive(lever, c->key, newitem);
    } else {
        cJSON_AddItemToObject(lever, c->key, newitem);
    }
}

esp_err_t config_manager_update_lever_bool(int tab_idx, int lever_idx, const char *key, bool value) {
    struct lever_bool_update_ctx ctx = {tab_idx, lever_idx, key, value};
    return update_config_with_cjson(update_lever_bool_cb, &ctx, false);
}



void config_manager_deinit(void) {
    if (is_using_dynamic) {
        free_dynamic_config(&active_dynamic_config);
        is_using_dynamic = false;
    }
    config_manager_free_pending();
}
