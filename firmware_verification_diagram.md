# ArduPilot Firmware Verification Process - Mermaid Diagram

## Overview
This diagram shows the complete firmware verification process in ArduPilot's bootloader, including CRC validation and signature verification.

## Main Flow Diagram

```mermaid
flowchart TD
    A[Bootloader Start] --> B["check_good_firmware()"]
    
    B --> C{"AP_SIGNED_FIRMWARE<br/>enabled?"}
    
    %% Signed Firmware Path
    C -->|Yes| D["check_good_firmware_signed()"]
    D --> E[Search for SIGNED signature]
    E --> F{"Descriptor<br/>found?"}
    F -->|No| G[FAIL_REASON_NO_APP_SIG]
    F -->|Yes| H[Check image size]
    H --> I{"Size valid?"}
    I -->|No| J[FAIL_REASON_BAD_LENGTH_APP]
    I -->|Yes| K[Check board ID]
    K --> L{"Board ID<br/>matches?"}
    L -->|No| M[FAIL_REASON_BAD_BOARD_ID]
    L -->|Yes| N[Calculate CRC for flash1 and flash2]
    N --> O{"CRC matches<br/>stored values?"}
    O -->|No| P[FAIL_REASON_BAD_CRC]
    O -->|Yes| Q["check_firmware_signature()"]
    
    %% Signature verification subprocess
    Q --> R{"All public keys<br/>zero?"}
    R -->|Yes| S[CHECK_FW_OK]
    R -->|No| T[Check signature length = 72]
    T --> U{"Length OK?"}
    U -->|No| V[FAIL_REASON_BAD_FIRMWARE_SIGNATURE]
    U -->|Yes| W[Check signature version = 30437]
    W --> X{"Version OK?"}
    X -->|No| V
    X -->|Yes| Y[Loop through all public keys]
    Y --> Z[crypto_check_init/update/final]
    Z --> AA{"Signature valid<br/>for any key?"}
    AA -->|Yes| S
    AA -->|No| BB[FAIL_REASON_VERIFICATION]
    
    %% Fallback logic for signed firmware
    D --> CC{"Result OK?"}
    CC -->|Yes| DD[Return CHECK_FW_OK]
    CC -->|No| EE{"All public keys<br/>zero?"}
    EE -->|No| FF[Return error]
    EE -->|Yes| GG["Try check_good_firmware_unsigned()"]
    GG --> HH{"Unsigned check<br/>OK?"}
    HH -->|Yes| DD
    HH -->|No| FF
    
    %% Unsigned Firmware Path
    C -->|No| II["check_good_firmware_unsigned()"]
    II --> JJ[Search for UNSIGNED signature]
    JJ --> KK{"Descriptor<br/>found?"}
    KK -->|No| LL[FAIL_REASON_NO_APP_SIG]
    KK -->|Yes| MM[Check image size]
    MM --> NN{"Size valid?"}
    NN -->|No| OO[FAIL_REASON_BAD_LENGTH_APP]
    NN -->|Yes| PP[Check board ID]
    PP --> QQ{"Board ID<br/>matches?"}
    QQ -->|No| RR[FAIL_REASON_BAD_BOARD_ID]
    QQ -->|Yes| SS[Calculate CRC for flash1 and flash2]
    SS --> TT{"CRC matches<br/>stored values?"}
    TT -->|No| UU[FAIL_REASON_BAD_CRC]
    TT -->|Yes| VV[CHECK_FW_OK]
    
    %% Fallback for unsigned firmware path
    II --> WW{"Result OK?"}
    WW -->|Yes| XX[Return CHECK_FW_OK]
    WW -->|No| YY["Try check_good_firmware_signed()"]
    YY --> ZZ{"Signed check OK?"}
    ZZ -->|Yes| XX
    ZZ -->|No| AAA[Return error]
    
    %% Final outcomes
    S --> BBB[Firmware Valid - Boot Application]
    DD --> BBB
    VV --> BBB
    XX --> BBB
    
    G --> CCC[Firmware Invalid - Stay in Bootloader]
    J --> CCC
    M --> CCC
    P --> CCC
    V --> CCC
    BB --> CCC
    FF --> CCC
    LL --> CCC
    OO --> CCC
    RR --> CCC
    UU --> CCC
    AAA --> CCC

    %% Styling
    classDef successBox fill:#90EE90,stroke:#006400,stroke-width:2px
    classDef errorBox fill:#FFB6C1,stroke:#8B0000,stroke-width:2px
    classDef processBox fill:#87CEEB,stroke:#000080,stroke-width:2px
    classDef decisionBox fill:#FFE4B5,stroke:#FF8C00,stroke-width:2px
    
    class S,DD,VV,XX,BBB successBox
    class G,J,M,P,V,BB,FF,LL,OO,RR,UU,AAA,CCC errorBox
    class D,E,H,K,N,Q,T,W,Y,Z,II,JJ,MM,PP,SS,YY processBox
    class C,F,I,L,O,R,U,X,AA,CC,EE,HH,KK,NN,QQ,TT,WW,ZZ decisionBox
```

## Memory Layout and CRC Calculation

```mermaid
flowchart LR
    subgraph "Flash Memory Layout"
        A[Bootloader<br/>FLASH_BOOTLOADER_LOAD_KB] --> B[App Start Offset<br/>APP_START_OFFSET_KB]
        B --> C[Application Code<br/>flash1]
        C --> D[App Descriptor<br/>Contains CRCs & Metadata]
        D --> E[More Application Code<br/>flash2]
        E --> F[Reserved End<br/>FLASH_RESERVE_END_KB]
    end
    
    subgraph "CRC Calculation"
        G[Calculate CRC1<br/>for flash1 region] --> H[Calculate CRC2<br/>for flash2 region]
        H --> I[Compare with<br/>stored CRC values<br/>in descriptor]
    end
    
    C -.-> G
    E -.-> H
    D -.-> I
```

## Error Codes and Results

```mermaid
graph TD
    A[check_fw_result_t] --> B[CHECK_FW_OK]
    A --> C[FAIL_REASON_NO_APP_SIG]
    A --> D[FAIL_REASON_BAD_LENGTH_APP]
    A --> E[FAIL_REASON_BAD_BOARD_ID]
    A --> F[FAIL_REASON_BAD_LENGTH_DESCRIPTOR]
    A --> G[FAIL_REASON_BAD_CRC]
    A --> H[FAIL_REASON_BAD_FIRMWARE_SIGNATURE]
    A --> I[FAIL_REASON_VERIFICATION]
    
    B --> J[✅ Boot Application]
    C --> K[❌ No descriptor found]
    D --> L[❌ Image too large]
    E --> M[❌ Wrong board]
    F --> N[❌ Bad descriptor]
    G --> O[❌ CRC mismatch]
    H --> P[❌ Bad signature format]
    I --> Q[❌ Signature verification failed]
    
    classDef success fill:#90EE90
    classDef error fill:#FFB6C1
    
    class B,J success
    class C,D,E,F,G,H,I,K,L,M,N,O,P,Q error
```

## Key Functions and Their Roles

| Function | Purpose |
|----------|---------|
| `check_good_firmware()` | Main entry point - decides signed vs unsigned path |
| `check_good_firmware_signed()` | Validates signed firmware with CRC and signature |
| `check_good_firmware_unsigned()` | Validates unsigned firmware with CRC only |
| `check_firmware_signature()` | Verifies cryptographic signature using public keys |
| `all_zero_public_keys()` | Checks if unsigned firmware is allowed |
| `crc32_small()` | Calculates CRC32 checksum for firmware regions |

## Security Analysis: Zero Public Keys Behavior

### Why Zero Keys Allow Boot

The bootloader allows firmware to boot when all public keys are zero due to a **development vs production build strategy**:

#### **Development/Debug Builds:**
- No cryptographic keys embedded in bootloader
- All public key slots contain zeros
- Enables rapid development without signing infrastructure
- Falls back to CRC-only validation

#### **Production Builds:**
- Real public keys embedded during manufacturing
- Strict signature verification required
- No fallback to unsigned verification

### **Critical Security Considerations:**

⚠️ **SECURITY RISK**: The zero-key fallback creates a potential attack vector:

1. **Attack Scenario**: If an attacker can zero out public keys in flash memory
2. **Fallback Triggered**: Bootloader detects "all zero keys"
3. **Weakened Security**: Falls back to CRC-only verification
4. **Malicious Firmware**: Attacker can load firmware that passes CRC but lacks valid signature

### **When Zero Keys Occur:**
- **Intentional**: Development builds without embedded keys
- **Manufacturing**: New boards before key programming
- **Corruption**: Flash memory corruption affecting key storage area
- **Attack**: Deliberate key erasure by malicious actor

### **Mitigation Strategies:**
- **Write-Protect Keys**: Use flash protection for public key storage area
- **Key Validation**: Additional integrity checks on public key area
- **Secure Boot**: Hardware-based root of trust
- **Production Flags**: Build-time flags to disable fallback behavior

### **OEM Security Responsibilities:**

🔒 **Critical OEM Protection Strategy**: The zero public key vulnerability is **significantly mitigated** when OEMs implement proper security measures:

#### **Bootloader Protection:**
- **OEM-Controlled Updates**: Only OEM can update/modify the bootloader itself
- **Protected Flash Sections**: Bootloader code and public keys in write-protected flash regions
- **Secure Update Channels**: Bootloader updates only through authenticated OEM processes

#### **Physical Security:**
- **Abstract Upgrade Ports**: Hide or disable direct bootloader access ports in production
- **Secure Connectors**: Programming interfaces accessible only with special tools/procedures  
- **Tamper Detection**: Hardware protection against unauthorized access attempts

#### **Build Configuration Security:**
```cpp
// Production builds should have:
#define AP_SIGNED_FIRMWARE 1        // ← Set at build time
// Public keys embedded in protected flash section
// Zero-key fallback disabled in production builds
```

#### **Why This Approach Works:**
1. **Attacker Cannot Modify Bootloader**: Without OEM access, bootloader code is immutable
2. **Public Keys Protected**: Keys stored in bootloader-protected flash regions
3. **Physical Access Required**: Direct hardware modification needed to compromise system
4. **OEM Controls Security**: Only authorized OEM processes can change security configuration

**Result**: If bootloader cannot be modified by attackers, the public keys remain intact and zero-key fallback never occurs, maintaining full signature verification security.

## Notes
- The bootloader splits firmware into two regions (flash1 and flash2) for CRC calculation
- Public keys are stored in a special section (`.apsec_data`) 
- Signature verification uses the Monocypher cryptographic library
- The system supports fallback from signed to unsigned verification and vice versa
- Board ID matching prevents loading firmware built for different hardware
- **⚠️ Zero public keys represent a significant security consideration in production systems**
