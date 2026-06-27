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
 * @file can_drivers.c
 * @brief Hardware driver implementations for CAN Drivers.
 *
 * Connect OpenLcbCLib to your CAN controller, timers, and
 * platform resources.
 *
 * @author Robert Scott
 * @date 2026
 */

    #include <string.h>
    #include "can_drivers.h"
    #include "lcc_drivers.h"
    #include "esp_log.h"
    #include "driver/gpio.h"
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wcpp"
    #include "driver/twai.h"
#pragma GCC diagnostic pop
    #include "freertos/FreeRTOS.h"
    #include "freertos/task.h"
    #include "drivers/canbus/can_rx_statemachine.h"
    #include "openlcb/openlcb_gridconnect.h"
    #include "lwip/sockets.h"

    static const char *TAG = "CAN_DRIVER";
    static bool is_driver_connected = false;
    static TaskHandle_t can_rx_task_handle = NULL;
    static int tx_queue_len = 0;

    extern int active_tcp_socket;

    // Background task to receive incoming physical CAN frames
/**
 * @brief  Background task to receive incoming physical CAN frames.
 *
 * Continuously polls the TWAI hardware driver for incoming CAN messages.
 * When a valid message is received, it is forwarded to the local OpenLCB stack
 * and bridged to the TCP client if one is connected.
 *
 * @param[in]  pvParameters   Task parameters provided by FreeRTOS during creation.
 */
static void can_rx_task(void *pvParameters) {
        can_msg_t can_msg;
        can_msg.state.allocated = 1;

        while (1) {
            twai_message_t rx_msg;
            esp_err_t err = twai_receive(&rx_msg, pdMS_TO_TICKS(100));

            if (err == ESP_OK) {
                if (rx_msg.extd) {
                    can_msg.identifier = rx_msg.identifier;
                    can_msg.payload_count = rx_msg.data_length_code;
                    for (int i = 0; i < rx_msg.data_length_code; i++) {
                        can_msg.payload[i] = rx_msg.data[i];
                    }

                    // 1. Forward to the local OpenLCB stack
                    lcc_drivers_lock_shared_resources();
                    CanRxStatemachine_incoming_can_driver_callback(&can_msg);
                    lcc_drivers_unlock_shared_resources();

                    // 2. Bridge to the TCP client if one is connected
                    if (active_tcp_socket >= 0) {
                        gridconnect_buffer_t gc_string;
                        OpenLcbGridConnect_from_can_msg(&gc_string, &can_msg);
                        size_t str_len = strlen((char *)(&gc_string));
                        send(active_tcp_socket, gc_string, str_len, 0);
                    }
                }
            } else if (err == ESP_ERR_TIMEOUT) {
                // Timeout, loop again
            } else {
                ESP_LOGE(TAG, "CAN receive error: 0x%x", err);
                vTaskDelay(pdMS_TO_TICKS(10));
            }
        }
    }

    void can_driver_initialize(void) {
        // TODO: Update these to your actual CAN TX and RX pins
        twai_general_config_t g_config = TWAI_GENERAL_CONFIG_DEFAULT(GPIO_NUM_4, GPIO_NUM_5, TWAI_MODE_NORMAL);
        twai_timing_config_t t_config = TWAI_TIMING_CONFIG_125KBITS(); // LCC standard is 125 kbps
        twai_filter_config_t f_config = TWAI_FILTER_CONFIG_ACCEPT_ALL();

        tx_queue_len = g_config.tx_queue_len;

        if (twai_driver_install(&g_config, &t_config, &f_config) == ESP_OK) {
            if (twai_start() == ESP_OK) {
                is_driver_connected = true;
                ESP_LOGI(TAG, "TWAI driver started successfully.");

                xTaskCreatePinnedToCore(
                    can_rx_task,
                    "can_rx_task",
                    3072,
                    NULL,
                    10,
                    &can_rx_task_handle,
                    0 // Pin to Core 0 (leave Core 1 for LVGL UI)
                );
            } else {
                ESP_LOGE(TAG, "Failed to start TWAI driver.");
            }
        } else {
            ESP_LOGE(TAG, "Failed to install TWAI driver.");
        }
    }

    // Transmit to the physical CAN controller ONLY (no TCP forwarding)
    bool can_driver_transmit_physical(can_msg_t *can_msg) {
        if (!is_driver_connected) {
            return false;
        }

        twai_message_t tx_msg = {
            .identifier = can_msg->identifier,
            .extd = 1,
            .data_length_code = can_msg->payload_count,
            .rtr = 0,
            .ss = 0,
            .self = 0,
            .dlc_non_comp = 0
        };

        for (int i = 0; i < can_msg->payload_count; i++) {
            tx_msg.data[i] = can_msg->payload[i];
        }

        esp_err_t err = twai_transmit(&tx_msg, 0);
        return (err == ESP_OK);
    }

    // Node-level transmit: called by local node engine to send outbound data
bool can_driver_transmit(can_msg_t *can_msg) {
        // 1. Transmit to physical CAN bus (only if initialized)
        bool can_ok = false;
        if (is_driver_connected) {
            can_ok = can_driver_transmit_physical(can_msg);
        }

        // 2. Mirror outbound transmission to TCP client (if connected)
        bool tcp_ok = false;
        if (active_tcp_socket >= 0) {
            gridconnect_buffer_t gc_string;
            OpenLcbGridConnect_from_can_msg(&gc_string, can_msg);
            size_t str_len = strlen((char *)(&gc_string));
            tcp_ok = (send(active_tcp_socket, gc_string, str_len, 0) >= 0);
        }

        // Return success if at least one active channel succeeded
        return can_ok || tcp_ok;
    }


    bool can_driver_is_clear(void) {
        if (!is_driver_connected) {
            return (active_tcp_socket >= 0);
        }

        twai_status_info_t status;
        twai_get_status_info(&status);

        // If TCP is connected, we ALWAYS consider the channel "clear" for the sake 
        // of the OpenLCB stack, so Wi-Fi traffic isn't blocked by a broken CAN bus.
        if (active_tcp_socket >= 0) {
            if (status.state == TWAI_STATE_BUS_OFF) {
                twai_initiate_recovery();
            }
            return true;
        }

        if (status.state == TWAI_STATE_BUS_OFF) {
            ESP_LOGW(TAG, "CAN Bus off. Recovering...");
            twai_initiate_recovery();
            return false;
        }

        return (status.msgs_to_tx == 0);
    }


    bool can_driver_is_connected(void) {
        return is_driver_connected;
    }
