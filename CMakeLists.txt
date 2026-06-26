# The following five lines of boilerplate have to be in your project's
# CMakeLists in this exact order for cmake to work correctly
cmake_minimum_required(VERSION 3.22)

# Suppress the strict ESP-IDF v6 component validation warnings
function(message)
    list(JOIN ARGV " " FULL_MSG)
    if(ARGV0 STREQUAL "WARNING" AND FULL_MSG MATCHES "belongs to component")
        return() # Silently drop the validation warning
    endif()
    _message(${ARGV}) # Pass everything else
endfunction()
include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(esp32-lever-frame VERSION 1.7.0)
