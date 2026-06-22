#ifndef SYSTEM_EVENTS_H
#define SYSTEM_EVENTS_H

#include "esp_event.h"
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Define the event base for the system
ESP_EVENT_DECLARE_BASE(LEVER_SYSTEM_EVENTS);

// Define event IDs
enum {
    EVENT_CONFIG_RELOADED,
    EVENT_LEVER_STATE_CHANGED,
    EVENT_NETWORK_CONNECTED,
};

// Event data for EVENT_LEVER_STATE_CHANGED
typedef struct {
    int tab_index;
    int lever_index;
    bool new_state;
} event_lever_state_t;

#ifdef __cplusplus
}
#endif

#endif // SYSTEM_EVENTS_H
