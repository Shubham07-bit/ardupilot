#!/usr/bin/env python3
import hashlib
import os
import sys

def main():
    if len(sys.argv) != 3:
        print("Usage: python3 append_checksum.py <input_apj> <output_apj>")
        sys.exit(1)

    BIN_INPUT = sys.argv[1]
    BIN_OUTPUT = sys.argv[2]
    CHECKSUM_SIZE = 32

    if not os.path.exists(BIN_INPUT):
        print(f"Input file not found: {BIN_INPUT}")
        sys.exit(1)

    # Load original firmware
    with open(BIN_INPUT, "rb") as f:
        firmware_data = bytearray(f.read())

    firmware_size = len(firmware_data)
    checksum_offset = firmware_size  # Inject right after firmware ends
    total_size = firmware_size + CHECKSUM_SIZE

    # Compute SHA-256 excluding the checksum region
    checksum = hashlib.sha256(firmware_data).digest()
    print(f"✅ SHA-256: {checksum.hex()}")
    print(f"📌 Firmware size: {firmware_size} bytes")
    print(f"📌 Checksum offset: 0x{checksum_offset:X}")

    # Pad firmware and inject checksum
    padded_data = firmware_data + checksum

    # Save final binary
    with open(BIN_OUTPUT, "wb") as f:
        f.write(padded_data)

    print(f"✅ Final binary size: {len(padded_data)} bytes")
    print(f"✅ Checksum injected at 0x{checksum_offset:X}")
    print("✅ Created apj with checksum:", BIN_OUTPUT)

if __name__ == "__main__":
    main()