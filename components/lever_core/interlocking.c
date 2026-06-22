#include "interlocking.h"

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
