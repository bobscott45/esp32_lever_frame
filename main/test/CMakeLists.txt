cmake_minimum_required(VERSION 3.10)
project(InterlockingTests)

# Fetch Unity testing framework automatically
include(FetchContent)
FetchContent_Declare(
  unity
  GIT_REPOSITORY https://github.com/ThrowTheSwitch/Unity.git
  GIT_TAG        v2.5.2
)
FetchContent_MakeAvailable(unity)

# 1. Interlocking Tests
add_executable(run_interlocking_tests 
    test_interlocking.c 
    ../../components/lever_core/interlocking.c
)
target_include_directories(run_interlocking_tests PRIVATE ../../components/lever_core mocks)
target_link_libraries(run_interlocking_tests PRIVATE unity)

# Find the cJSON component in the ESP-IDF managed_components
set(CJSON_DIR "../../managed_components/espressif__cjson/cJSON")

# 2. Config Manager Tests
add_executable(run_config_tests
    test_config.c
    ../../components/lever_core/config_manager.c
    ../../components/lever_core/interlocking.c
    mocks/mock_nvs.c
    ${CJSON_DIR}/cJSON.c
)
target_include_directories(run_config_tests PRIVATE ../../components/lever_core mocks ${CJSON_DIR})
target_link_libraries(run_config_tests PRIVATE unity)

# 3. Controller Tests
add_executable(run_controller_tests
    test_controller.c
    ../../components/lever_core/controller.c
    ../../components/lever_core/config_manager.c
    ../../components/lever_core/interlocking.c
    mocks/mock_nvs.c
    ${CJSON_DIR}/cJSON.c
)
target_include_directories(run_controller_tests PRIVATE ../../components/lever_core mocks ${CJSON_DIR})
target_link_libraries(run_controller_tests PRIVATE unity)
