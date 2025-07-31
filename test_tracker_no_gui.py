#!/usr/bin/env python3
"""
Test AntennaTracker without GUI
This script connects to the tracker and tests basic functionality
"""

import time
import sys
try:
    from pymavlink import mavutil
except ImportError:
    print("ERROR: pymavlink not installed. Install with: pip install pymavlink")
    sys.exit(1)

def test_tracker_connection(connection_string):
    """Test connection to AntennaTracker"""
    print(f"Connecting to tracker at {connection_string}")
    
    try:
        # Connect to tracker
        master = mavutil.mavlink_connection(connection_string)
        print("Waiting for heartbeat...")
        
        # Wait for heartbeat
        master.wait_heartbeat(timeout=10)
        print(f"Connected! System ID: {master.target_system}, Component ID: {master.target_component}")
        
        # Request parameters
        print("Requesting parameters...")
        master.mav.param_request_list_send(master.target_system, master.target_component)
        
        # Check some key parameters
        key_params = ['GPS_TYPE', 'COMPASS_ENABLE', 'AHRS_EKF_TYPE', 'START_LATITUDE', 'START_LONGITUDE']
        
        print("\nChecking key parameters:")
        for param_name in key_params:
            # Request specific parameter
            master.mav.param_request_read_send(
                master.target_system,
                master.target_component,
                param_name.encode('utf-8'),
                -1
            )
            
            # Wait for response
            msg = master.recv_match(type='PARAM_VALUE', timeout=5)
            if msg and msg.param_id.decode('utf-8').strip('\x00') == param_name:
                print(f"  {param_name}: {msg.param_value}")
            else:
                print(f"  {param_name}: Not received")
        
        # Get current mode
        print("\nRequesting current status...")
        master.mav.request_data_stream_send(
            master.target_system,
            master.target_component,
            mavutil.mavlink.MAV_DATA_STREAM_ALL,
            1, 1
        )
        
        # Wait for heartbeat to get mode
        msg = master.recv_match(type='HEARTBEAT', timeout=5)
        if msg:
            mode_mapping = {
                0: 'MANUAL',
                1: 'STOP', 
                2: 'SCAN',
                3: 'SERVO_TEST',
                4: 'AUTO',
                5: 'INITIALISING',
                10: 'GUIDED'
            }
            mode = mode_mapping.get(msg.custom_mode, f'UNKNOWN({msg.custom_mode})')
            armed = msg.base_mode & mavutil.mavlink.MAV_MODE_FLAG_SAFETY_ARMED
            print(f"Current mode: {mode}")
            print(f"Armed: {'YES' if armed else 'NO'}")
        
        # Try to switch to SCAN mode
        print("\nSwitching to SCAN mode...")
        master.mav.set_mode_send(
            master.target_system,
            mavutil.mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
            2  # SCAN mode
        )
        
        # Wait for mode change confirmation
        time.sleep(2)
        msg = master.recv_match(type='HEARTBEAT', timeout=5)
        if msg:
            mode = mode_mapping.get(msg.custom_mode, f'UNKNOWN({msg.custom_mode})')
            print(f"New mode: {mode}")
        
        # Check for any error messages
        print("\nChecking for status messages...")
        for i in range(10):  # Check for 10 messages
            msg = master.recv_match(type='STATUSTEXT', timeout=1)
            if msg:
                print(f"  STATUS: {msg.text.decode('utf-8')}")
        
        print("\nTest completed successfully!")
        return True
        
    except Exception as e:
        print(f"ERROR: {e}")
        return False

def main():
    # Default connection strings for different setups
    connections = [
        'tcp:127.0.0.1:5770',  # SITL tracker
        'udp:127.0.0.1:14551', # Alternative SITL
        '/dev/ttyUSB0',        # Hardware on USB
        '/dev/ttyACM0'         # Hardware on ACM
    ]
    
    print("AntennaTracker Test Script (No GUI)")
    print("=" * 40)
    
    if len(sys.argv) > 1:
        # Use provided connection string
        connection = sys.argv[1]
        print(f"Using connection: {connection}")
        test_tracker_connection(connection)
    else:
        # Try common connection strings
        print("Trying common connection strings...")
        for conn in connections:
            print(f"\nTrying {conn}...")
            if test_tracker_connection(conn):
                break
            time.sleep(1)

if __name__ == "__main__":
    main()
