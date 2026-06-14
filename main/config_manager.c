#include "config_manager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_log.h"
#include "cJSON.h"

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
    .restore_last_state = true
};

// Currently active dynamic configuration
static lever_system_config_t active_dynamic_config = {0};
static bool is_using_dynamic = false;
static uint32_t active_config_hash = 0;

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
static config_change_cb_t on_config_change = NULL;

const char *lever_type_to_str(lever_type_t type) {
    switch (type) {
        case LEVER_TYPE_HOME_SIGNAL:    return "HOME_SIGNAL";
        case LEVER_TYPE_DISTANT_SIGNAL: return "DISTANT_SIGNAL";
        case LEVER_TYPE_POINTS:         return "POINTS";
        case LEVER_TYPE_FACING_POINTS:  return "FACING_POINTS";
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
    return LEVER_TYPE_SPARE;
}

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
    memset(config, 0, sizeof(lever_system_config_t));
}

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
        ESP_LOGI(TAG, "No NVS partition found. Using default compile-time config.");
        return ESP_OK;
    } else if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error opening NVS handle: %s", esp_err_to_name(err));
        return err;
    }

    size_t required_size = 0;
    err = nvs_get_blob(my_handle, "config_json", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No config_json found in NVS. Using default compile-time config.");
        nvs_close(my_handle);
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

esp_err_t config_manager_save_json(const char *json_str) {
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
        free_dynamic_config(&active_dynamic_config);
    }
    active_dynamic_config = new_config;
    is_using_dynamic = true;
    active_config_hash = hash_json_string(json_str);

    ESP_LOGI(TAG, "Dynamic configuration updated and saved to NVS.");

    // Trigger callback if registered
    if (on_config_change) {
        on_config_change();
    }

    return ESP_OK;
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
        cJSON_AddStringToObject(root, "wifi_ssid", "ZENBQ16"); // Default fallback
    }

    if (curr->wifi_station_password) {
        cJSON_AddStringToObject(root, "wifi_station_password", curr->wifi_station_password);
    } else {
        cJSON_AddStringToObject(root, "wifi_station_password", "Juniper#1945"); // Default fallback
    }
    
    cJSON_AddBoolToObject(root, "restore_last_state", curr->restore_last_state);
    cJSON_AddNumberToObject(root, "conflict_policy", curr->conflict_policy);
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

void config_manager_set_on_change(config_change_cb_t cb) {
    on_config_change = cb;
}

void config_manager_deinit(void) {
    if (is_using_dynamic) {
        free_dynamic_config(&active_dynamic_config);
        is_using_dynamic = false;
    }
}
