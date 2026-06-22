#!/bin/bash

# bump_version.sh
# Updates the project version across CMakeLists.txt, CHANGELOG.md,
# and C/XML source files.

cd "$(dirname "$0")/.." || exit 1

NEW_VERSION=$1

if [ -z "$NEW_VERSION" ]; then
    echo "Usage: ./utils/bump_version.sh <new_version>"
    echo "Example: ./utils/bump_version.sh 1.3.0"
    exit 1
fi

# Ensure version format matches X.Y.Z
if ! [[ "$NEW_VERSION" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]]; then
    echo "❌ Error: Version must be in format X.Y.Z (e.g. 1.3.0)"
    exit 1
fi

echo "🚀 Bumping version to $NEW_VERSION..."

# 1. Update CMakeLists.txt
sed -i -E "s/(project\([^ ]+ VERSION )[0-9]+\.[0-9]+\.[0-9]+(\))/\1$NEW_VERSION\2/" CMakeLists.txt
echo "✅ Updated CMakeLists.txt"

# 2. Update components/openlcb_node/openlcb_user_config.c
sed -i -E "s/(\.snip\.hardware_version = \")[^\"]+(\",)/\1$NEW_VERSION\2/" components/openlcb_node/openlcb_user_config.c
sed -i -E "s/(\.snip\.software_version = \")[^\"]+(\",)/\1$NEW_VERSION\2/" components/openlcb_node/openlcb_user_config.c
echo "✅ Updated components/openlcb_node/openlcb_user_config.c"

# 3. Update components/openlcb_node/cdi_array.h
sed -i -E "s/(<hardwareVersion>)[^<]+(<\/hardwareVersion>)/\1$NEW_VERSION\2/" components/openlcb_node/cdi_array.h
sed -i -E "s/(<softwareVersion>)[^<]+(<\/softwareVersion>)/\1$NEW_VERSION\2/" components/openlcb_node/cdi_array.h
echo "✅ Updated components/openlcb_node/cdi_array.h"

# 4. Update CHANGELOG.md
CHANGELOG_DATE=$(date +%Y-%m-%d)
FIRST_LINE=$(grep -E "^## \[.*\]" CHANGELOG.md | head -n 1)

if [[ "$FIRST_LINE" != *"[$NEW_VERSION]"* ]]; then
    # Inject new version header before the first existing release
    awk -v ver="$NEW_VERSION" -v date="$CHANGELOG_DATE" '
    /^## \[/ && !inserted {
        print "## [" ver "] - " date "\n"
        print "### Added\n- \n"
        print "### Changed\n- \n"
        inserted=1
    }
    {print}
    ' CHANGELOG.md > CHANGELOG.tmp && mv CHANGELOG.tmp CHANGELOG.md
    echo "✅ Added template for $NEW_VERSION to CHANGELOG.md"
else
    echo "⚠️ Version $NEW_VERSION is already at the top of CHANGELOG.md."
fi

echo ""
echo "🎉 Done! Project version is now $NEW_VERSION."
