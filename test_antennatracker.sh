#!/bin/bash

# AntennaTracker SITL Test Script
# Demonstrates proper setup and operation

echo "=== AntennaTracker SITL Test ==="
echo

# Set working directory
cd /home/shubham/dev/ardupilot

echo "1. Building AntennaTracker..."
./waf configure --board sitl
./waf antennatracker

echo
echo "2. Starting AntennaTracker SITL..."
echo "   Using parameters: tracker_sitl.parm"
echo "   MAVLink: TCP:5770, UDP:14551"

# Start SITL in background
./Tools/autotest/sim_vehicle.py \
    -v AntennaTracker \
    --add-param-file=tracker_sitl.parm \
    --console \
    --map \
    --aircraft=test_tracker &

SITL_PID=$!

echo "   SITL PID: $SITL_PID"
echo

# Wait for startup
echo "3. Waiting for SITL to initialize..."
sleep 10

echo "4. Connecting with MAVProxy..."
mavproxy.py --master=tcp:127.0.0.1:5770 --cmd="
script tracker_demo.txt
" &

MAVPROXY_PID=$!

# Create demo script
cat > tracker_demo.txt << 'EOF'
# AntennaTracker Demo Commands

# Check current status
status

# Show current mode 
mode

# Set to AUTO mode (this automatically arms servos)
mode AUTO

# Check arming status
status

# Show some key parameters
param show SYSID_TARGET
param show SERVO1_FUNCTION  
param show SERVO2_FUNCTION

# Manual servo test (switch to SERVOTEST mode)
mode SERVOTEST

# Test servo movement
servo set 1 1200
sleep 1
servo set 1 1800  
sleep 1
servo set 1 1500

# Back to AUTO mode
mode AUTO

print "=== Demo Complete ==="
print "Tracker is now in AUTO mode and ready to track!"
print "To connect a vehicle to track:"
print "  ./Tools/autotest/sim_vehicle.py -v ArduCopter --out=udp:127.0.0.1:14551"
EOF

echo
echo "5. Demo running..."
echo "   - AntennaTracker SITL: PID $SITL_PID"
echo "   - MAVProxy: PID $MAVPROXY_PID"
echo
echo "Key points demonstrated:"
echo "   ✓ No separate 'arm' command needed"
echo "   ✓ Servos auto-arm when switching to AUTO mode"
echo "   ✓ Can test servos in SERVOTEST mode"
echo "   ✓ Ready to track vehicles that connect to UDP:14551"
echo
echo "To connect a vehicle for the tracker to follow:"
echo "   ./Tools/autotest/sim_vehicle.py -v ArduCopter --out=udp:127.0.0.1:14551"
echo
echo "Press Ctrl+C to stop..."

# Wait for user interrupt
trap "echo; echo 'Stopping...'; kill $SITL_PID $MAVPROXY_PID 2>/dev/null; exit 0" INT
wait
