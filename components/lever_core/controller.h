#ifndef CONTROLLER_H
#define CONTROLLER_H

#include <stdbool.h>
#include <stddef.h>
#include "config_manager.h"

void controller_init(const lever_system_config_t *config);
void controller_free(void);

// Tab and lever indices are 0-based
bool controller_get_lever_state(int tab_index, int lever_index);
void controller_set_lever_state(int tab_index, int lever_index, bool is_thrown);

// Manual locks
bool controller_get_lever_lock(int tab_index, int lever_index);
void controller_set_lever_lock(int tab_index, int lever_index, bool is_locked);

// Active Tab
int controller_get_active_tab(void);
void controller_set_active_tab(int tab_index);


// Serialization
size_t controller_get_serialized_size(void);
bool controller_get_serialized_state(uint8_t *buffer, size_t max_len);
bool controller_apply_serialized_state(const uint8_t *buffer, size_t len);

// Returns a pointer to the array of lever states for a specific tab
const bool* controller_get_tab_states(int tab_index);

// Request a move (returns true if accepted, false if blocked by interlocking)
bool controller_request_lever_move(int tab_index, int lever_index, bool target_state_thrown);

#endif // CONTROLLER_H
