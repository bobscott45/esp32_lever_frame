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
 * @file      system_events.h
 * @brief     Definitions for system_events.h
 *
 * @author    Robert Scott
 * @date      2026
 */

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
