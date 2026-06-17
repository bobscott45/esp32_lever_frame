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
#include "openlcb/openlcb_gridconnect.h"
#include "openlcb/openlcb_node.h"


#define WIFI_CONNECTED_BIT BIT0
static const char *TAG = "LCC_TCP_DRIVER";
extern const openlcb_node_t *node;



extern EventGroupHandle_t wifi_event_group;

int active_tcp_socket = -1;



static void tcp_server_task(void *pvParameters) {
    ESP_LOGI("TCP_Server", "Waiting for WiFi connection before starting GridConnect server...");
    if (wifi_event_group) {
        xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    } else {
        ESP_LOGW("TCP_Server", "wifi_event_group is NULL. TCP server may not have a connection.");
    }
    ESP_LOGI("TCP_Server", "WiFi Connected! Starting OpenLCB Hub...");

    char rx_buffer[256];
    struct sockaddr_in dest_addr = {
        .sin_addr.s_addr = htonl(INADDR_ANY),
        .sin_family = AF_INET,
        .sin_port = htons(12021) // Standard OpenLCB/GridConnect Port
    };

    while (1) {
        int listen_sock = socket(AF_INET, SOCK_STREAM, IPPROTO_IP);
        
        int opt = 1;
        setsockopt(listen_sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
        
        bind(listen_sock, (struct sockaddr *)&dest_addr, sizeof(dest_addr));
        listen(listen_sock, 1);

        ESP_LOGI("TCP_Server", "Listening for OpenLCB traffic on port 12021");

        struct sockaddr_storage source_addr;
        socklen_t addr_len = sizeof(source_addr);
        
        // Block until an OpenLCB client (like JMRI) connects
        int sock = accept(listen_sock, (struct sockaddr *)&source_addr, &addr_len);
        if (sock >= 0) {
            ESP_LOGI("TCP_Server", "OpenLCB Client Connected!");
            // Store the socket so TcpDriver_transmit can use it
            active_tcp_socket = sock; 
            gridconnect_buffer_t gc_buffer;
            can_msg_t can_msg;
            // Receive loop
            while (1) {
                int len = recv(sock, rx_buffer, sizeof(rx_buffer), 0);
                if (len <= 0) {
                    ESP_LOGI("TCP_Server", "Client disconnected");
                    break; // Break out to wait for a new connection
                }
                // Process the TCP stream byte-by-byte
                for (int i = 0; i < len; i++) {
                    // Feed one character. Returns true the moment it hits ';'
                    bool valid = OpenLcbGridConnect_copy_out_gridconnect_when_done(rx_buffer[i], &gc_buffer);

                    if (valid) {
                        // A complete message was JUST formed. Process it immediately!
                        // ESP_LOGI("TCP_RX", "Parsed GC String: %s", (char*)&gc_buffer);

                        OpenLcbGridConnect_to_can_msg(&gc_buffer, &can_msg);
                        // ESP_LOGI("TCP_RX", "Converted to CAN ID: %lu, Len: %lu", can_msg.identifier, can_msg.payload_count);

                        // Push to OpenLCB engine
                        //Forward to CAN bus
                        can_driver_transmit_physical(&can_msg);
                        //Push to local OpenLCB engine
                        CanRxStatemachine_incoming_can_driver_callback(&can_msg);
                    }
                }
            }
            
            // Clean up when the client drops
            active_tcp_socket = -1;
            close(sock);
        }
        close(listen_sock);
    }
}






void tcp_driver_initialize(void) {
    ESP_LOGI(TAG, "Initializing TCP/IP driver");
    
    // Start the raw TCP server task instead of start_http_server(). It will wait for WiFi internally.
    xTaskCreate(tcp_server_task, "openlcb_tcp", 4096, NULL, 5, NULL);
}
