#!/usr/bin/env python3
"""
TFS20L Sensor Test Script

This script helps verify that the TFS20L sensor is properly connected
and communicating on the I2C bus.

Usage:
    python3 test_tfs20l.py [--address 0x10] [--bus 1]

Requirements:
    - Python 3
    - smbus library (pip3 install smbus)
    - I2C enabled on the system

Note: This script requires sudo privileges on most systems for I2C access.
"""

import argparse
import time
import sys

try:
    import smbus
except ImportError:
    print("Error: smbus library not found. Install with: pip3 install smbus")
    sys.exit(1)

class TFS20LTester:
    def __init__(self, bus_number=1, device_address=0x10):
        """
        Initialize TFS20L tester
        
        Args:
            bus_number: I2C bus number (typically 1 on Raspberry Pi)
            device_address: I2C address of TFS20L (default 0x10)
        """
        self.bus_number = bus_number
        self.device_address = device_address
        self.bus = None
        
        # Register addresses - matching the driver implementation
        self.REG_DISTANCE_LOW = 0x00
        self.REG_DISTANCE_HIGH = 0x01
        self.REG_STRENGTH_LOW = 0x02
        self.REG_STRENGTH_HIGH = 0x03
        self.REG_TEMP_LOW = 0x04
        self.REG_TEMP_HIGH = 0x05
        self.REG_VERSION_REVISION = 0x0A
        self.REG_VERSION_MINOR = 0x0B
        self.REG_VERSION_MAJOR = 0x0C
        self.REG_ENABLE = 0x25
        
    def connect(self):
        """Connect to I2C bus"""
        try:
            self.bus = smbus.SMBus(self.bus_number)
            print(f"Connected to I2C bus {self.bus_number}")
            return True
        except Exception as e:
            print(f"Failed to connect to I2C bus {self.bus_number}: {e}")
            return False
    
    def read_register(self, register_addr):
        """
        Read a single byte from the specified register
        
        Args:
            register_addr: Register address to read from
            
        Returns:
            Register value or None if failed
        """
        try:
            value = self.bus.read_byte_data(self.device_address, register_addr)
            return value
        except Exception as e:
            print(f"Failed to read register 0x{register_addr:02X}: {e}")
            return None
    
    def read_distance(self):
        """
        Read distance measurement from TFS20L
        
        Returns:
            Distance in cm or None if failed
        """
        try:
            # Read distance (assuming little-endian format)
            low_byte = self.read_register(self.REG_DISTANCE_LOW)
            high_byte = self.read_register(self.REG_DISTANCE_HIGH)
            
            if low_byte is None or high_byte is None:
                return None
                
            distance_cm = (high_byte << 8) | low_byte
            return distance_cm
            
        except Exception as e:
            print(f"Failed to read distance: {e}")
            return None
    
    def read_strength(self):
        """
        Read signal strength from TFS20L
        
        Returns:
            Signal strength or None if failed
        """
        try:
            # Read signal strength (assuming little-endian format)
            low_byte = self.read_register(self.REG_STRENGTH_LOW)
            high_byte = self.read_register(self.REG_STRENGTH_HIGH)
            
            if low_byte is None or high_byte is None:
                return None
                
            strength = (high_byte << 8) | low_byte
            return strength
            
        except Exception as e:
            print(f"Failed to read strength: {e}")
            return None
    
    def read_firmware_version(self):
        """
        Read firmware version from TFS20L
        
        Returns:
            Firmware version as string "major.minor.revision" or None if failed
        """
        try:
            # Enable the sensor
            try:
                self.bus.write_byte_data(self.device_address, self.REG_ENABLE, 0x01)
                print("Enabled sensor")
                time.sleep(0.1)  # Allow time for sensor to process
            except Exception as e:
                print(f"Warning: Could not enable sensor: {e}")
            
            # Read firmware version
            major = self.read_register(self.REG_VERSION_MAJOR)
            minor = self.read_register(self.REG_VERSION_MINOR)
            revision = self.read_register(self.REG_VERSION_REVISION)
            
            if major is None or minor is None or revision is None:
                return None
                
            return f"{major}.{minor}.{revision}"
            
        except Exception as e:
            print(f"Failed to read firmware version: {e}")
            return None
            
    def read_temperature(self):
        """
        Read temperature from TFS20L
        
        Returns:
            Temperature in degrees Celsius or None if failed
        """
        try:
            # Read temperature (assuming little-endian format)
            low_byte = self.read_register(self.REG_TEMP_LOW)
            high_byte = self.read_register(self.REG_TEMP_HIGH)
            
            if low_byte is None or high_byte is None:
                return None
                
            # Temperature is typically in 0.01°C units and may need a conversion factor
            temp_raw = (high_byte << 8) | low_byte
            temp_celsius = temp_raw / 100.0  # Assuming 0.01°C units, adjust as needed
            return temp_celsius
            
        except Exception as e:
            print(f"Failed to read temperature: {e}")
            return None
    
    def test_connectivity(self):
        """Test basic connectivity to the sensor"""
        print(f"Testing TFS20L connectivity at address 0x{self.device_address:02X}...")
        
        # Try to read firmware version
        version = self.read_firmware_version()
        if version is not None:
            print(f"✓ Sensor detected! Firmware version: {version}")
            return True
        else:
            print("✗ No response from sensor")
            return False
    
    def continuous_reading(self, duration=10):
        """
        Continuously read sensor data for specified duration
        
        Args:
            duration: Duration in seconds
        """
        print(f"Reading sensor data for {duration} seconds...")
        print("Distance(cm) | Strength | Temperature(°C)")
        
        start_time = time.time()
        while time.time() - start_time < duration:
            distance = self.read_distance()
            strength = self.read_signal_strength()
            temperature = self.read_temperature()
            
            if distance is not None and strength is not None:
                temp_str = f"{temperature:.2f}" if temperature is not None else "N/A"
                print(f"{distance:11d} | {strength:8d} | {temp_str}")
            else:
                print("Failed to read sensor data")
                
            time.sleep(0.1)
        print("-" * 35)
        
        start_time = time.time()
        while time.time() - start_time < duration:
            distance = self.read_distance()
            strength = self.read_strength()
            status = self.read_register(self.REG_STATUS)
            
            # Format output
            dist_str = f"{distance:4d}" if distance is not None else " N/A"
            strength_str = f"{strength:4d}" if strength is not None else " N/A"
            status_str = f"0x{status:02X}" if status is not None else " N/A"
            
            print(f"    {dist_str}     |   {strength_str}   |  {status_str}")
            
            time.sleep(0.1)  # 10Hz update rate
    
    def scan_i2c_bus(self):
        """Scan I2C bus for devices"""
        print(f"Scanning I2C bus {self.bus_number} for devices...")
        devices_found = []
        
        for addr in range(0x08, 0x78):  # Valid I2C address range
            try:
                self.bus.read_byte(addr)
                devices_found.append(addr)
                print(f"Device found at address 0x{addr:02X}")
            except:
                pass
        
        if not devices_found:
            print("No I2C devices found")
        else:
            print(f"Found {len(devices_found)} device(s)")
        
        return devices_found

def main():
    parser = argparse.ArgumentParser(description='Test TFS20L rangefinder sensor')
    parser.add_argument('--address', type=lambda x: int(x, 0), default=0x10,
                        help='I2C address of TFS20L (default: 0x10)')
    parser.add_argument('--bus', type=int, default=1,
                        help='I2C bus number (default: 1)')
    parser.add_argument('--scan', action='store_true',
                        help='Scan I2C bus for devices')
    parser.add_argument('--duration', type=int, default=10,
                        help='Duration for continuous reading in seconds (default: 10)')
    
    args = parser.parse_args()
    
    # Create tester instance
    tester = TFS20LTester(args.bus, args.address)
    
    # Connect to I2C bus
    if not tester.connect():
        sys.exit(1)
    
    try:
        if args.scan:
            # Scan for I2C devices
            devices = tester.scan_i2c_bus()
            if args.address not in devices:
                print(f"Warning: TFS20L not found at expected address 0x{args.address:02X}")
        
        # Test connectivity
        if not tester.test_connectivity():
            print("Connectivity test failed. Check wiring and I2C address.")
            sys.exit(1)
        
        # Continuous reading
        try:
            tester.continuous_reading(args.duration)
        except KeyboardInterrupt:
            print("\nStopped by user")
    
    except Exception as e:
        print(f"Error during testing: {e}")
        sys.exit(1)
    
    print("Test completed successfully")

if __name__ == "__main__":
    main()
