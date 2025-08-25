
# Waf Upgrades: Secure Firmware Build, Signing, and Checksum

The flags `--signed-fw` and `--private-key` already existed for configuration.

**New logic was added:**
- After building, if `--signed-fw` and `--private-key` are set, the firmware is automatically signed.
- If `--append-checksum` is detected at build time, a checksum is appended to the (already signed) firmware.
- The process ensures signing always happens before checksum appending.

**Other details:**
- If `--signed-fw` is set but `--private-key` is missing, a warning is shown and signing is skipped.
- If the firmware file is missing, the post-build step is skipped with a warning.
- Signing options are set at configuration time and saved for future builds; checksum can be toggled per build.
- Manual signing and checksum scripts remain available for advanced use.

This enables a fully automated, robust, and user-friendly secure firmware build process for ArduPilot.
