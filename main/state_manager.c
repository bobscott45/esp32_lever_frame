#include "state_manager.h"
#include "nvs.h"
#include "esp_log.h"
#include "lever_frame.h"

static const char *TAG = "StateManager";

// Helper to iterate through the tabview and extract/apply state
static void iterate_levers(lv_obj_t *wrapper, uint8_t *states, bool apply) {
    if (!wrapper) return;
    lv_obj_t *tv = lv_obj_get_child(wrapper, 0);
    if (!tv) return;
    lv_obj_t *content = lv_tabview_get_content(tv);
    if (!content) return;
    
    uint32_t tab_count = lv_obj_get_child_cnt(content);
    int lever_idx = 0;
    
    for (uint32_t t = 0; t < tab_count; t++) {
        lv_obj_t *tab = lv_obj_get_child(content, t);
        if (!tab) continue;
        
        lv_obj_t *frame = lv_obj_get_child(tab, 0);
        if (!frame) continue;
        
        uint32_t l_count = lv_obj_get_child_cnt(frame);
        for (uint32_t l = 0; l < l_count; l++) {
            lv_obj_t *l_wrapper = lv_obj_get_child(frame, l);
            if (!l_wrapper) continue;
            
            lv_obj_t *container = lv_obj_get_child(l_wrapper, 0);
            if (!container) continue;
            
            lv_obj_t *switch_group = lv_obj_get_child(container, 1);
            if (!switch_group) continue;
            
            lv_obj_t *sw = lv_obj_get_child(switch_group, 1);
            if (!sw) continue;
            lv_obj_t *collar_btn = lv_obj_get_child(l_wrapper, 1);
            if (!collar_btn) continue;
            
            int sw_bit_idx = lever_idx * 2;
            int lock_bit_idx = lever_idx * 2 + 1;
            
            if (apply) {
                // Apply state
                bool is_reversed = (states[sw_bit_idx / 8] & (1 << (sw_bit_idx % 8))) != 0;
                bool is_locked = (states[lock_bit_idx / 8] & (1 << (lock_bit_idx % 8))) != 0;
                
                if (is_reversed) {
                    lv_obj_add_state(sw, LV_STATE_CHECKED);
                } else {
                    lv_obj_clear_state(sw, LV_STATE_CHECKED);
                }
                
                if (is_locked) {
                    lv_obj_add_state(collar_btn, LV_STATE_CHECKED);
                } else {
                    lv_obj_clear_state(collar_btn, LV_STATE_CHECKED);
                }
                
                // Send an event so the switch UI updates, but use a custom code to avoid infinite save loops
                lv_obj_send_event(sw, LV_EVENT_REFRESH, NULL);
                lv_obj_send_event(collar_btn, LV_EVENT_REFRESH, NULL);
            } else {
                // Save state
                if (lv_obj_has_state(sw, LV_STATE_CHECKED)) {
                    states[sw_bit_idx / 8] |= (1 << (sw_bit_idx % 8));
                } else {
                    states[sw_bit_idx / 8] &= ~(1 << (sw_bit_idx % 8));
                }
                
                if (lv_obj_has_state(collar_btn, LV_STATE_CHECKED)) {
                    states[lock_bit_idx / 8] |= (1 << (lock_bit_idx % 8));
                } else {
                    states[lock_bit_idx / 8] &= ~(1 << (lock_bit_idx % 8));
                }
            }
            lever_idx++;
        }
        
        // After applying all states to a frame, update the interlocking logic
        if (apply) {
            lever_frame_update_system_locks(frame);
        }
    }
}

void state_manager_load_and_apply(lv_obj_t *system_wrapper, uint32_t current_config_hash) {
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READONLY, &my_handle);
    if (err != ESP_OK) {
        ESP_LOGI(TAG, "No NVS or no saved state found. Booting with default safe state.");
        return;
    }
    
    uint32_t saved_hash = 0;
    err = nvs_get_u32(my_handle, "cfg_hash", &saved_hash);
    if (err != ESP_OK || saved_hash != current_config_hash) {
        ESP_LOGW(TAG, "Config hash mismatch (saved: %lu, current: %lu). Layout changed! Wiping state and booting to safe defaults.", saved_hash, current_config_hash);
        nvs_close(my_handle);
        return;
    }
    
    size_t required_size = 0;
    err = nvs_get_blob(my_handle, "lever_st_v2", NULL, &required_size);
    if (err == ESP_OK && required_size > 0) {
        uint8_t *states = calloc(1, required_size);
        if (states) {
            nvs_get_blob(my_handle, "lever_st_v2", states, &required_size);
            ESP_LOGI(TAG, "Restoring %d bytes of lever state from NVS...", required_size);
            iterate_levers(system_wrapper, states, true);
            free(states);
        }
    }
    nvs_close(my_handle);
}

void state_manager_save(lv_obj_t *system_wrapper, uint32_t current_config_hash) {
    if (!system_wrapper) return;
    lv_obj_t *tv = lv_obj_get_child(system_wrapper, 0);
    if (!tv) return;
    lv_obj_t *content = lv_tabview_get_content(tv);
    if (!content) return;
    
    int total_levers = 0;
    uint32_t tab_count = lv_obj_get_child_cnt(content);
    for (uint32_t t = 0; t < tab_count; t++) {
        lv_obj_t *tab = lv_obj_get_child(content, t);
        if (!tab) continue;
        lv_obj_t *frame = lv_obj_get_child(tab, 0);
        if (!frame) continue;
        total_levers += lv_obj_get_child_cnt(frame);
    }
    
    int bytes_needed = (total_levers * 2 + 7) / 8;
    if (bytes_needed == 0) return;
    
    uint8_t *states = calloc(1, bytes_needed);
    if (!states) return;
    
    iterate_levers(system_wrapper, states, false);
    
    nvs_handle_t my_handle;
    esp_err_t err = nvs_open("storage", NVS_READWRITE, &my_handle);
    if (err == ESP_OK) {
        nvs_set_u32(my_handle, "cfg_hash", current_config_hash);
        nvs_set_blob(my_handle, "lever_st_v2", states, bytes_needed);
        nvs_commit(my_handle);
        nvs_close(my_handle);
        ESP_LOGI(TAG, "Lever state automatically saved to NVS.");
    }
    
    free(states);
}
