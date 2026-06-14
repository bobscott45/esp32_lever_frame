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
