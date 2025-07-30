#pragma once

#include "AP_BattMonitor_SMBus_Generic.h"

#if AP_BATTERY_SMBUS_BQ34100_ENABLED

#define DEFAULT_BAT_CAP 11271 //mAh
// Specific reg addresses for BQ34100
// BQ34100 Register Addresses
#define REG_SOC                    0x02  // State of Charge
#define REG_REMAIN_CAP             0x04  // Remaining Capacity (mAh)
#define REG_FULL_CAP               0x06  // Full Charge Capacity (mAh)
#define REG_VOLTAGE                0x08  // Voltage (mV)
#define REG_AVG_CURRENT            0x0A  // Average Current (mA)
#define REG_BATT_TEMP              0x0C  // Battery Temperature (Kelvin)
#define REG_CYCLE_COUNT            0x2C  // Cycle Count
#define REG_SOH                    0x2E  // State of Health (%)
#define REG_FLAGS                  0x0E  // Battery Status Flags
#define REG_SERIAL_NUMBER          0x7E  // Serial Number
#define REG_CURRENT                0x10  // Current (mA)
#define REG_TIME_TO_EMPTY          0x18  // Time to Empty (minutes)
#define REG_TIME_TO_FULL           0x1A  // Time to Full (minutes)
#define REG_AVAILABLE_ENERGY       0x24  // Available Energy (Watt-hours)
#define REG_AVG_POWER              0x26  // Average Power (Watts)
#define REG_TTE_AT_CONSTANT_POWER  0x28  // Time to Empty at Constant Power (minutes)
#define REG_CHARGE_VOLTAGE         0x30  // Charge Voltage (mV)
#define REG_CHARGE_CURRENT         0x32  // Charge Current (mA)
#define REG_DESIGN_CAP             0x3C  // Design Capacity (mAh)



class AP_BattMonitor_SMBus_BQ34100: public AP_BattMonitor_SMBus
{
public:
    AP_BattMonitor_SMBus_BQ34100(AP_BattMonitor &mon,
                                  AP_BattMonitor::BattMonitor_State &mon_state,
                                  AP_BattMonitor_Params &params);
    void init(void) override;

private:

    void timer() override; // Override timer for custom logic
    // BQ34100 is a BMS attached to Battery directly
    void fetch_bms_data(); // Fetch all relevant data from the BMS
    
    float get_voltage();        // Voltage in volts
    float get_current();        // Current in amps
    float get_avg_current();    // avg current
    float get_temperature();    // Temperature in Celsius
    uint8_t get_soc();          // State of Charge in percentage
    uint16_t get_soh();         // State of Health
    uint16_t get_remaining_capacity(); // Remaining capacity in mAh
    uint16_t get_full_capacity();      // Full charge capacity in mAh
    // read_block_bare - returns number of characters read if successful, zero if unsuccessful
    bool read_block_bare(uint8_t reg, uint8_t* data, uint8_t len) const;
    void update_health();
    uint32_t last_volt_read_us;

};

#endif // AP_BATTERY_SMBUS_BQ34100_ENABLED
