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

static const char *TAG = "OPENLCB_INT";

openlcb_node_t *local_node = NULL;

static void on_consumed_event_pcer(openlcb_node_t *openlcb_node, uint16_t index, event_id_t *event_id, event_payload_t *payload) {
    ESP_LOGI(TAG, "Consumed Event Received! EventID: 0x%016llX", (unsigned long long)*event_id);
    // TODO: Determine which lever this belongs to and update state
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

static event_id_t lcc_to_event_id(lcc_event_id_t lcc_event) {
    event_id_t eid = 0;
    for(int j = 0; j < 8; j++) {
        eid = (eid << 8) | lcc_event.bytes[j];
    }
    return eid;
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
        // Register producers and consumers based on the config memory
        node_config_memory_t* mem = lcc_drivers_get_config();
        if (mem) {
            for (int i = 0; i < 16; i++) {
                // Register consumers (servos that listen for events)
                event_id_t closed_eid = lcc_to_event_id(mem->servos[i].closed_event);
                if (closed_eid != 0) {
                    OpenLcbApplication_register_consumer_eventid(local_node, closed_eid, EVENT_STATUS_UNKNOWN);
                }

                event_id_t thrown_eid = lcc_to_event_id(mem->servos[i].thrown_event);
                if (thrown_eid != 0) {
                    OpenLcbApplication_register_consumer_eventid(local_node, thrown_eid, EVENT_STATUS_UNKNOWN);
                }

                // Register producers (switches/levers that generate events)
                event_id_t active_eid = lcc_to_event_id(mem->switches[i].active_event);
                if (active_eid != 0) {
                    OpenLcbApplication_register_producer_eventid(local_node, active_eid, EVENT_STATUS_UNKNOWN);
                }

                event_id_t inactive_eid = lcc_to_event_id(mem->switches[i].inactive_event);
                if (inactive_eid != 0) {
                    OpenLcbApplication_register_producer_eventid(local_node, inactive_eid, EVENT_STATUS_UNKNOWN);
                }
            }
            ESP_LOGI(TAG, "Registered %d consumers and %d producers.", local_node->consumers.count, local_node->producers.count);
        }
    } else {
        ESP_LOGE(TAG, "Failed to create OpenLCB Node!");
    }
    
    xTaskCreate(openlcb_task, "openlcb_task", 4096, NULL, 5, NULL);
}

void openlcb_produce_event(event_id_t event_id) {
    if (!local_node) return;
    if (event_id == 0) return;
    
    OpenLcbApplication_send_event_pc_report(local_node, event_id);
    ESP_LOGI(TAG, "Sent PCER for EventID: 0x%016llX", (unsigned long long)event_id);
}
