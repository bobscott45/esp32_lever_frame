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
 * @file      openlcb_integration.c
 * @brief     Implementation of openlcb_integration.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "openlcb_integration.h"
#include "openlcb_user_config.h"
#include "openlcb/openlcb_config.h"
#include "openlcb/openlcb_application.h"
#include "drivers/canbus/can_config.h"
#include "lcc_drivers.h"
#include "can_drivers.h"
#include "tcp_drivers.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "state_manager.h"
#include "config_manager.h"
#include "controller.h"
#include "interlocking.h"
#include <string.h>

static const char *TAG = "OPENLCB_INT";

openlcb_node_t *local_node = NULL;

void openlcb_integration_update_levers_by_event(event_id_t event_id);

/**
 * @brief  Callback for a consumed PCER event.
 *
 * Invoked by the OpenLCB stack when a Producer-Consumer Event Report (PCER) 
 * is received for an event ID that this node is registered to consume. 
 * Triggers lever updates corresponding to the received event.
 *
 * @param[in]  openlcb_node   Pointer to the local OpenLCB node.
 * @param[in]  index          Index of the consumer that matched.
 * @param[in]  event_id       Pointer to the received 64-bit event ID.
 * @param[in]  payload        Pointer to the event payload data, if any.
 */
static void on_consumed_event_pcer(openlcb_node_t *openlcb_node, uint16_t index, event_id_t *event_id, event_payload_t *payload) {
    ESP_LOGI(TAG, "Consumed Event Received! EventID: 0x%016llX", (unsigned long long)*event_id);
    openlcb_integration_update_levers_by_event(*event_id);
}

static const openlcb_config_t openlcb_config = {
    .lock_shared_resources = lcc_drivers_lock_shared_resources,
    .unlock_shared_resources = lcc_drivers_unlock_shared_resources,
    .config_mem_read = lcc_drivers_config_mem_read,
    .config_mem_write = lcc_drivers_config_mem_write,
    .reboot = lcc_drivers_reboot,
    .on_consumed_event_pcer = on_consumed_event_pcer
};

#ifdef OPENLCB_COMPILE_CAN
static const can_config_t local_can_config = {
    .transmit_raw_can_frame = can_driver_transmit,
    .is_tx_buffer_clear = can_driver_is_clear,
    .lock_shared_resources = lcc_drivers_lock_shared_resources,
    .unlock_shared_resources = lcc_drivers_unlock_shared_resources,
};
#endif

/**
 * @brief  FreeRTOS task for running the OpenLCB state machine.
 *
 * Continuously calls the OpenLCB run function to process incoming messages 
 * and execute state machines. Also triggers the 100ms OpenLCB timer tick 
 * every 10 iterations.
 *
 * @param[in]  pvParameters   Task parameters provided during creation.
 */
static void openlcb_task(void *pvParameters) {
    int ticks = 0;
    while(1) {
        OpenLcbConfig_run();
        vTaskDelay(pdMS_TO_TICKS(10));
        ticks++;
        if (ticks >= 10) {
            ticks = 0;
            OpenLcbConfig_100ms_timer_tick();
        }
    }
}



void openlcb_integration_init(void) {
    const lever_system_config_t *curr_config = config_manager_get_current();
    if (!curr_config->lcc_enabled) {
        ESP_LOGI(TAG, "LCC Master is disabled in config. Skipping initialization.");
        return;
    }

    ESP_LOGI(TAG, "Initializing LCC Drivers...");
    lcc_drivers_initialize();
    
#ifdef OPENLCB_COMPILE_CAN
    // can_driver_initialize(); // Disabled physical CAN, only using GridConnect TCP
    CanConfig_initialize(&local_can_config);
#endif

    tcp_driver_initialize(); // Start GridConnect TCP Server

    OpenLcbConfig_initialize(&openlcb_config);
    
    // Register the node ID
    local_node = OpenLcbConfig_create_node(NODE_ID, &openlcb_user_config_node_parameters);

    if (local_node) {
        // Register producers and consumers based on active configuration
        const lever_system_config_t *system_config = config_manager_get_current();
        if (system_config && system_config->lcc_enabled) {
            int producers = 0;
            int consumers = 0;
            for (size_t t = 0; t < system_config->tab_count; t++) {
                const tab_def_t *tab = &system_config->tabs[t];
                for (size_t l = 0; l < tab->lever_count; l++) {
                    const lever_def_t *lever = &tab->levers[l];
                    if (!lever->lcc_enabled) continue;

                    event_id_t normal_eid = lcc_parse_event_id(lever->lcc_event_normal);
                    event_id_t reversed_eid = lcc_parse_event_id(lever->lcc_event_reversed);

                    if (normal_eid != 0) {
                        OpenLcbApplication_register_producer_eventid(local_node, normal_eid, EVENT_STATUS_UNKNOWN);
                        OpenLcbApplication_register_consumer_eventid(local_node, normal_eid, EVENT_STATUS_UNKNOWN);
                        producers++;
                        consumers++;
                    }

                    if (reversed_eid != 0) {
                        OpenLcbApplication_register_producer_eventid(local_node, reversed_eid, EVENT_STATUS_UNKNOWN);
                        OpenLcbApplication_register_consumer_eventid(local_node, reversed_eid, EVENT_STATUS_UNKNOWN);
                        producers++;
                        consumers++;
                    }
                }
            }
            ESP_LOGI(TAG, "Registered %d consumers and %d producers based on active config.", consumers, producers);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create OpenLCB Node!");
    }
    
    xTaskCreatePinnedToCore(openlcb_task, "openlcb_task", 4096, NULL, 5, NULL, 0);
}

void openlcb_produce_event(event_id_t event_id) {
    if (!local_node) return;
    if (event_id == 0) return;
    
    OpenLcbApplication_send_event_pc_report(local_node, event_id);
    ESP_LOGD(TAG, "Sent PCER for EventID: 0x%016llX", (unsigned long long)event_id);
}

event_id_t lcc_parse_event_id(const char *str) {
    if (!str || strlen(str) == 0) return 0;
    
    event_id_t event_id = 0;
    int nibbles = 0;
    
    for (int i = 0; str[i] != '\0' && nibbles < 16; i++) {
        char c = str[i];
        int val = -1;
        if (c >= '0' && c <= '9') val = c - '0';
        else if (c >= 'a' && c <= 'f') val = c - 'a' + 10;
        else if (c >= 'A' && c <= 'F') val = c - 'A' + 10;
        
        if (val != -1) {
            event_id = (event_id << 4) | val;
            nibbles++;
        }
    }
    
    if (nibbles < 16) {
        return 0;
    }
    
    return event_id;
}

void openlcb_integration_update_levers_by_event(event_id_t event_id) {
    const lever_system_config_t *sys_config = config_manager_get_current();
        if (!sys_config) return;

        for (size_t t = 0; t < sys_config->tab_count; t++) {
            const tab_def_t *tab = &sys_config->tabs[t];
            for (size_t l = 0; l < tab->lever_count; l++) {
                const lever_def_t *lever = &tab->levers[l];
                if (!lever->lcc_enabled) continue;

                event_id_t normal_eid = lcc_parse_event_id(lever->lcc_event_normal);
                event_id_t reversed_eid = lcc_parse_event_id(lever->lcc_event_reversed);

                if (normal_eid != 0 && normal_eid == event_id) {
                    if (controller_get_lever_state(t, l)) {
                        bool allowed = controller_request_lever_move(t, l, false);
                        if (!allowed) {
                            ESP_LOGI(TAG, "LCC normal event ignored due to strict local interlocking policy.");
                        }
                    }
                } else if (reversed_eid != 0 && reversed_eid == event_id) {
                    if (!controller_get_lever_state(t, l)) {
                        bool allowed = controller_request_lever_move(t, l, true);
                        if (!allowed) {
                            ESP_LOGI(TAG, "LCC reverse event ignored due to strict local interlocking policy.");
                        }
                    }
            }
        }
    }
}
