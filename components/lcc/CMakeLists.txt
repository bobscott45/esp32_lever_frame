# NOTE: OpenLcbCLib is a Git submodule.
# Do not edit files inside OpenLcbCLib/ directly.
# Upstream: https://github.com/JimKueneman/OpenLcbCLib.git

cmake_minimum_required(VERSION 3.16)
file(GLOB_RECURSE SOURCES "OpenLcbCLib/src/*.c")

idf_component_register(
        SRCS ${SOURCES}
        INCLUDE_DIRS "../../main"  "OpenLcbCLib/src"
)