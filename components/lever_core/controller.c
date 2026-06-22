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
 * @file      controller.c
 * @brief     Implementation of controller.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "controller.h"
#include "interlocking.h"
#include <stdlib.h>
#include <string.h>

static bool **s_tab_lever_states = NULL;
static bool **s_tab_lever_locks = NULL;
static int *s_lever_counts = NULL;
static int s_tab_count = 0;
static int s_active_tab_index = 0;
#include "system_events.h"

void controller_init(const lever_system_config_t *config) {
    controller_free();
    
    if (!config || config->tab_count == 0) return;
    
    s_tab_count = config->tab_count;
    s_tab_lever_states = calloc(s_tab_count, sizeof(bool *));
    s_tab_lever_locks = calloc(s_tab_count, sizeof(bool *));
    s_lever_counts = calloc(s_tab_count, sizeof(int));
    s_active_tab_index = 0;
    
    if (!s_tab_lever_states || !s_tab_lever_locks || !s_lever_counts) {
        controller_free();
        return;
    }
    
    for (int i = 0; i < s_tab_count; i++) {
        s_lever_counts[i] = config->tabs[i].lever_count;
        if (s_lever_counts[i] > 0) {
            s_tab_lever_states[i] = calloc(s_lever_counts[i], sizeof(bool));
            s_tab_lever_locks[i] = calloc(s_lever_counts[i], sizeof(bool));
        }
    }
}

void controller_free(void) {
    if (s_tab_lever_states) {
        for (int i = 0; i < s_tab_count; i++) {
            if (s_tab_lever_states[i]) free(s_tab_lever_states[i]);
        }
        free(s_tab_lever_states);
        s_tab_lever_states = NULL;
    }
    if (s_tab_lever_locks) {
        for (int i = 0; i < s_tab_count; i++) {
            if (s_tab_lever_locks[i]) free(s_tab_lever_locks[i]);
        }
        free(s_tab_lever_locks);
        s_tab_lever_locks = NULL;
    }
    if (s_lever_counts) {
        free(s_lever_counts);
        s_lever_counts = NULL;
    }
    s_tab_count = 0;
    s_active_tab_index = 0;
}

bool controller_get_lever_state(int tab_index, int lever_index) {
    if (!s_tab_lever_states || tab_index < 0 || tab_index >= s_tab_count) return false;
    if (lever_index < 0 || lever_index >= s_lever_counts[tab_index]) return false;
    return s_tab_lever_states[tab_index][lever_index];
}

void controller_set_lever_state(int tab_index, int lever_index, bool is_thrown) {
    if (!s_tab_lever_states || tab_index < 0 || tab_index >= s_tab_count) return;
    if (lever_index < 0 || lever_index >= s_lever_counts[tab_index]) return;
    s_tab_lever_states[tab_index][lever_index] = is_thrown;
}

const bool* controller_get_tab_states(int tab_index) {
    if (!s_tab_lever_states || tab_index < 0 || tab_index >= s_tab_count) return NULL;
    return s_tab_lever_states[tab_index];
}

bool controller_get_lever_lock(int tab_index, int lever_index) {
    if (!s_tab_lever_locks || tab_index < 0 || tab_index >= s_tab_count) return false;
    if (lever_index < 0 || lever_index >= s_lever_counts[tab_index]) return false;
    return s_tab_lever_locks[tab_index][lever_index];
}

void controller_set_lever_lock(int tab_index, int lever_index, bool is_locked) {
    if (!s_tab_lever_locks || tab_index < 0 || tab_index >= s_tab_count) return;
    if (lever_index < 0 || lever_index >= s_lever_counts[tab_index]) return;
    s_tab_lever_locks[tab_index][lever_index] = is_locked;
}

int controller_get_active_tab(void) {
    return s_active_tab_index;
}

void controller_set_active_tab(int tab_index) {
    if (tab_index >= 0 && tab_index < s_tab_count) {
        s_active_tab_index = tab_index;
    }
}



bool controller_request_lever_move(int tab_index, int lever_index, bool target_state_thrown) {
    if (!s_tab_lever_states || tab_index < 0 || tab_index >= s_tab_count) return false;
    if (lever_index < 0 || lever_index >= s_lever_counts[tab_index]) return false;
    
    const lever_system_config_t *config = config_manager_get_current();
    if (!config || tab_index >= config->tab_count) return false;
    
    const tab_def_t *tab_def = &config->tabs[tab_index];
    const bool *lever_states = s_tab_lever_states[tab_index];
    
    // Ask the interlocking model if this is allowed
    bool allowed = lever_evaluate_interlocking(tab_def, lever_states, lever_index, target_state_thrown);
    
    if (allowed) {
        s_tab_lever_states[tab_index][lever_index] = target_state_thrown;
        event_lever_state_t ev_data = {
            .tab_index = tab_index,
            .lever_index = lever_index,
            .new_state = target_state_thrown
        };
        esp_event_post(LEVER_SYSTEM_EVENTS, EVENT_LEVER_STATE_CHANGED, &ev_data, sizeof(ev_data), portMAX_DELAY);
        return true;
    }
    
    return false;
}

// Format: 1 byte for active tab + N bytes for lever states (2 bits per lever)
size_t controller_get_serialized_size(void) {
    int total_levers = 0;
    for (int i = 0; i < s_tab_count; i++) {
        total_levers += s_lever_counts[i];
    }
    return 1 + (total_levers * 2 + 7) / 8;
}

bool controller_get_serialized_state(uint8_t *buffer, size_t max_len) {
    if (!buffer || max_len < controller_get_serialized_size()) return false;
    memset(buffer, 0, controller_get_serialized_size());
    
    buffer[0] = (uint8_t)s_active_tab_index;
    
    int global_lever_idx = 0;
    for (int t = 0; t < s_tab_count; t++) {
        for (int l = 0; l < s_lever_counts[t]; l++) {
            int sw_bit = global_lever_idx * 2;
            int lock_bit = global_lever_idx * 2 + 1;
            
            if (s_tab_lever_states[t][l]) {
                buffer[1 + sw_bit / 8] |= (1 << (sw_bit % 8));
            }
            if (s_tab_lever_locks[t][l]) {
                buffer[1 + lock_bit / 8] |= (1 << (lock_bit % 8));
            }
            global_lever_idx++;
        }
    }
    return true;
}

bool controller_apply_serialized_state(const uint8_t *buffer, size_t len) {
    if (!buffer || len < controller_get_serialized_size()) return false;
    
    if (buffer[0] < s_tab_count) {
        s_active_tab_index = buffer[0];
    }
    
    int global_lever_idx = 0;
    for (int t = 0; t < s_tab_count; t++) {
        for (int l = 0; l < s_lever_counts[t]; l++) {
            int sw_bit = global_lever_idx * 2;
            int lock_bit = global_lever_idx * 2 + 1;
            
            s_tab_lever_states[t][l] = (buffer[1 + sw_bit / 8] & (1 << (sw_bit % 8))) != 0;
            s_tab_lever_locks[t][l] = (buffer[1 + lock_bit / 8] & (1 << (lock_bit % 8))) != 0;
            global_lever_idx++;
        }
    }
    return true;
}
