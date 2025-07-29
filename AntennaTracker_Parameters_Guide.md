# AntennaTracker Parameters Guide

This guide covers all essential parameters for configuring an ArduPilot AntennaTracker, both for physical hardware and SITL simulation.

## Quick Setup for SITL

### Essential Parameters (Minimum Required)
```bash
param set SYSID_TARGET 1        # Track vehicle with System ID 1
param set MAV_UPDATE_RATE 5     # Request position updates at 5Hz
param set ALT_SOURCE 2          # Use GPS vehicle only for altitude
param set SERVO1_FUNCTION 6     # Yaw servo on channel 1
param set SERVO2_FUNCTION 7     # Pitch servo on channel 2
param write
```

### Connect and Test
```bash
# Connect to tracker
mavproxy.py --master=tcp:127.0.0.1:5770

# Arm and set mode
arm throttle
mode auto
```

---

## Complete Parameter Reference

### 1. Communication & Target Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| `SYSID_TARGET` | 1 | Vehicle's MAVLink system ID to track (0=auto-detect) |
| `MAV_UPDATE_RATE` | 5 | Rate (Hz) to request position/baro data from vehicle |

**Usage:**
```bash
param set SYSID_TARGET 1
param set MAV_UPDATE_RATE 5
```

---

### 2. Servo Configuration

| Parameter | Value | Description |
|-----------|-------|-------------|
| `SERVO_PITCH_TYPE` | 0 | Servo type: 0=Position, 1=OnOff, 2=Continuous |
| `SERVO_YAW_TYPE` | 0 | Servo type: 0=Position, 1=OnOff, 2=Continuous |
| `SERVO1_FUNCTION` | 6 | Channel 1 = Mount Yaw |
| `SERVO2_FUNCTION` | 7 | Channel 2 = Mount Pitch |

**Servo Output Setup:**
```bash
param set SERVO_PITCH_TYPE 0
param set SERVO_YAW_TYPE 0
param set SERVO1_FUNCTION 6     # Yaw
param set SERVO2_FUNCTION 7     # Pitch
param set SERVO1_MIN 1000
param set SERVO1_MAX 2000
param set SERVO1_TRIM 1500
param set SERVO2_MIN 1000
param set SERVO2_MAX 2000
param set SERVO2_TRIM 1500
```

---

### 3. Movement Limits & Range

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `YAW_RANGE` | 360 | 0-360° | Total yaw movement range |
| `PITCH_MIN` | -90 | -90-0° | Minimum pitch angle (down) |
| `PITCH_MAX` | 90 | 0-90° | Maximum pitch angle (up) |

**Range Configuration:**
```bash
param set YAW_RANGE 360
param set PITCH_MIN -90
param set PITCH_MAX 90
```

---

### 4. Tracking Behavior

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `DISTANCE_MIN` | 5.0 | 0-100m | Minimum distance to track target |
| `ALT_SOURCE` | 0 | 0-2 | Altitude source: 0=Baro, 1=GPS, 2=GPS vehicle only |
| `STARTUP_DELAY` | 0 | 0-10s | Delay before first servo movement |

**Tracking Setup:**
```bash
param set DISTANCE_MIN 5
param set ALT_SOURCE 2          # Recommended for SITL
param set STARTUP_DELAY 0
```

---

### 5. PID Control - Yaw Axis

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `YAW2SRV_P` | 1.0 | 0.0-3.0 | Proportional gain |
| `YAW2SRV_I` | 0.1 | 0.0-3.0 | Integral gain |
| `YAW2SRV_D` | 0.02 | 0.001-0.1 | Derivative gain |
| `YAW2SRV_IMAX` | 100 | 0-4000 | Maximum integral output |

**Yaw PID Tuning:**
```bash
param set YAW2SRV_P 1.0
param set YAW2SRV_I 0.1
param set YAW2SRV_D 0.02
param set YAW2SRV_IMAX 100
```

---

### 6. PID Control - Pitch Axis

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `PITCH2SRV_P` | 1.0 | 0.0-3.0 | Proportional gain |
| `PITCH2SRV_I` | 0.1 | 0.0-3.0 | Integral gain |
| `PITCH2SRV_D` | 0.02 | 0.001-0.1 | Derivative gain |
| `PITCH2SRV_IMAX` | 100 | 0-4000 | Maximum integral output |

**Pitch PID Tuning:**
```bash
param set PITCH2SRV_P 1.0
param set PITCH2SRV_I 0.1
param set PITCH2SRV_D 0.02
param set PITCH2SRV_IMAX 100
```

---

### 7. Movement Speed Control

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `YAW_SLEW_TIME` | 2 | 0-20s | Time for full yaw rotation (0=unlimited) |
| `PITCH_SLEW_TIME` | 2 | 0-20s | Time for full pitch movement (0=unlimited) |
| `MIN_REVERSE_TIME` | 1 | 0-20s | Minimum time for yaw reversal |

**Speed Control:**
```bash
param set YAW_SLEW_TIME 2
param set PITCH_SLEW_TIME 2
param set MIN_REVERSE_TIME 1
```

---

### 8. OnOff Servo Parameters (if using OnOff servos)

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `ONOFF_YAW_RATE` | 9.0 | 0-50°/s | Yaw rate for on/off servos |
| `ONOFF_PITCH_RATE` | 1.0 | 0-50°/s | Pitch rate for on/off servos |
| `ONOFF_YAW_MINT` | 0.1 | 0-2s | Minimum yaw movement time |
| `ONOFF_PITCH_MINT` | 0.1 | 0-2s | Minimum pitch movement time |

**OnOff Servo Setup:**
```bash
param set SERVO_PITCH_TYPE 1
param set SERVO_YAW_TYPE 1
param set ONOFF_YAW_RATE 9.0
param set ONOFF_PITCH_RATE 1.0
param set ONOFF_YAW_MINT 0.1
param set ONOFF_PITCH_MINT 0.1
```

---

### 9. Position Settings (No GPS)

| Parameter | Default | Range | Description |
|-----------|---------|-------|-------------|
| `START_LATITUDE` | 0 | -90 to 90° | Initial tracker latitude |
| `START_LONGITUDE` | 0 | -180 to 180° | Initial tracker longitude |

**Fixed Position Setup:**
```bash
param set START_LATITUDE -35.363261   # Your location
param set START_LONGITUDE 149.165230  # Your location
```

---

## Complete Setup Scripts

### Script 1: Basic SITL Setup
```bash
#!/bin/bash
# Basic AntennaTracker SITL setup

mavproxy.py --master=tcp:127.0.0.1:5770 --cmd="
param set SYSID_TARGET 1;
param set MAV_UPDATE_RATE 5;
param set ALT_SOURCE 2;
param set DISTANCE_MIN 5;
param set SERVO_PITCH_TYPE 0;
param set SERVO_YAW_TYPE 0;
param set SERVO1_FUNCTION 6;
param set SERVO2_FUNCTION 7;
param set YAW2SRV_P 1.0;
param set YAW2SRV_I 0.1;
param set PITCH2SRV_P 1.0;
param set PITCH2SRV_I 0.1;
param write;
arm throttle;
mode auto;
"
```

### Script 2: Physical Hardware Setup
```bash
#!/bin/bash
# Physical AntennaTracker setup with GPS

mavproxy.py --master=/dev/ttyUSB0 --baudrate=57600 --cmd="
param set SYSID_TARGET 1;
param set MAV_UPDATE_RATE 3;
param set ALT_SOURCE 1;
param set DISTANCE_MIN 10;
param set SERVO_PITCH_TYPE 0;
param set SERVO_YAW_TYPE 0;
param set SERVO1_FUNCTION 6;
param set SERVO2_FUNCTION 7;
param set SERVO1_MIN 1000;
param set SERVO1_MAX 2000;
param set SERVO1_TRIM 1500;
param set SERVO2_MIN 1000;
param set SERVO2_MAX 2000;
param set SERVO2_TRIM 1500;
param set YAW2SRV_P 0.8;
param set YAW2SRV_I 0.05;
param set PITCH2SRV_P 0.8;
param set PITCH2SRV_I 0.05;
param write;
"
```

---

## Parameter File Templates

### template_sitl.parm
```
SYSID_TARGET,1
MAV_UPDATE_RATE,5
ALT_SOURCE,2
DISTANCE_MIN,5
SERVO_PITCH_TYPE,0
SERVO_YAW_TYPE,0
SERVO1_FUNCTION,6
SERVO2_FUNCTION,7
YAW2SRV_P,1.0
YAW2SRV_I,0.1
YAW2SRV_D,0.02
PITCH2SRV_P,1.0
PITCH2SRV_I,0.1
PITCH2SRV_D,0.02
YAW_RANGE,360
PITCH_MIN,-90
PITCH_MAX,90
```

### template_hardware.parm
```
SYSID_TARGET,1
MAV_UPDATE_RATE,3
ALT_SOURCE,1
DISTANCE_MIN,10
SERVO_PITCH_TYPE,0
SERVO_YAW_TYPE,0
SERVO1_FUNCTION,6
SERVO2_FUNCTION,7
SERVO1_MIN,1000
SERVO1_MAX,2000
SERVO1_TRIM,1500
SERVO2_MIN,1000
SERVO2_MAX,2000
SERVO2_TRIM,1500
YAW2SRV_P,0.8
YAW2SRV_I,0.05
YAW2SRV_D,0.02
PITCH2SRV_P,0.8
PITCH2SRV_I,0.05
PITCH2SRV_D,0.02
YAW_SLEW_TIME,2
PITCH_SLEW_TIME,2
```

**Load parameter file:**
```bash
param load template_sitl.parm
```

---

## Verification Commands

### Check Critical Parameters
```bash
param show SYSID_TARGET
param show MAV_UPDATE_RATE
param show ALT_SOURCE
param show SERVO1_FUNCTION
param show SERVO2_FUNCTION
```

### Monitor Operation
```bash
status
mode
watch SERVO_OUTPUT_RAW
watch NAV_CONTROLLER_OUTPUT
watch GLOBAL_POSITION_INT
```

### Test Tracking
```bash
# Arm tracker
arm throttle

# Set to auto mode
mode auto

# Check if receiving position updates
module load messagetypes
messagetypes GLOBAL_POSITION_INT
```

---

## Troubleshooting

### Common Issues and Solutions

| Problem | Check Parameter | Solution |
|---------|----------------|----------|
| Tracker not moving | `SYSID_TARGET` | Set to vehicle's system ID (usually 1) |
| No position updates | `MAV_UPDATE_RATE` | Increase to 3-5 Hz |
| Wrong direction | `SERVO1_REV`, `SERVO2_REV` | Reverse servo direction |
| Oscillation | `YAW2SRV_P`, `PITCH2SRV_P` | Reduce P gain |
| Slow response | `YAW_SLEW_TIME`, `PITCH_SLEW_TIME` | Reduce or set to 0 |
| Altitude errors | `ALT_SOURCE` | Use 2 for SITL, 1 for hardware |

### Debug Commands
```bash
# Check communication
link

# Monitor messages
set streamrate -1
watch HEARTBEAT

# Check servo outputs
rc list
watch RC_CHANNELS
```

---

## Advanced Configuration

### Compass Calibration (Hardware Only)
```bash
# Start calibration
compass_magcal start

# Move tracker in all directions for 60 seconds
# ...

# Accept calibration
compass_magcal accept
```

### Logging Configuration
```bash
param set LOG_BITMASK 65535    # Log everything
param set LOG_DISARMED 1       # Log when disarmed
```

### GPS Configuration (Hardware)
```bash
param set GPS_TYPE 1           # AUTO
param set GPS_AUTO_CONFIG 1    # Auto configure
```

---

## AntennaTracker Arming System

**CRITICAL UNDERSTANDING**: AntennaTracker uses a completely different arming system than ArduCopter/ArduPlane.

### How AntennaTracker Arming Actually Works

1. **No traditional pre-arm checks**: There are NO `ARMING_CHECK`, `ARMING_REQUIRE`, or similar parameters
2. **Mode-based automatic arming**: Servos are automatically armed/disarmed when switching modes
3. **Simple servo enable/disable**: "Arming" just enables servo movement for tracking

### Mode Behavior

| Mode | Servos Armed | Description |
|------|-------------|-------------|
| `STOP` | ❌ Disarmed | Servos disabled, no movement |
| `INITIALISING` | ❌ Disarmed | Startup mode, servos disabled |
| `AUTO` | ✅ Armed | Automatic tracking mode |
| `MANUAL` | ✅ Armed | Manual control mode |
| `SCAN` | ✅ Armed | Scanning pattern mode |
| `GUIDED` | ✅ Armed | External guidance mode |
| `SERVOTEST` | ✅ Armed | Servo testing mode |

### Starting the Tracker

To activate tracking:

1. **Set mode to AUTO or MANUAL**:
   ```bash
   # Via MAVProxy
   mavproxy.py --master=tcp:127.0.0.1:5770
   mode AUTO
   
   # Via MAVLink command
   # MAV_CMD_DO_SET_MODE: mode=AUTO
   ```

2. **Servos automatically arm** when switching to tracking modes

3. **No separate arm command needed** (unlike Copter/Plane)

### Manual Arm/Disarm Commands

You can manually control servo arming via MAVLink:

```bash
# Arm servos manually
arm throttle

# Disarm servos manually  
disarm

# Via MAVLink: MAV_CMD_COMPONENT_ARM_DISARM
# param1=1 for arm, param1=0 for disarm
```

### Troubleshooting "Won't Arm" Issues

If your tracker seems stuck:

1. **Check current mode**: `param show FLTMODE*` or use `status` in MAVProxy
2. **Switch to AUTO mode**: `mode AUTO` 
3. **Verify servo configuration**: Check `SERVO1_FUNCTION=6` and `SERVO2_FUNCTION=7`
4. **Check for errors**: Use `errors` command in MAVProxy
5. **For no-GPS setups**: Ensure `START_LATITUDE` and `START_LONGITUDE` are set

### No-GPS Operation

For fixed-position trackers without GPS:

```bash
# Required for no-GPS operation
param set GPS_TYPE 0              # Disable GPS
param set START_LATITUDE 40.0719  # Your exact latitude
param set START_LONGITUDE -74.012 # Your exact longitude
param set AHRS_GPS_USE 0          # Don't use GPS for attitude
param set AHRS_EKF_TYPE 10        # Use DCM instead of EKF
```

**The tracker will work without GPS/compass if position is set correctly!**

---
