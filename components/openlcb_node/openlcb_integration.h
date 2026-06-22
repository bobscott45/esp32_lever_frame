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
 * @file      openlcb_integration.h
 * @brief     Definitions for openlcb_integration.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef OPENLCB_INTEGRATION_H
#define OPENLCB_INTEGRATION_H

#include <stdint.h>
#include "openlcb/openlcb_types.h"

/**
 * @brief  Initializes the OpenLCB integration.
 *
 * Checks if LCC master is enabled in configuration, initializes drivers 
 * (LCC, CAN, TCP), creates the local OpenLCB node, and registers event 
 * producers and consumers based on the system configuration. Finally, it 
 * starts a FreeRTOS task to run the OpenLCB state machine.
 */
void openlcb_integration_init(void);

/**
 * @brief  Produces an OpenLCB event.
 *
 * Sends an OpenLCB Producer-Consumer Event Report (PCER) over the 
 * network if a local node is active and the event ID is valid (non-zero).
 *
 * @param[in]  event_id   The 64-bit event ID to produce.
 */
void openlcb_produce_event(event_id_t event_id);

/**
 * @brief  Parses a string representation of an event ID.
 *
 * Converts a hexadecimal string representing an event ID into its 
 * corresponding 64-bit integer value. Stops parsing after 16 valid 
 * hexadecimal characters.
 *
 * @param[in]  str   String representation of the event ID.
 * 
 * @return 
 *   - The parsed event ID on success
 *   - 0 if the string is invalid or incomplete
 */
event_id_t lcc_parse_event_id(const char *str);

/**
 * @brief  Updates lever states based on a received event.
 *
 * Iterates through the active system configuration and checks if the 
 * received event matches any lever's normal or reversed event ID. 
 * If a match is found and the lever is not already in the target state, 
 * a movement request is made subject to local interlocking policy.
 *
 * @param[in]  event_id   The 64-bit event ID that was received.
 */
void openlcb_integration_update_levers_by_event(event_id_t event_id);

extern openlcb_node_t *local_node;

#endif // OPENLCB_INTEGRATION_H
