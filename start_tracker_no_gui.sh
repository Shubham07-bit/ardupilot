#!/bin/bash
# Startup script for AntennaTracker without GUI modules

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

echo -e "${GREEN}Starting AntennaTracker without GUI...${NC}"

# Check if tracker is already running
if pgrep -f "sim_vehicle.py.*Tracker" > /dev/null; then
    echo -e "${YELLOW}Tracker SITL already running${NC}"
else
    echo -e "${GREEN}Starting Tracker SITL...${NC}"
    cd /home/shubham/dev/ardupilot
    
    # Start tracker SITL in background
    Tools/autotest/sim_vehicle.py -v Tracker --console --map --out=tcpin:0.0.0.0:5770 &
    TRACKER_PID=$!
    
    echo "Waiting for tracker to start..."
    sleep 10
fi

echo -e "${GREEN}Connecting to tracker with MAVProxy (no GUI modules)...${NC}"

# Connect with MAVProxy but disable GUI modules that need wxpython
mavproxy.py \
    --master=tcp:127.0.0.1:5770 \
    --load-module="console" \
    --console \
    --cmd="param load tracker_no_gps.parm; param fetch; set heartbeat 2"

echo -e "${YELLOW}MAVProxy exited${NC}"

# Optionally kill the tracker SITL
if [ ! -z "$TRACKER_PID" ]; then
    echo -e "${YELLOW}Stopping Tracker SITL...${NC}"
    kill $TRACKER_PID
fi
