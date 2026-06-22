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

// Returns a pointer to the array of lever states for a specific tab
const bool* controller_get_tab_states(int tab_index);

// Request a move (returns true if accepted, false if blocked by interlocking)
bool controller_request_lever_move(int tab_index, int lever_index, bool target_state_thrown);

#endif // CONTROLLER_H
