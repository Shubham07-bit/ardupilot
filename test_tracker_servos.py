#!/usr/bin/env python3
"""
Test AntennaTracker servo outputs without GUI
This script manually controls the tracker servos for testing
"""

import time
import sys
from pymavlink import mavutil

def connect_tracker(connection_string="tcp:127.0.0.1:5770"):
    """Connect to the AntennaTracker"""
    print(f"Connecting to tracker at {connection_string}...")
    master = mavutil.mavlink_connection(connection_string)
    
    # Wait for heartbeat
    print("Waiting for heartbeat...")
    master.wait_heartbeat()
    print(f"Connected to system {master.target_system}, component {master.target_component}")
    return master

def set_servo_pwm(master, servo_num, pwm_value):
    """Set a specific servo to a PWM value"""
    print(f"Setting servo {servo_num} to {pwm_value} PWM")
    
    master.mav.command_long_send(
        master.target_system,
        master.target_component,
        mavutil.mavlink.MAV_CMD_DO_SET_SERVO,
        0,
        servo_num,    # Servo number (1-based)
        pwm_value,    # PWM value
        0, 0, 0, 0, 0
    )

def test_servo_sweep(master, servo_num, min_pwm=1000, max_pwm=2000, steps=5):
    """Test a servo by sweeping through its range"""
    print(f"\nTesting servo {servo_num} sweep from {min_pwm} to {max_pwm}")
    
    # Sweep from min to max
    for i in range(steps + 1):
        pwm = min_pwm + (max_pwm - min_pwm) * i // steps
        set_servo_pwm(master, servo_num, pwm)
        time.sleep(1)
    
    # Return to center
    center_pwm = (min_pwm + max_pwm) // 2
    set_servo_pwm(master, servo_num, center_pwm)
    time.sleep(1)

def set_tracker_mode(master, mode_name):
    """Set tracker mode"""
    modes = {
        'MANUAL': 0,
        'STOP': 1,
        'SCAN': 2,
        'SERVO_TEST': 3,
        'AUTO': 4,
        'INITIALISING': 5
    }
    
    if mode_name.upper() not in modes:
        print(f"Unknown mode: {mode_name}")
        return False
    
    mode_num = modes[mode_name.upper()]
    print(f"Setting mode to {mode_name} ({mode_num})")
    
    master.mav.command_long_send(
        master.target_system,
        master.target_component,
        mavutil.mavlink.MAV_CMD_DO_SET_MODE,
        0,
        mavutil.mavlink.MAV_MODE_FLAG_CUSTOM_MODE_ENABLED,
        mode_num,
        0, 0, 0, 0, 0
    )
    return True

def arm_tracker(master):
    """Arm the tracker"""
    print("Arming tracker...")
    master.mav.command_long_send(
        master.target_system,
        master.target_component,
        mavutil.mavlink.MAV_CMD_COMPONENT_ARM_DISARM,
        0,
        1,  # arm
        0, 0, 0, 0, 0, 0
    )

def main():
    if len(sys.argv) > 1:
        connection = sys.argv[1]
    else:
        connection = "tcp:127.0.0.1:5770"
    
    try:
        # Connect to tracker
        master = connect_tracker(connection)
        
        print("\n=== AntennaTracker Servo Test (No GUI) ===")
        print("This script will test your tracker servos manually")
        
        # Set to SERVO_TEST mode for manual control
        set_tracker_mode(master, "SERVO_TEST")
        time.sleep(2)
        
        # Arm the tracker
        arm_tracker()
        time.sleep(2)
        
        while True:
            print("\nServo Test Options:")
            print("1. Test Yaw servo (channel 1)")
            print("2. Test Pitch servo (channel 2)")
            print("3. Set custom servo PWM")
            print("4. Center all servos")
            print("5. Set tracker mode")
            print("6. Exit")
            
            choice = input("Enter choice (1-6): ").strip()
            
            if choice == "1":
                test_servo_sweep(master, 1, 1000, 2000, 4)
            elif choice == "2":
                test_servo_sweep(master, 2, 1000, 2000, 4)
            elif choice == "3":
                try:
                    servo = int(input("Servo number (1-8): "))
                    pwm = int(input("PWM value (1000-2000): "))
                    set_servo_pwm(master, servo, pwm)
                except ValueError:
                    print("Invalid input")
            elif choice == "4":
                print("Centering all servos...")
                set_servo_pwm(master, 1, 1500)  # Yaw center
                set_servo_pwm(master, 2, 1500)  # Pitch center
            elif choice == "5":
                mode = input("Enter mode (MANUAL/STOP/SCAN/SERVO_TEST/AUTO): ")
                set_tracker_mode(master, mode)
            elif choice == "6":
                print("Exiting...")
                break
            else:
                print("Invalid choice")
                
    except KeyboardInterrupt:
        print("\nExiting...")
    except Exception as e:
        print(f"Error: {e}")

if __name__ == "__main__":
    main()
