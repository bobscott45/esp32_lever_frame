#include "unity.h"
#include "controller.h"
#include <string.h>

void setUp(void) {
    // controller_free is safely called inside init, but let's be sure
    controller_free();
}

void tearDown(void) {
    controller_free();
}

void test_controller_init_null_config(void) {
    controller_init(NULL);
    TEST_ASSERT_NULL(controller_get_tab_states(0));
    TEST_ASSERT_FALSE(controller_get_lever_state(0, 0));
}

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

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_controller_init_null_config);
    RUN_TEST(test_controller_init_valid_config);
    RUN_TEST(test_controller_out_of_bounds);
    RUN_TEST(test_controller_request_move);
    return UNITY_END();
}
