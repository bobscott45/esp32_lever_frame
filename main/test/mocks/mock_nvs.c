#include "nvs.h"
#include "nvs_flash.h"
#include <string.h>
#include <stdlib.h>

static void *mock_nvs_blob_data = NULL;
static size_t mock_nvs_blob_len = 0;
static char mock_nvs_blob_key[64] = {0};

// Expose these for testing
void mock_nvs_inject_blob(const char* key, const void* data, size_t len) {
    if (mock_nvs_blob_data) {
        free(mock_nvs_blob_data);
    }
    strncpy(mock_nvs_blob_key, key, sizeof(mock_nvs_blob_key) - 1);
    mock_nvs_blob_len = len;
    mock_nvs_blob_data = malloc(len);
    if (data && len > 0) {
        memcpy(mock_nvs_blob_data, data, len);
    }
}

void mock_nvs_clear(void) {
    if (mock_nvs_blob_data) {
        free(mock_nvs_blob_data);
        mock_nvs_blob_data = NULL;
    }
    mock_nvs_blob_len = 0;
    mock_nvs_blob_key[0] = '\0';
}

esp_err_t nvs_flash_init(void) { return ESP_OK; }
esp_err_t nvs_flash_erase(void) { mock_nvs_clear(); return ESP_OK; }

esp_err_t nvs_open(const char* namespace_name, nvs_open_mode_t open_mode, nvs_handle_t *out_handle) {
    *out_handle = 1;
    return ESP_OK;
}

esp_err_t nvs_get_blob(nvs_handle_t handle, const char* key, void* out_value, size_t* length) {
    if (strcmp(key, mock_nvs_blob_key) != 0 || mock_nvs_blob_len == 0) {
        return ESP_ERR_NVS_NOT_FOUND;
    }
    if (out_value == NULL) {
        *length = mock_nvs_blob_len;
        return ESP_OK;
    }
    if (*length < mock_nvs_blob_len) {
        return ESP_ERR_INVALID_SIZE;
    }
    memcpy(out_value, mock_nvs_blob_data, mock_nvs_blob_len);
    *length = mock_nvs_blob_len;
    return ESP_OK;
}

esp_err_t nvs_set_blob(nvs_handle_t handle, const char* key, const void* value, size_t length) {
    mock_nvs_inject_blob(key, value, length);
    return ESP_OK;
}

esp_err_t nvs_commit(nvs_handle_t handle) {
    return ESP_OK;
}

void nvs_close(nvs_handle_t handle) {
}
