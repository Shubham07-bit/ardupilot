#!/usr/bin/env python3
import hashlib
import os
import sys

def main():
    if len(sys.argv) != 2:
        print("Usage: python3 append_checksum.py <input_apj>")
        sys.exit(1)

    FIRMWARE_PATH = sys.argv[1]
    CHECKSUM_SIZE = 32
    if not os.path.exists(FIRMWARE_PATH):
        print(f"Input file not found: {FIRMWARE_PATH}")
        sys.exit(1)

    with open(FIRMWARE_PATH, "rb") as f:
        full_data = f.read()

    # Separate firmware and checksum
    firmware_data = full_data[:-CHECKSUM_SIZE]
    stored_checksum = full_data[-CHECKSUM_SIZE:]

    # Recalculate checksum
    calculated_checksum = hashlib.sha256(firmware_data).digest()

    # Compare
    print(f"📦 Stored Checksum     : {stored_checksum.hex()}")
    print(f"🔁 Recalculated Checksum: {calculated_checksum.hex()}")

    if stored_checksum == calculated_checksum:
        print("✅ Checksum is VALID")
    else:
        print("❌ Checksum is INVALID")

if __name__ == "__main__":
    main()