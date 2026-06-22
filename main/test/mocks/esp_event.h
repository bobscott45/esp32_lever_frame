#ifndef MOCK_ESP_EVENT_H
#define MOCK_ESP_EVENT_H

#include <stdint.h>
#include <stddef.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef const char* esp_event_base_t;

#define ESP_EVENT_DECLARE_BASE(base) extern const char* base
#define ESP_EVENT_DEFINE_BASE(base) const char* base = #base

#define portMAX_DELAY 0xFFFFFFFF

static inline esp_err_t esp_event_post(esp_event_base_t event_base, int32_t event_id,
                                       const void *event_data, size_t event_data_size,
                                       uint32_t ticks_to_wait) {
    return ESP_OK;
}

#ifdef __cplusplus
}
#endif

#endif // MOCK_ESP_EVENT_H
