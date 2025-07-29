#!/bin/bash
#
# TFS20L Integration Build Test
# 
# This script tests if the TFS20L rangefinder integration compiles correctly
# with ArduPilot. It performs a basic compilation check for ArduCopter.
#

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ARDUPILOT_ROOT="$(cd "$SCRIPT_DIR/../.." && pwd)"

echo "TFS20L Integration Build Test"
echo "============================="
echo "ArduPilot Root: $ARDUPILOT_ROOT"
echo ""

cd "$ARDUPILOT_ROOT"

# Check if we're in the right directory
if [ ! -f "wscript" ]; then
    echo "Error: Not in ArduPilot root directory"
    echo "Please run this script from the ArduPilot root or fix paths"
    exit 1
fi

# Check for required files
echo "Checking TFS20L driver files..."
DRIVER_FILES=(
    "libraries/AP_RangeFinder/AP_RangeFinder_Benewake_TFS20L.h"
    "libraries/AP_RangeFinder/AP_RangeFinder_Benewake_TFS20L.cpp"
)

for file in "${DRIVER_FILES[@]}"; do
    if [ -f "$file" ]; then
        echo "✓ Found: $file"
    else
        echo "✗ Missing: $file"
        exit 1
    fi
done

echo ""
echo "Configuring build environment..."

# Configure for a common board (e.g., CubeOrange)
./waf configure --board CubeOrange --enable-asserts

echo ""
echo "Building ArduCopter with TFS20L integration..."

# Build only the rangefinder library and copter to test integration
./waf build --target bin/arducopter --jobs=4

if [ $? -eq 0 ]; then
    echo ""
    echo "✓ Build successful! TFS20L integration compiles correctly."
    echo ""
    echo "Next steps:"
    echo "1. Flash the firmware to your flight controller"
    echo "2. Set RNGFND1_TYPE = 45 for TFS20L"
    echo "3. Configure other rangefinder parameters as needed"
    echo "4. Test with actual hardware"
else
    echo ""
    echo "✗ Build failed. Check compilation errors above."
    exit 1
fi

echo ""
echo "Build test completed successfully!"
