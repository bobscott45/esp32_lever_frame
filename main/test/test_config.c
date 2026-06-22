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
 * @file      test_config.c
 * @brief     Implementation of test_config.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "unity.h"
#include "config_manager.h"
#include "cJSON.h"
#include <string.h>
#include <stdlib.h>

// Forward declarations from mock_nvs.c
void mock_nvs_inject_blob(const char* key, const void* data, size_t len);
void mock_nvs_clear(void);

/**
 * @brief  Set up test environment.
 *
 * Prepares the mock NVS environment before a test runs.
 */
void setUp(void) {
    mock_nvs_clear();
}

/**
 * @brief  Tear down test environment.
 *
 * Cleans up the configuration manager and mock NVS after a test finishes.
 */
void tearDown(void) {
    config_manager_deinit();
    mock_nvs_clear();
}

/**
 * @brief  Test configuration default fallback.
 *
 * Tests that the configuration manager falls back to the default configuration 
 * when the NVS is empty.
 */
void test_config_default_fallback(void)
{
    // Ensure NVS is empty
    mock_nvs_clear();
    
    // Init should fall back to default
    TEST_ASSERT_EQUAL(0, config_manager_init());
    
    const lever_system_config_t *cfg = config_manager_get_current();
    TEST_ASSERT_NOT_NULL(cfg);
    TEST_ASSERT_EQUAL(3, cfg->tab_count);
    TEST_ASSERT_EQUAL_STRING("North Junction Frame 1", cfg->tabs[0].name);
    TEST_ASSERT_EQUAL(8, cfg->tabs[0].lever_count);
}

/**
 * @brief  Test configuration save and load.
 *
 * Tests that a configuration can be correctly serialized to JSON, saved to NVS, 
 * and subsequently loaded and parsed back.
 */
void test_config_save_and_load(void)
{
    // 1. Initialise with defaults
    TEST_ASSERT_EQUAL(0, config_manager_init());
    
    // 2. Generate JSON string from default config
    char *json1 = config_manager_get_json_str();
    TEST_ASSERT_NOT_NULL(json1);
    
    // 3. Save this JSON back (this goes to mock NVS)
    TEST_ASSERT_EQUAL(0, config_manager_save_json(json1));
    
    // 4. De-init the config manager to wipe memory
    config_manager_deinit();
    
    // 5. Re-init. It should now read the JSON from our mock NVS
    TEST_ASSERT_EQUAL(0, config_manager_init());
    
    // 6. Generate JSON again and compare. They should match identically.
    char *json2 = config_manager_get_json_str();
    TEST_ASSERT_NOT_NULL(json2);
    
    // Compare structural equivalence or string equivalence?
    // cJSON print isn't guaranteed to be identical string, but for our code it usually is.
    // Let's parse both and compare them.
    cJSON *c1 = cJSON_Parse(json1);
    cJSON *c2 = cJSON_Parse(json2);
    
    TEST_ASSERT_NOT_NULL(c1);
    TEST_ASSERT_NOT_NULL(c2);
    TEST_ASSERT_TRUE(cJSON_Compare(c1, c2, true));
    
    cJSON_Delete(c1);
    cJSON_Delete(c2);
    free(json1);
    free(json2);
}

/**
 * @brief  Main entry point for tests.
 *
 * Runs the test suite using Unity.
 * 
 * @return 
 *   - ESP_OK on success
 *   - ESP_ERR_INVALID_ARG if parameters are invalid
 *   - ESP_FAIL on general failure
 */
int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_config_default_fallback);
    RUN_TEST(test_config_save_and_load);
    return UNITY_END();
}
