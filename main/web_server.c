#include "web_server.h"
#include <string.h>
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_http_server.h"
#include "config_manager.h"
#include "esp_system.h"

static const char *TAG = "WebServer";
static httpd_handle_t server = NULL;
static esp_netif_t *ap_netif = NULL;

// ASM symbols for embedded index.html.gz
extern const uint8_t index_html_start[] asm("_binary_index_html_gz_start");
extern const uint8_t index_html_end[] asm("_binary_index_html_gz_end");

extern void ui_show_remote_config_overlay(void);

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

static esp_err_t api_ping_get_handler(httpd_req_t *req) {
    ui_show_remote_config_overlay();
    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    httpd_resp_sendstr(req, "{\"status\":\"ok\"}");
    return ESP_OK;
}

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

static httpd_handle_t start_webserver(void) {
    httpd_handle_t server_handle = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();
    config.stack_size = 10240; // 10KB stack size for JSON processing
    config.max_uri_handlers = 6;

    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server_handle, &config) == ESP_OK) {
        httpd_register_uri_handler(server_handle, &root_uri);
        httpd_register_uri_handler(server_handle, &index_uri);
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

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    const lever_system_config_t *curr_config = config_manager_get_current();
    const char *ap_password = (curr_config && curr_config->wifi_password && strlen(curr_config->wifi_password) >= 8) ? curr_config->wifi_password : "signalman";

    wifi_config_t wifi_config = {
        .ap = {
            .ssid = "Lever-Frame-Config",
            .ssid_len = strlen("Lever-Frame-Config"),
            .channel = 6, // Channel 6 is often less congested and faster to be listed
            .max_connection = 4,
            .authmode = WIFI_AUTH_WPA_WPA2_PSK,
            .pmf_cfg = {
                .required = false,
            },
        },
    };
    strncpy((char *)wifi_config.ap.password, ap_password, sizeof(wifi_config.ap.password) - 1);

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    ESP_LOGI(TAG, "Wi-Fi softAP 'Lever-Frame-Config' started successfully. Password: %s", ap_password);

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
