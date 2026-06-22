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
 * @file      nvs.h
 * @brief     Definitions for nvs.h
 *
 * @author    Robert Scott
 * @date      2026
 */

#ifndef MOCK_NVS_H
#define MOCK_NVS_H

#include "esp_err.h"
#include <stddef.h>

typedef uint32_t nvs_handle_t;

typedef enum {
    NVS_READONLY,
    NVS_READWRITE
} nvs_open_mode_t;

/**
 * @brief  Open NVS namespace.
 *
 * Mocks the ESP-IDF NVS open function.
 *
 * @param[in]  namespace_name   Name of the NVS namespace.
 * @param[in]  open_mode        Mode to open the namespace.
 * @param[out] out_handle       Pointer to store the returned NVS handle.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t nvs_open(const char* namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle);

/**
 * @brief  Get blob from NVS.
 *
 * Mocks reading a binary blob from NVS.
 *
 * @param[in]  handle      NVS handle.
 * @param[in]  key         Key of the blob.
 * @param[out] out_value   Pointer to store the read data.
 * @param[out] length      Pointer to store/provide data length.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length);

/**
 * @brief  Set blob in NVS.
 *
 * Mocks writing a binary blob to NVS.
 *
 * @param[in]  handle      NVS handle.
 * @param[in]  key         Key of the blob.
 * @param[in]  value       Pointer to the data.
 * @param[in]  length      Length of the data.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length);

/**
 * @brief  Commit NVS changes.
 *
 * Mocks committing changes to NVS storage.
 *
 * @param[in]  handle   NVS handle.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
esp_err_t nvs_commit(nvs_handle_t handle);

/**
 * @brief  Close NVS handle.
 *
 * Mocks closing an NVS handle.
 *
 * @param[in]  handle   NVS handle to close.
 */
void nvs_close(nvs_handle_t handle);

#endif
