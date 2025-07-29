# TFS20L Rangefinder Integration for ArduPilot

This document describes the integration of the Benewake TFS20L rangefinder sensor with ArduPilot.

## Overview

The TFS20L is an I2C-based rangefinder sensor that provides distance measurements through a register-based interface. Unlike command-response sensors like the TFMiniPlus, the TFS20L continuously updates internal registers that can be read directly.

## Features

- **Interface**: I2C (register-based communication)
- **Range**: Typically 10cm to 12m (sensor-dependent)
- **Update Rate**: 20Hz
- **Data Format**: 16-bit distance values in centimeters
- **Signal Quality**: Available through strength registers

## Hardware Setup

1. **Wiring**:
   - VCC: Connect to 3.3V or 5V (check sensor specifications)
   - GND: Connect to ground
   - SDA: Connect to I2C SDA line
   - SCL: Connect to I2C SCL line

2. **I2C Address**: Default 0x10 (configurable via RNGFND_ADDR parameter)

## ArduPilot Configuration

### Parameters

Set the following parameters via GCS (Mission Planner, QGroundControl, etc.):

```
RNGFND1_TYPE = 45        # TFS20L sensor type
RNGFND1_ADDR = 16        # I2C address (0x10 = 16 decimal)
RNGFND1_ORIENT = 25      # ROTATION_PITCH_270 (pointing down)
RNGFND1_MIN_CM = 10      # Minimum range in cm
RNGFND1_MAX_CM = 1200    # Maximum range in cm
RNGFND1_STOP_PIN = -1    # Not used for I2C sensors
RNGFND1_SETTLE = 0       # No settling time needed
RNGFND1_RMETRIC = 1      # Return last valid reading on timeout
RNGFND1_PWRRNG = 0       # No power saving
RNGFND1_GNDCLEAR = 10    # Ground clearance in cm
```

### Lua Script Integration

If you're using the wall-following Lua scripts provided, they will automatically detect and use the TFS20L sensor when properly configured. The scripts support:

- Distance-based wall following
- Obstacle avoidance
- Safety monitoring
- GCS status reporting

## Register Map

The register addresses used in the TFS20L driver implementation:

```cpp
// TFS20L register addresses
static constexpr uint8_t TFS20L_DIST_LOW = 0x00;         // Distance low byte
static constexpr uint8_t TFS20L_DIST_HIGH = 0x01;        // Distance high byte  
static constexpr uint8_t TFS20L_AMP_LOW = 0x02;          // Signal strength/amplitude low byte
static constexpr uint8_t TFS20L_AMP_HIGH = 0x03;         // Signal strength/amplitude high byte
static constexpr uint8_t TFS20L_TEMP_LOW = 0x04;         // Temperature low byte
static constexpr uint8_t TFS20L_TEMP_HIGH = 0x05;        // Temperature high byte
static constexpr uint8_t TFS20L_VERSION_REVISION = 0x0A; // Version revision byte
static constexpr uint8_t TFS20L_VERSION_MINOR = 0x0B;    // Version minor byte
static constexpr uint8_t TFS20L_VERSION_MAJOR = 0x0C;    // Version major byte
static constexpr uint8_t TFS20L_ENABLE = 0x25;           // Enable register
```

## Usage Instructions

1. **Install the Driver**: The TFS20L driver files are located in:
   - `libraries/AP_RangeFinder/AP_RangeFinder_Benewake_TFS20L.h`
   - `libraries/AP_RangeFinder/AP_RangeFinder_Benewake_TFS20L.cpp`

2. **Compile ArduPilot**: Build your target (Copter, Plane, Rover) with the new driver included.

3. **Configure Parameters**: Set the parameters as described above.

4. **Verify Operation**: 
   - Check GCS messages for "TFS20L: found sensor version X"
   - Monitor distance readings in GCS
   - Verify proper orientation and range limits

## Integration with Wall-Following Scripts

The provided Lua wall-following scripts automatically work with the TFS20L sensor:

### Basic Wall Following
```lua
-- The scripts will use rangefinder instance configured for lateral sensing
-- Typically RNGFND2 with ROTATION_YAW_90 (pointing right) or 
-- RNGFND3 with ROTATION_YAW_270 (pointing left)
```

### Configuration for Wall Following
```
# Downward-facing rangefinder (altitude)
RNGFND1_TYPE = 45
RNGFND1_ORIENT = 25      # ROTATION_PITCH_270 (down)

# Right-facing rangefinder (wall following)  
RNGFND2_TYPE = 45
RNGFND2_ORIENT = 2       # ROTATION_YAW_90 (right)
RNGFND2_ADDR = 17        # Different I2C address if multiple sensors

# Left-facing rangefinder (optional, for corridor navigation)
RNGFND3_TYPE = 45
RNGFND3_ORIENT = 6       # ROTATION_YAW_270 (left)
RNGFND3_ADDR = 18        # Different I2C address
```

## Troubleshooting

### Common Issues

1. **No Sensor Detected**:
   - Check I2C wiring
   - Verify I2C address (use i2cdetect on Linux systems)
   - Ensure sensor is powered correctly

2. **Inconsistent Readings**:
   - Check signal strength values
   - Verify sensor mounting (avoid vibrations)
   - Adjust MIN/MAX range parameters

3. **No Data Updates**:
   - Verify RNGFND1_TYPE is set to 45
   - Check ArduPilot message logs for error messages
   - Ensure sensor orientation is correct

### Diagnostic Messages

Monitor these GCS messages:
- "TFS20L: found sensor version X" (initialization success)
- "TFS20L: No response from sensor" (communication failure)
- "RangeFinder: No data" (no readings received)

## Performance Considerations

- **Update Rate**: 20Hz provides good responsiveness for most applications
- **Filtering**: ArduPilot automatically filters readings for stability
- **Multiple Sensors**: Use different I2C addresses for multiple TFS20L sensors

## Customization

### Modifying Register Addresses

Update the register definitions in `AP_RangeFinder_Benewake_TFS20L.h`:

```cpp
// Update these based on actual TFS20L datasheet
static constexpr uint8_t REG_DISTANCE_LOW = 0xXX;    // Your register address
static constexpr uint8_t REG_DISTANCE_HIGH = 0xXX;   // Your register address
// ... etc
```

### Adjusting Data Format

If the TFS20L uses different data formats, modify the `timer()` function in `AP_RangeFinder_Benewake_TFS20L.cpp`:

```cpp
// Example: if distance is in mm instead of cm
distance_cm = distance_raw / 10;  // Convert mm to cm

// Example: if using big-endian format
distance_cm = (uint16_t(distance_data[0]) << 8) | distance_data[1];
```

## Contributing

When updating this driver:
1. Update register addresses based on actual datasheet
2. Test with real hardware
3. Verify integration with existing ArduPilot systems
4. Update documentation as needed

## See Also

- [ArduPilot RangeFinder Documentation](https://ardupilot.org/copter/docs/common-rangefinder-landingpage.html)
- [Wall Following Lua Scripts](wall_follow_documentation.md)
- [TFMiniPlus Driver](AP_RangeFinder_Benewake_TFMiniPlus.cpp) (reference implementation)
