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



EventGroupHandle_t wifi_event_group;

int active_tcp_socket = -1;



static void tcp_server_task(void *pvParameters) {
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

static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(TAG, "WiFi Disconnected! Reason code: %d", event->reason);

        // Auto-retry connection
        esp_wifi_connect();
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
    }
}





static bool init_wifi() {
    ESP_LOGI(TAG, "Initializing WiFi");

    ESP_ERROR_CHECK(esp_netif_init());
    esp_netif_create_default_wifi_sta();
    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));
    //"ZENBQ16"
    //"Juniper#1945",
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = "ZENBQ16",
            .password = "Juniper#1945",
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);
    ESP_ERROR_CHECK(esp_wifi_start());
    return true;
}




void tcp_driver_initialize(void) {
    ESP_LOGI(TAG, "Initializing TCP/IP driver");
    
    esp_err_t err = esp_event_loop_create_default();
    if (err != ESP_OK && err != ESP_ERR_INVALID_STATE) {
        ESP_ERROR_CHECK(err);
    }
    wifi_event_group = xEventGroupCreate();
    if (!wifi_event_group) {
        ESP_LOGE(TAG, "Failed to create WiFi event group");
        return;
    }
    
    // Connect to the Wi-Fi network
    init_wifi(); 
    ESP_LOGI(TAG, "Waiting for WiFi connection...");
    xEventGroupWaitBits(wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "WiFi Connected! Initializing OpenLCB Hub...");
    // Start the raw TCP server task instead of start_http_server()
    xTaskCreate(tcp_server_task, "openlcb_tcp", 4096, NULL, 5, NULL);
}
