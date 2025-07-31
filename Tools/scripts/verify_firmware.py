#!/usr/bin/env python3
'''
Verify firmware checksum for DGCA compliance
'''

import time
import sys
from pymavlink import mavutil

def verify_firmware_checksum(mavlink_connection, reference_checksum=None):
    '''
    Request and verify firmware checksum from the vehicle
    '''
    print("Requesting firmware checksum...")
    
    # Request checksum from vehicle
    mavlink_connection.mav.firmware_checksum_request_send(0)  # firmware_type=0 for main firmware
    
    # Wait for response with 5 second timeout
    start_time = time.time()
    while time.time() - start_time < 5:
        msg = mavlink_connection.recv_match(type='FIRMWARE_CHECKSUM', blocking=True, timeout=1)
        if msg is not None:
            checksum = ''.join(format(x, '02x') for x in msg.checksum)
            print(f"Received checksum: {checksum}")
            print(f"Verification status: {'Verified' if msg.checksum_verified else 'Not Verified'}")
            print(f"Firmware type: {msg.firmware_type}")
            print(f"Timestamp: {msg.time_boot_ms} ms")
            
            if reference_checksum:
                if checksum.lower() == reference_checksum.lower():
                    print("Checksum matches reference value!")
                else:
                    print("WARNING: Checksum does not match reference value!")
                    print(f"Expected: {reference_checksum}")
                    print(f"Received: {checksum}")
            return True
            
    print("Error: No response received from vehicle")
    return False

def main():
    '''
    Main function
    '''
    import argparse
    parser = argparse.ArgumentParser(description="Verify firmware checksum for DGCA compliance")
    parser.add_argument('--connect', required=True,
                      help='Vehicle connection string. E.g. udp:localhost:14550')
    parser.add_argument('--ref', '-r',
                      help='Reference checksum to verify against (optional)')
    args = parser.parse_args()
    
    try:
        # Connect to vehicle
        conn = mavutil.mavlink_connection(args.connect)
        print(f"Connecting to {args.connect}...")
        
        # Wait for heartbeat
        conn.wait_heartbeat()
        print("Connected!")
        
        # Verify checksum
        verify_firmware_checksum(conn, args.ref)
        
    except Exception as e:
        print(f"Error: {e}")
        return 1
    
    return 0

if __name__ == '__main__':
    sys.exit(main())
