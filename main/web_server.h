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
 * @file      web_server.h
 * @brief     Definitions for web_server.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef WEB_SERVER_H
#define WEB_SERVER_H

#include <esp_err.h>

/**
 * @brief  Initialize Wi-Fi in softAP mode and start the HTTP server for remote configuration.
 *
 * This function sets up the Wi-Fi AP and STA interfaces, registers event handlers,
 * and starts the HTTP server. It creates necessary event loops if they don't exist.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t web_server_start(void);

/**
 * @brief  Stop the HTTP server and Wi-Fi interface.
 *
 * Stops the HTTP server if running, and stops and deinitializes the Wi-Fi interface.
 * The netif and event loops are kept active.
 */
void web_server_stop(void);

/**
 * @brief  Get the STA IP address as a string.
 *
 * Retrieves the current IP address of the STA interface and writes it to the provided buffer.
 * If not connected, it writes "Connecting..." to the buffer.
 *
 * @param[out] ip_buf       Buffer to store the IP address string.
 * @param[in]  buf_len      Length of the buffer.
 */
void web_server_get_sta_ip(char *ip_buf, size_t buf_len);

#endif // WEB_SERVER_H
