#include "controller.h"
#include <stdlib.h>
#include <string.h>

static bool **s_tab_lever_states = NULL;
static size_t s_tab_count = 0;
static size_t *s_lever_counts = NULL;

void controller_init(const lever_system_config_t *config) {
    controller_free();
    if (!config || !config->tabs) return;

    s_tab_count = config->tab_count;
    if (s_tab_count > 0) {
        s_tab_lever_states = calloc(s_tab_count, sizeof(bool *));
        s_lever_counts = calloc(s_tab_count, sizeof(size_t));
        
        for (size_t t = 0; t < s_tab_count; t++) {
            s_lever_counts[t] = config->tabs[t].lever_count;
            if (s_lever_counts[t] > 0) {
                s_tab_lever_states[t] = calloc(s_lever_counts[t], sizeof(bool));
            }
        }
    }
}

void controller_free(void) {
    if (s_tab_lever_states) {
        for (size_t t = 0; t < s_tab_count; t++) {
            if (s_tab_lever_states[t]) {
                free(s_tab_lever_states[t]);
            }
        }
        free(s_tab_lever_states);
        s_tab_lever_states = NULL;
    }
    if (s_lever_counts) {
        free(s_lever_counts);
        s_lever_counts = NULL;
    }
    s_tab_count = 0;
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
