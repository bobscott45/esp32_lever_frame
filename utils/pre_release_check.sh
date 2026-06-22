#!/bin/bash

# pre_release_check.sh
# This script ensures that the version specified in CHANGELOG.md
# matches the hardcoded versions in the C and XML source files.

# Navigate to the script's parent directory (project root)
cd "$(dirname "$0")/.." || exit 1

# Extract latest version from CHANGELOG.md
# Assumes format like: ## [1.2.0] - 2026-06-22
CHANGELOG_VERSION=$(grep -E "^## \[.*\]" CHANGELOG.md | head -n 1 | sed -E 's/## \[([^]]+)\].*/\1/')

if [ -z "$CHANGELOG_VERSION" ]; then
    echo "❌ Error: Could not determine the latest version from CHANGELOG.md"
    exit 1
fi

echo "Expected Version (from CHANGELOG.md): $CHANGELOG_VERSION"

# Extract versions from components/openlcb_node/openlcb_user_config.c
CONFIG_HW_VERSION=$(grep "\.snip\.hardware_version" components/openlcb_node/openlcb_user_config.c | sed -E 's/.*"([^"]+)".*/\1/')
CONFIG_SW_VERSION=$(grep "\.snip\.software_version" components/openlcb_node/openlcb_user_config.c | sed -E 's/.*"([^"]+)".*/\1/')

# Extract version from CMakeLists.txt
CMAKE_VERSION=$(grep -E "^project\(.* VERSION .*\)" CMakeLists.txt | sed -E 's/.*VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/')

# Extract version from README.md
README_VERSION=$(grep -E "^# ESP32 Lever Frame v" README.md | sed -E 's/.*v([0-9]+\.[0-9]+\.[0-9]+).*/\1/')

# Extract versions from components/openlcb_node/cdi_array.h
CDI_HW_VERSION=$(grep "<hardwareVersion>" components/openlcb_node/cdi_array.h | sed -E 's/.*<hardwareVersion>([^<]+)<\/hardwareVersion>.*/\1/')
CDI_SW_VERSION=$(grep "<softwareVersion>" components/openlcb_node/cdi_array.h | sed -E 's/.*<softwareVersion>([^<]+)<\/softwareVersion>.*/\1/')

MISMATCH=0

# Perform checks
if [ "$CMAKE_VERSION" != "$CHANGELOG_VERSION" ]; then
    echo "❌ Mismatch in CMakeLists.txt: found '$CMAKE_VERSION'"
    MISMATCH=1
fi

if [ "$README_VERSION" != "$CHANGELOG_VERSION" ]; then
    echo "❌ Mismatch in README.md: found '$README_VERSION'"
    MISMATCH=1
fi

if [ "$CONFIG_HW_VERSION" != "$CHANGELOG_VERSION" ]; then
    echo "❌ Mismatch in components/openlcb_node/openlcb_user_config.c (hardware_version): found '$CONFIG_HW_VERSION'"
    MISMATCH=1
fi

if [ "$CONFIG_SW_VERSION" != "$CHANGELOG_VERSION" ]; then
    echo "❌ Mismatch in components/openlcb_node/openlcb_user_config.c (software_version): found '$CONFIG_SW_VERSION'"
    MISMATCH=1
fi

if [ "$CDI_HW_VERSION" != "$CHANGELOG_VERSION" ]; then
    echo "❌ Mismatch in components/openlcb_node/cdi_array.h (hardwareVersion): found '$CDI_HW_VERSION'"
    MISMATCH=1
fi

if [ "$CDI_SW_VERSION" != "$CHANGELOG_VERSION" ]; then
    echo "❌ Mismatch in components/openlcb_node/cdi_array.h (softwareVersion): found '$CDI_SW_VERSION'"
    MISMATCH=1
fi

if [ $MISMATCH -ne 0 ]; then
    echo ""
    echo "🚨 Pre-release version check FAILED."
    exit 1
else
    echo "✅ Pre-release version check PASSED. All versions match $CHANGELOG_VERSION."
    exit 0
fi
