#include "unity.h"
#include "interlocking.h"
#include <string.h>

void setUp(void) {
    // set stuff up here
}

void tearDown(void) {
    // clean stuff up here
}

void test_interlocking_simple_lock(void)
{
    // Configure Lever 0 (locks Lever 1 Normal)
    lever_def_t levers[2];
    memset(levers, 0, sizeof(levers));
    
    // Lever 0 config: locks Lever 1 (index 1) in Normal state (required_state = false)
    levers[0].conditions[0].target_lever_index = 1;
    levers[0].conditions[0].required_state = false;
    levers[0].conditions[0].alt_target_lever_index = -1; // No otherwise condition
    
    levers[1].conditions[0].target_lever_index = -1;
    
    // Remaining conditions unused
    for (int i = 1; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        levers[0].conditions[i].target_lever_index = -1;
        levers[1].conditions[i].target_lever_index = -1;
    }

    tab_def_t tab;
    tab.levers = levers;
    tab.lever_count = 2;

    bool states[2] = {false, false}; // Both normal initially

    // 1. If we try to throw Lever 0, Lever 1 is Normal (false), so condition is MET.
    bool can_throw_0 = lever_evaluate_interlocking(&tab, states, 0, true);
    TEST_ASSERT_TRUE(can_throw_0);

    // 2. Simulate Lever 0 being thrown
    states[0] = true;

    // 3. Now try to throw Lever 1. 
    // Lever 0 is thrown, and its rule requires Lever 1 to be Normal.
    // So moving Lever 1 to Reversed (true) should be BLOCKED.
    bool can_throw_1 = lever_evaluate_interlocking(&tab, states, 1, true);
    TEST_ASSERT_FALSE(can_throw_1);

    // 4. Try returning Lever 0 to Normal. Should be allowed.
    bool can_return_0 = lever_evaluate_interlocking(&tab, states, 0, false);
    TEST_ASSERT_TRUE(can_return_0);
}

void test_interlocking_otherwise_logic(void)
{
    // Lever 0 locks Lever 1 Normal o/w Lever 2 is Reversed
    lever_def_t levers[3];
    memset(levers, 0, sizeof(levers));
    
    levers[0].conditions[0].target_lever_index = 1;
    levers[0].conditions[0].required_state = false;
    levers[0].conditions[0].alt_target_lever_index = 2;
    levers[0].conditions[0].alt_required_state = true;
    
    levers[1].conditions[0].target_lever_index = -1;
    levers[2].conditions[0].target_lever_index = -1;
    
    for (int i = 1; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        levers[0].conditions[i].target_lever_index = -1;
        levers[1].conditions[i].target_lever_index = -1;
        levers[2].conditions[i].target_lever_index = -1;
    }

    tab_def_t tab;
    tab.levers = levers;
    tab.lever_count = 3;

    bool states[3] = {false, true, false}; // Lever 1 Reversed, Lever 2 Normal
    
    // We try to throw Lever 0. 
    // Target (Lever 1) is NOT Normal. 
    // Alt target (Lever 2) is NOT Reversed.
    // So condition fails.
    bool can_throw_0_fail = lever_evaluate_interlocking(&tab, states, 0, true);
    TEST_ASSERT_FALSE(can_throw_0_fail);

    // Now set Lever 2 to Reversed
    states[2] = true;
    
    // Try again. Alt target is met, so it should succeed.
    bool can_throw_0_pass = lever_evaluate_interlocking(&tab, states, 0, true);
    TEST_ASSERT_TRUE(can_throw_0_pass);
}

void test_interlocking_reverse_locking(void)
{
    // Lever 1 locks Lever 0 Normal. 
    // If Lever 1 is THROWN, Lever 0 should NOT be able to throw.
    lever_def_t levers[2];
    memset(levers, 0, sizeof(levers));
    
    levers[1].conditions[0].target_lever_index = 0;
    levers[1].conditions[0].required_state = false;
    levers[1].conditions[0].alt_target_lever_index = -1;
    
    for (int i = 1; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        levers[0].conditions[i].target_lever_index = -1;
        levers[1].conditions[i].target_lever_index = -1;
    }
    levers[0].conditions[0].target_lever_index = -1; // Lever 0 has no rules

    tab_def_t tab;
    tab.levers = levers;
    tab.lever_count = 2;

    bool states[2] = {false, true}; // Lever 1 is ALREADY THROWN

    // Try to throw Lever 0. It should be blocked by Lever 1's rule.
    bool can_throw_0 = lever_evaluate_interlocking(&tab, states, 0, true);
    TEST_ASSERT_FALSE(can_throw_0);
}

void test_interlocking_mutual_locking(void)
{
    // Lever 0 locks Lever 1 Normal. Lever 1 locks Lever 0 Normal.
    lever_def_t levers[2];
    memset(levers, 0, sizeof(levers));
    
    levers[0].conditions[0].target_lever_index = 1;
    levers[0].conditions[0].required_state = false;
    levers[0].conditions[0].alt_target_lever_index = -1;
    
    levers[1].conditions[0].target_lever_index = 0;
    levers[1].conditions[0].required_state = false;
    levers[1].conditions[0].alt_target_lever_index = -1;
    
    for (int i = 1; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        levers[0].conditions[i].target_lever_index = -1;
        levers[1].conditions[i].target_lever_index = -1;
    }

    tab_def_t tab;
    tab.levers = levers;
    tab.lever_count = 2;

    bool states[2] = {false, false};

    // Both Normal. Throwing Lever 0 is fine.
    TEST_ASSERT_TRUE(lever_evaluate_interlocking(&tab, states, 0, true));
    
    // Assume Lever 0 is thrown
    states[0] = true;
    
    // Throwing Lever 1 should fail
    TEST_ASSERT_FALSE(lever_evaluate_interlocking(&tab, states, 1, true));
}

void test_interlocking_multiple_conditions(void)
{
    // Lever 0 locks Lever 1 Normal AND Lever 2 Reversed
    lever_def_t levers[3];
    memset(levers, 0, sizeof(levers));
    
    levers[0].conditions[0].target_lever_index = 1;
    levers[0].conditions[0].required_state = false;
    levers[0].conditions[0].alt_target_lever_index = -1;
    
    levers[0].conditions[1].target_lever_index = 2;
    levers[0].conditions[1].required_state = true;
    levers[0].conditions[1].alt_target_lever_index = -1;
    
    for (int i = 2; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        levers[0].conditions[i].target_lever_index = -1;
    }
    
    for (int i = 0; i < MAX_INTERLOCKING_CONDITIONS; i++) {
        levers[1].conditions[i].target_lever_index = -1;
        levers[2].conditions[i].target_lever_index = -1;
    }

    tab_def_t tab;
    tab.levers = levers;
    tab.lever_count = 3;

    // Case 1: Lever 1 Normal, Lever 2 Normal -> Fails because Lever 2 is not Reversed
    bool states1[3] = {false, false, false};
    TEST_ASSERT_FALSE(lever_evaluate_interlocking(&tab, states1, 0, true));
    
    // Case 2: Lever 1 Reversed, Lever 2 Reversed -> Fails because Lever 1 is not Normal
    bool states2[3] = {false, true, true};
    TEST_ASSERT_FALSE(lever_evaluate_interlocking(&tab, states2, 0, true));
    
    // Case 3: Lever 1 Normal, Lever 2 Reversed -> Passes both!
    bool states3[3] = {false, false, true};
    TEST_ASSERT_TRUE(lever_evaluate_interlocking(&tab, states3, 0, true));
}

int main(void) {
    UNITY_BEGIN();
    RUN_TEST(test_interlocking_simple_lock);
    RUN_TEST(test_interlocking_otherwise_logic);
    RUN_TEST(test_interlocking_reverse_locking);
    RUN_TEST(test_interlocking_mutual_locking);
    RUN_TEST(test_interlocking_multiple_conditions);
    return UNITY_END();
}
