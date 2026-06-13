#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <esp_err.h>

/**
 * @brief Initialize Wi-Fi in softAP mode and start the HTTP server
 *        for remote configuration.
 * @return ESP_OK on success
 */
esp_err_t web_server_start(void);

/**
 * @brief Stop the HTTP server and Wi-Fi interface.
 */
void web_server_stop(void);

#endif // WEB_SERVER_H
