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

#ifndef OPENLCB_INTEGRATION_H
#define OPENLCB_INTEGRATION_H

#include <stdint.h>
#include "openlcb/openlcb_types.h"

void openlcb_integration_init(void);
void openlcb_produce_event(event_id_t event_id);
event_id_t lcc_parse_event_id(const char *str);
void openlcb_integration_update_levers_by_event(event_id_t event_id);

extern openlcb_node_t *local_node;

#endif // OPENLCB_INTEGRATION_H
