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
 * @file      interlocking.c
 * @brief     Implementation of interlocking.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "interlocking.h"

/**
 * @brief  Evaluate a single interlocking condition.
 *
 * This function checks whether a specific interlocking condition is met given the current
 * state of all levers and the proposed state of a lever that is changing. It also handles
 * alternative conditions if they are specified.
 *
 * @param[in]  cond                   The interlocking condition to evaluate.
 * @param[in]  tab_def                The configuration rules for the current frame.
 * @param[in]  lever_states           A boolean array representing the current state of all levers.
 * @param[in]  changing_lever_idx     The index of the lever that is changing state.
 * @param[in]  changing_lever_state   The proposed new state of the changing lever.
 * 
 * @return 
 *   - true if the condition is met
 *   - false if the condition is not met
 */
static bool evaluate_condition(const interlocking_condition_t *cond, const tab_def_t *tab_def, const bool *lever_states, int changing_lever_idx, bool changing_lever_state) {
    if (cond->target_lever_index < 0) return true;
    
    bool target_state;
    if (cond->target_lever_index == changing_lever_idx) {
        target_state = changing_lever_state;
    } else if (cond->target_lever_index < tab_def->lever_count) {
        target_state = lever_states[cond->target_lever_index];
    } else {
        target_state = false;
    }
    bool target_met = (target_state == cond->required_state);
    
    bool alt_met = false;
    if (cond->alt_target_lever_index >= 0 && cond->alt_target_lever_index < tab_def->lever_count) {
        bool alt_state;
        if (cond->alt_target_lever_index == changing_lever_idx) {
            alt_state = changing_lever_state;
        } else {
            alt_state = lever_states[cond->alt_target_lever_index];
        }
        alt_met = (alt_state == cond->alt_required_state);
    }
    
    return target_met || alt_met;
}

bool lever_evaluate_interlocking(const tab_def_t *tab_def, const bool *lever_states, int lever_index_to_move, bool target_state_thrown) {
    if (!tab_def || !lever_states) return true;

    if (target_state_thrown) {
        const lever_def_t *my_def = &tab_def->levers[lever_index_to_move];
        for (int i = 0; i < MAX_INTERLOCKING_CONDITIONS; i++) {
            if (my_def->conditions[i].target_lever_index < 0) break;
            if (!evaluate_condition(&my_def->conditions[i], tab_def, lever_states, lever_index_to_move, target_state_thrown)) {
                return false;
            }
        }
    }

    for (int i = 0; i < tab_def->lever_count; i++) {
        if (i == lever_index_to_move) continue;
        
        if (lever_states[i]) {
            const lever_def_t *other_def = &tab_def->levers[i];
            for (int c = 0; c < MAX_INTERLOCKING_CONDITIONS; c++) {
                if (other_def->conditions[c].target_lever_index < 0) break;
                if (!evaluate_condition(&other_def->conditions[c], tab_def, lever_states, lever_index_to_move, target_state_thrown)) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool lever_is_state_illegal(const tab_def_t *tab_def, const bool *lever_states, int lever_index_to_check) {
    if (!tab_def || !lever_states) return false;

    bool is_thrown = lever_states[lever_index_to_check];

    if (is_thrown) {
        // If this lever is thrown, are its conditions met?
        const lever_def_t *my_def = &tab_def->levers[lever_index_to_check];
        for (int i = 0; i < MAX_INTERLOCKING_CONDITIONS; i++) {
            if (my_def->conditions[i].target_lever_index < 0) break;
            if (!evaluate_condition(&my_def->conditions[i], tab_def, lever_states, lever_index_to_check, is_thrown)) {
                return true; // Illegal! My condition is not met.
            }
        }
    }
    
    // Regardless of my state, does any OTHER thrown lever have a condition involving ME that is currently failing?
    for (int i = 0; i < tab_def->lever_count; i++) {
        if (i == lever_index_to_check) continue;
        
        if (lever_states[i]) {
            const lever_def_t *other_def = &tab_def->levers[i];
            for (int c = 0; c < MAX_INTERLOCKING_CONDITIONS; c++) {
                if (other_def->conditions[c].target_lever_index < 0) break;
                
                // We only care if THIS condition involves THIS lever
                if (other_def->conditions[c].target_lever_index == lever_index_to_check ||
                    other_def->conditions[c].alt_target_lever_index == lever_index_to_check) {
                    
                    // Evaluate the condition with my CURRENT state
                    if (!evaluate_condition(&other_def->conditions[c], tab_def, lever_states, lever_index_to_check, is_thrown)) {
                        return true; // Illegal! Another lever's condition on me is failing.
                    }
                }
            }
        }
    }
    return false;
}
