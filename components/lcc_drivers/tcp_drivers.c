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
 * @file tcp_drivers.c
 * @brief Hardware driver implementations for TCP/IP Drivers.
 *
 * @author Robert Scott
 * @date 2026
 */

#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_wifi.h"
#include "esp_wifi_default.h"
#include "nvs_flash.h"
#include "drivers/canbus/can_rx_statemachine.h"
#include "can_drivers.h"

#include "lwip/sockets.h"
#include <arpa/inet.h>
#include "openlcb/openlcb_gridconnect.h"
#include "openlcb/openlcb_node.h"
#include "config_manager.h"


#define WIFI_CONNECTED_BIT BIT0
static const char *TAG = "LCC_TCP_DRIVER";
extern const openlcb_node_t *node;



extern EventGroupHandle_t wifi_event_group;

int active_tcp_socket = -1;



/**
 * @brief  Background task for the TCP server.
 *
 * Waits for the WiFi connection to be established, then binds to the standard
 * OpenLCB GridConnect port (12021). Listens for incoming connections from
 * clients (e.g., JMRI) and bridges GridConnect strings to physical CAN messages
 * and the local OpenLCB node.
 *
 * @param[in]  pvParameters   Task parameters provided by FreeRTOS during creation.
 */
static void tcp_task(void *pvParameters) {
    ESP_LOGI("TCP_Task", "Waiting for WiFi connection before starting GridConnect...");
    if (wifi_event_group) {
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    } else {
        ESP_LOGW("TCP_Task", "wifi_event_group is NULL. TCP task may not have a connection.");
    }

    const lever_system_config_t *curr_config = config_manager_get_current();
    bool is_client = (curr_config->jmri_ip_address && strlen(curr_config->jmri_ip_address) > 0);

    if (is_client) {
        ESP_LOGI("TCP_Task", "WiFi Connected! Starting OpenLCB Client connecting to JMRI Hub at %s:12021", curr_config->jmri_ip_address);
    } else {
        ESP_LOGI("TCP_Task", "WiFi Connected! Starting OpenLCB Hub (Server) on port 12021");
    }

    char rx_buffer[256];

    while (1) {
        int sock = -1;

        if (is_client) {
            struct sockaddr_in dest_addr = {
                .sin_family = AF_INET,
                .sin_port = htons(12021)
            };
            dest_addr.sin_addr.s_addr = inet_addr(curr_config->jmri_ip_address);

            sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            if (sock < 0) {
                ESP_LOGE("TCP_Task", "Unable to create socket");
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }

            int err = connect(sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            if (err != 0) {
                ESP_LOGE("TCP_Task", "Socket unable to connect to %s:12021, retrying in 5s...", curr_config->jmri_ip_address);
                close(sock);
                vTaskDelay(pdMS_TO_TICKS(5000));
                continue;
            }
            ESP_LOGI("TCP_Task", "Connected to JMRI Hub!");
        } else {
            struct sockaddr_in dest_addr = {
                .sin_addr.s_addr = htonl(INADDR_ANY),
                .sin_family = AF_INET,
                .sin_port = htons(12021)
            };
            int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
            int opt = 1;
            setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
            bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
            listen(listen_sock, 1);

            ESP_LOGI("TCP_Task", "Listening for OpenLCB traffic on port 12021");

            struct sockaddr_storage source_addr;
            socklen_t addr_len = sizeof(source_addr);
            sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
            close(listen_sock);

            if (sock >= 0) {
                ESP_LOGI("TCP_Task", "OpenLCB Client Connected!");
            }
        }

        if (sock >= 0) {
            active_tcp_socket = sock; 
            gridconnect_buffer_t gc_buffer;
            can_msg_t can_msg;

            while (1) {
                int len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
                if (len <= 0) {
                    ESP_LOGI("TCP_Task", "%s", is_client ? "Disconnected from JMRI Hub" : "Client disconnected");
                    break;
                }
                for (int i = 0; i < len; i++) {
                    bool valid = OpenLcbGridConnect_copy_out_gridconnect_when_done(rx_buffer[i], &gc_buffer);
                    if (valid) {
                        OpenLcbGridConnect_to_can_msg(&gc_buffer, &can_msg);
                        can_driver_transmit_physical(&can_msg);
                        CanRxStatemachine_incoming_can_driver_callback(&can_msg);
                    }
                }
            }
            
            active_tcp_socket = -1;
            close(sock);
            
            if (is_client) {
                vTaskDelay(pdMS_TO_TICKS(3000)); // Delay before reconnecting
            }
        }
    }
}






void tcp_driver_initialize(void) {
    ESP_LOGI(TAG, "Initializing TCP/IP driver");
    
    // Start the raw TCP task instead of start_http_server(). It will wait for WiFi internally.
    xTaskCreatePinnedToCore(tcp_task, "openlcb_tcp", 4096, NULL, 5, NULL, 0);
}
