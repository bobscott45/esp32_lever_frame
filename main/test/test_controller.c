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
 * @file      test_controller.c
 * @brief     Implementation of test_controller.c
 *
 * @author    Robert Scott
 * @date      2026
 */

#include "unity.h"
#include "controller.h"
#include <string.h>
#include <stdlib.h>

/**
 * @brief  Set up test environment.
 *
 * Cleans the controller state before a test begins.
 */
void setUp(void) {
    // controller_free is safely called inside init, but let's be sure
    controller_free();
}

/**
 * @brief  Tear down test environment.
 *
 * Cleans the controller state after a test finishes.
 */
void tearDown(void) {
    controller_free();
}

/**
 * @brief  Test controller initialization with a NULL config.
 *
 * Verifies that the controller handles a NULL configuration safely without crashing.
 */
void test_controller_init_null_config(void) {
    controller_init(NULL);
    TEST_ASSERT_NULL(controller_get_tab_states(0));
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 0));
}

/**
 * @brief  Test controller initialization with a valid config.
 *
 * Verifies that the controller correctly sets up tab states and handles 
 * lever state requests.
 */
void test_controller_init_valid_config(void) {
    lever_system_config_t config;
    memset(&config, 0, sizeof(config));
    
    tab_def_t tabs[2];
    memset(tabs, 0, sizeof(tabs));
    
    // Tab 0 has 3 levers
    tabs[0].lever_count = 3;
    // Tab 1 has 5 levers
    tabs[1].lever_count = 5;
    
    config.tabs = tabs;
    config.tab_count = 2;
    
    controller_init(&config);
    
    // Check pointers are valid
    TEST_ASSERT_NOT_NULL(controller_get_tab_states(0));
    TEST_ASSERT_NOT_NULL(controller_get_tab_states(1));
    TEST_ASSERT_NULL(controller_get_tab_states(2)); // Out of bounds
    
    // Set some states
    controller_set_lever_state(0, 1, true);
    controller_set_lever_state(1, 4, true);
    
    // Verify states
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 0));
    TEST_ASSERT_TRUE(controller_get_lever_state(0, 1));
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 2));
    
    TEST_ASSERT_FALSE(controller_get_lever_state(1, 0));
    TEST_ASSERT_TRUE(controller_get_lever_state(1, 4));
}

/**
 * @brief  Test controller out of bounds access.
 *
 * Verifies that the controller gracefully rejects invalid tab or lever indices.
 */
void test_controller_out_of_bounds(void) {
    lever_system_config_t config;
    memset(&config, 0, sizeof(config));
    tab_def_t tabs[1];
    memset(tabs, 0, sizeof(tabs));
    tabs[0].lever_count = 2;
    config.tabs = tabs;
    config.tab_count = 1;
    
    controller_init(&config);
    
    // Out of bounds sets should not crash
    controller_set_lever_state(-1, 0, true);
    controller_set_lever_state(1, 0, true);
    controller_set_lever_state(0, -1, true);
    controller_set_lever_state(0, 2, true); // only 2 levers (0, 1)
    
    // Verify none were magically set
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 0));
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 1));
    
    // Out of bounds gets should return false
    TEST_ASSERT_FALSE(controller_get_lever_state(-1, 0));
    TEST_ASSERT_FALSE(controller_get_lever_state(1, 0));
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 2));
}

/**
 * @brief  Test controller request move logic.
 *
 * Verifies that the controller allows valid moves based on the interlocking config
 * and correctly rejects out-of-bounds requests.
 */
void test_controller_request_move(void) {
    // We must initialize the actual config manager because controller_request_lever_move 
    // retrieves the config from it.
    config_manager_init();
    const lever_system_config_t *config = config_manager_get_current();
    
    controller_init(config);
    
    // The default config has no interlocking rules, so everything should be accepted.
    // Try to move Tab 0, Lever 0 to THROWN (true)
    bool accepted = controller_request_lever_move(0, 0, true);
    TEST_ASSERT_TRUE(accepted);
    TEST_ASSERT_TRUE(controller_get_lever_state(0, 0));
    
    // Try to move it back to NORMAL (false)
    accepted = controller_request_lever_move(0, 0, false);
    TEST_ASSERT_TRUE(accepted);
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 0));
    
    // Out of bounds should be rejected
    TEST_ASSERT_FALSE(controller_request_lever_move(-1, 0, true));
    TEST_ASSERT_FALSE(controller_request_lever_move(0, -1, true));
    TEST_ASSERT_FALSE(controller_request_lever_move(99, 0, true));
    TEST_ASSERT_FALSE(controller_request_lever_move(0, 99, true));
    
    config_manager_deinit();
}

/**
 * @brief  Test controller state serialization.
 *
 * Verifies that the entire controller state can be serialized to a blob 
 * and subsequently restored perfectly.
 */
void test_controller_state_serialization(void) {
    lever_system_config_t config;
    memset(&config, 0, sizeof(config));
    tab_def_t tabs[2];
    memset(tabs, 0, sizeof(tabs));
    tabs[0].lever_count = 3;
    tabs[1].lever_count = 5;
    config.tabs = tabs;
    config.tab_count = 2;
    
    controller_init(&config);
    
    // Set some state
    controller_set_lever_state(0, 1, true); // Tab 0, Lever 1 = THROWN
    controller_set_lever_lock(0, 2, true);  // Tab 0, Lever 2 = LOCKED
    controller_set_lever_state(1, 4, true); // Tab 1, Lever 4 = THROWN
    controller_set_active_tab(1);
    
    // Serialize
    size_t blob_size = controller_get_serialized_size();
    uint8_t *blob = malloc(blob_size);
    controller_get_serialized_state(blob, blob_size);
    
    TEST_ASSERT_NOT_NULL(blob);
    TEST_ASSERT_GREATER_THAN(0, blob_size);
    
    // Wipe current state
    controller_free();
    controller_init(&config); // Reset to 0
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 1));
    TEST_ASSERT_EQUAL_INT(0, controller_get_active_tab());
    
    // Restore from blob
    bool success = controller_apply_serialized_state(blob, blob_size);
    TEST_ASSERT_TRUE(success);
    
    // Verify state was perfectly restored
    TEST_ASSERT_TRUE(controller_get_lever_state(0, 1));
    TEST_ASSERT_TRUE(controller_get_lever_lock(0, 2));
    TEST_ASSERT_TRUE(controller_get_lever_state(1, 4));
    TEST_ASSERT_FALSE(controller_get_lever_state(1, 0)); // Was untouched
    TEST_ASSERT_EQUAL_INT(1, controller_get_active_tab());
    
    free(blob);
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
    RUN_TEST(test_controller_init_null_config);
    RUN_TEST(test_controller_init_valid_config);
    RUN_TEST(test_controller_out_of_bounds);
    RUN_TEST(test_controller_request_move);
    RUN_TEST(test_controller_state_serialization);
    return UNITY_END();
}
