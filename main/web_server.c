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
 * @file      web_server.c
 * @brief     Implementation of web_server.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "web_server.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "config_manager.h"
#include "esp_system.h"
#include "freertos/timers.h"
#include "openlcb_user_config.h"

static TimerHandle_t wifi_retry_timer = NULL;

/**
 * @brief  Callback for Wi-Fi retry timer.
 *
 * This function is triggered when the Wi-Fi retry timer expires, 
 * attempting to reconnect the Wi-Fi interface.
 *
 * @param[in]  xTimer   Handle of the expired timer.
 */
static void wifi_retry_timer_cb(TimerHandle_t xTimer) {
    wifi_config_t conf;
    esp_wifi_get_config(WIFI_IF_STA, &conf);
    if (strlen((char *)conf.sta.ssid) > 0) {
        ESP_LOGI("WebServer", "Retrying WiFi connection...");
        esp_wifi_connect();
    }
}

static const char *TAG = "WebServer";
static httpd_handle_t server = NULL;
static esp_netif_t *ap_netif = NULL;
static esp_netif_t *sta_netif = NULL;
EventGroupHandle_t wifi_event_group;
#define WIFI_CONNECTED_BIT BIT0

/**
 * @brief  Handle Wi-Fi and IP events.
 *
 * This function processes system events related to Wi-Fi connection state 
 * and IP address assignment, including initiating connection retries.
 *
 * @param[in]  arg          User argument.
 * @param[in]  event_base   Event base.
 * @param[in]  event_id     Event ID.
 * @param[in]  event_data   Event data.
 */
static void wifi_event_handler(void* arg, esp_event_base_t event_base,
                          int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        wifi_config_t conf;
        esp_wifi_get_config(WIFI_IF_STA, &conf);
        if (strlen((char *)conf.sta.ssid) > 0) {
            esp_wifi_connect();
        } else {
            ESP_LOGI(TAG, "No STA SSID configured, skipping connect");
        }
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        wifi_event_sta_disconnected_t* event = (wifi_event_sta_disconnected_t*) event_data;
        ESP_LOGE(TAG, "WiFi Disconnected! Reason code: %d. Retrying in 5 seconds...", event->reason);
        // Auto-retry connection with backoff to prevent killing the AP
        if (wifi_retry_timer == NULL) {
            wifi_retry_timer = xTimerCreate("wifi_retry", pdMS_TO_TICKS(5000), pdFALSE, NULL, wifi_retry_timer_cb);
        }
        if (wifi_retry_timer != NULL) {
            xTimerStart(wifi_retry_timer, 0);
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        if (wifi_event_group) {
            xEventGroupSetBits(wifi_event_group, WIFI_CONNECTED_BIT);
        }
    }
}

void web_server_get_sta_ip(char *ip_buf, size_t buf_len) {
    if (ip_buf && buf_len > 0) {
        ip_buf[0] = '\0';
        if (sta_netif) {
            esp_netif_ip_info_t ip_info;
            if (esp_netif_get_ip_info(sta_netif, &ip_info) == ESP_OK) {
                if (ip_info.ip.addr != 0) {
                    snprintf(ip_buf, buf_len, IPSTR, IP2STR(&ip_info.ip));
                } else {
                    strncpy(ip_buf, "Connecting...", buf_len);
                }
            }
        }
    }
}

// ASM symbols for embedded index.html.gz
extern const uint8_t index_html_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_gz_end");

#include "ui_overlays.h"

/**
 * @brief  Handler for the root /index.html path.
 *
 * Serves the embedded gzip-compressed index.html file and shows 
 * the remote configuration overlay on the UI.
 *
 * @param[in]  req   HTTP request pointer.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static esp_err_t root_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "Serving /index.html (gzipped)");
    ui_show_remote_config_overlay();
    httpd_resp_set_type(req, "text/html");
    httpd_resp_set_hdr(req, "Content-Encoding", "gzip");
    httpd_resp_set_hdr(req, "Cache-Control", "no-store, no-cache, must-revalidate, max-age=0");
    httpd_resp_set_hdr(req, "Pragma", "no-cache");
    httpd_resp_send(req, (const char *)index_html_start, index_html_end - index_html_start);
    return ESP_OK;
}

/**
 * @brief  Handler for getting the configuration via API.
 *
 * Returns the current configuration as a JSON string and updates 
 * the UI overlay.
 *
 * @param[in]  req   HTTP request pointer.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static esp_err_t api_config_get_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "GET /api/config");
    ui_show_remote_config_overlay();
    char *json_str = config_manager_get_json_str();
    if (!json_str) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Failed to get config string");
        return ESP_FAIL;
    }

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, json_str);
    free(json_str);
    return ESP_OK;
}

/**
 * @brief  Handler for the ping API.
 *
 * Responds with a simple status OK JSON string to indicate the 
 * web server is alive.
 *
 * @param[in]  req   HTTP request pointer.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static esp_err_t api_ping_get_handler(httpd_req_t *req) {
    ui_show_remote_config_overlay();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

/**
 * @brief  Handler for posting configuration updates via API.
 *
 * Reads the incoming JSON payload, saves it to the configuration manager, 
 * and then schedules a device reboot on success.
 *
 * @param[in]  req   HTTP request pointer.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static esp_err_t api_config_post_handler(httpd_req_t *req) {
    ESP_LOGI(TAG, "POST /api/config");
    ui_show_remote_config_overlay();
    int total_len = req->content_len;
    int cur_len = 0;

    if (total_len <= 0) {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Empty request body");
        return ESP_FAIL;
    }

    char *buf = malloc(total_len + 1);
    if (!buf) {
        httpd_resp_send_err(req, HTTPD_500_INTERNAL_SERVER_ERROR, "Memory allocation failed");
        return ESP_FAIL;
    }

    int retries = 5;
    while (cur_len < total_len) {
        int received = httpd_req_recv(req, buf + cur_len, total_len - cur_len);
        if (received <= 0) {
            if (received == HTTPD_SOCK_ERR_TIMEOUT) {
                if (--retries > 0) {
                    vTaskDelay(pdMS_TO_TICKS(10)); // Yield and retry
                    continue;
                }
                httpd_resp_send_err(req, HTTPD_408_REQ_TIMEOUT, "Request Timeout");
            } else {
                httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Error receiving packet");
            }
            free(buf);
            return ESP_FAIL;
        }
        retries = 5; // Reset retries on successful read
        cur_len += received;
    }
    buf[total_len] = '\0';

    esp_err_t err = config_manager_save_json(buf);
    free(buf);

    if (err == ESP_OK) {
        httpd_resp_set_type(req, "application/json");
        httpd_resp_sendstr(req, "{\"status\":\"success\",\"message\":\"Rebooting device...\"}");
        
        // Let the HTTP response flush, then restart
        vTaskDelay(pdMS_TO_TICKS(500));
        esp_restart();
        return ESP_OK;
    } else {
        httpd_resp_send_err(req, HTTPD_400_BAD_REQUEST, "Invalid JSON structure");
        return ESP_FAIL;
    }
}

static const httpd_uri_t root_uri = {
    .uri       = "/",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t index_uri = {
    .uri       = "/index.html",
    .method    = HTTP_GET,
    .handler   = root_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t api_config_get_uri = {
    .uri       = "/api/config",
    .method    = HTTP_GET,
    .handler   = api_config_get_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t api_config_post_uri = {
    .uri       = "/api/config",
    .method    = HTTP_POST,
    .handler   = api_config_post_handler,
    .user_ctx  = NULL
};

static const httpd_uri_t api_ping_get_uri = {
    .uri       = "/api/ping",
    .method    = HTTP_GET,
    .handler   = api_ping_get_handler,
    .user_ctx  = NULL
};

/**
 * @brief  Handler for getting the status via API.
 *
 * Returns a JSON string containing AP IP, STA IP, Node ID, 
 * and version information.
 *
 * @param[in]  req   HTTP request pointer.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
static esp_err_t api_status_handler(httpd_req_t *req) {
    httpd_resp_set_type(req, "application/json");
    
    char sta_ip[32] = "Not Connected";
    web_server_get_sta_ip(sta_ip, sizeof(sta_ip));

    uint64_t nid = NODE_ID;
    char node_id_str[32];
    snprintf(node_id_str, sizeof(node_id_str), "%02X.%02X.%02X.%02X.%02X.%02X",
             (uint8_t)(nid >> 40), (uint8_t)(nid >> 32), (uint8_t)(nid >> 24),
             (uint8_t)(nid >> 16), (uint8_t)(nid >> 8), (uint8_t)(nid));

    char response[512];
    snprintf(response, sizeof(response), "{\"ap_ip\":\"192.168.4.1\",\"sta_ip\":\"%s\",\"tcp_port\":12021,\"node_id\":\"%s\",\"version\":\"%s\"}", sta_ip, node_id_str, PROJECT_VER);
    httpd_resp_sendstr(req, response);
    return ESP_OK;
}

static const httpd_uri_t uri_api_status = {
    .uri       = "/api/status",
    .method    = HTTP_GET,
    .handler   = api_status_handler,
    .user_ctx  = NULL
};

/**
 * @brief  Start the internal HTTP web server.
 *
 * Configures the HTTP server parameters, registers the URI handlers, 
 * and starts the server.
 * 
 * @return httpd_handle_t Handle to the running server or NULL on failure.
 */
static httpd_handle_t start_webserver(void) {
    httpd_handle_t server_handle = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 10240; // 10KB stack size for JSON processing
    config.max_uri_handlers = 8;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server_handle, &config) == ESP_OK) {
        httpd_register_uri_handler(server_handle, &root_uri);
        httpd_register_uri_handler(server_handle, &index_uri);
        httpd_register_uri_handler(server_handle, &uri_api_status);
        httpd_register_uri_handler(server_handle, &api_config_get_uri);
        httpd_register_uri_handler(server_handle, &api_config_post_uri);
        httpd_register_uri_handler(server_handle, &api_ping_get_uri);
        return server_handle;
    }

    ESP_LOGE(TAG, "Error starting HTTP server!");
    return NULL;
}

esp_err_t web_server_start(void) {
    esp_err_t ret = esp_netif_init();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to init netif: %s", esp_err_to_name(ret));
        return ret;
    }

    ret = esp_event_loop_create_default();
    if (ret != ESP_OK && ret != ESP_ERR_INVALID_STATE) {
        ESP_LOGE(TAG, "Failed to create default event loop: %s", esp_err_to_name(ret));
        return ret;
    }

    ap_netif = esp_netif_create_default_wifi_ap();
    if (!ap_netif) {
        ESP_LOGE(TAG, "Failed to create AP netif");
        return ESP_FAIL;
    }
    
    sta_netif = esp_netif_create_default_wifi_sta();
    if (!sta_netif) {
        ESP_LOGE(TAG, "Failed to create STA netif");
        return ESP_FAIL;
    }

    wifi_event_group = xEventGroupCreate();
    esp_event_handler_instance_register(WIFI_EVENT, ESP_EVENT_ANY_ID, wifi_event_handler, NULL, NULL);
    esp_event_handler_instance_register(IP_EVENT, IP_EVENT_STA_GOT_IP, wifi_event_handler, NULL, NULL);

#ifndef CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM
#define CONFIG_ESP_WIFI_ESPNOW_MAX_ENCRYPT_NUM 0
#define CONFIG_ESP_WIFI_STATIC_RX_BUFFER_NUM 10
#define CONFIG_ESP_WIFI_DYNAMIC_RX_BUFFER_NUM 32
#define CONFIG_ESP_WIFI_TX_BUFFER_TYPE 1
#define CONFIG_ESP_WIFI_DYNAMIC_RX_MGMT_BUF 1
#define CONFIG_ESP_WIFI_TX_BA_WIN 6
#define CONFIG_ESP_WIFI_RX_BA_WIN 6
#define CONFIG_ESP_WIFI_MGMT_SBUF_NUM 32
#define CONFIG_ESP_WIFI_RX_MGMT_BUF_NUM 32
#define CONFIG_ESP_WIFI_CACHE_TX_BUFFER_NUM 32
#define CONFIG_ESP_WIFI_AMSDU_TX_ENABLED 0
#endif

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    const lever_system_config_t *curr_config = config_manager_get_current();
    const char *ap_password = (curr_config && curr_config->wifi_password && strlen(curr_config->wifi_password) >= 8) ? curr_config->wifi_password : "signalman";
    const char *sta_ssid = (curr_config && curr_config->wifi_ssid && strlen(curr_config->wifi_ssid) > 0) ? curr_config->wifi_ssid : CONFIG_WIFI_DEFAULT_SSID;
    const char *sta_password = (curr_config && curr_config->wifi_station_password && strlen(curr_config->wifi_station_password) > 0) ? curr_config->wifi_station_password : CONFIG_WIFI_DEFAULT_PASSWORD;

    wifi_config_t wifi_config_ap = {
        .ap = {
            .ssid = "Lever-Frame-Config",
            .ssid_len = strlen("Lever-Frame-Config"),
            .channel = 1,
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    strncpy((char *)wifi_config_ap.ap.password, ap_password, sizeof(wifi_config_ap.ap.password) - 1);

    wifi_config_t wifi_config_sta = {
        .sta = {
            .threshold.authmode = WIFI_AUTH_OPEN,
            .sae_pwe_h2e = WPA3_SAE_PWE_BOTH,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    strncpy((char *)wifi_config_sta.sta.ssid, sta_ssid, sizeof(wifi_config_sta.sta.ssid) - 1);
    strncpy((char *)wifi_config_sta.sta.password, sta_password, sizeof(wifi_config_sta.sta.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_APSTA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config_ap));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config_sta));
    ESP_ERROR_CHECK(esp_wifi_start());
    
    // Limit TX power to 8.5 dBm (approx 200mA) to prevent brownouts on weak 3.3V LDOs
    esp_wifi_set_max_tx_power(34);

    ESP_LOGI(TAG, "Wi-Fi softAP 'Lever-Frame-Config' started successfully. Password: %s", ap_password);
    ESP_LOGI(TAG, "Wi-Fi STA connecting to '%s'...", sta_ssid);

    server = start_webserver();
    if (!server) {
        return ESP_FAIL;
    }

    return ESP_OK;
}

void web_server_stop(void) {
    if (server) {
        httpd_stop(server);
        server = NULL;
    }
    esp_wifi_stop();
    esp_wifi_deinit();
    // note: netif and event loop are kept as they are global states
}
