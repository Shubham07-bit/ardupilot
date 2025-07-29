#include "AP_BattMonitor_config.h"
#if AP_BATTERY_SMBUS_BQ34100_ENABLED

#include "AP_BattMonitor_SMBus_BQ34100.h"
#include <AP_Common/AP_Common.h>
#include <AP_Math/AP_Math.h>
#include "AP_BattMonitor.h"
#include <AP_HAL/AP_HAL.h>
#include <GCS_MAVLink/GCS.h>

#define DEBUG_BMS 0

extern const AP_HAL::HAL& hal;

AP_BattMonitor_SMBus_BQ34100::AP_BattMonitor_SMBus_BQ34100(AP_BattMonitor &mon,
                                                           AP_BattMonitor::BattMonitor_State &mon_state,
                                                           AP_BattMonitor_Params &params)
    : AP_BattMonitor_SMBus(mon, mon_state, params, AP_BATTMONITOR_SMBUS_BUS_INTERNAL) {
        _params._pack_capacity.set(DEFAULT_BAT_CAP);
    }

void AP_BattMonitor_SMBus_BQ34100::init(void){
    AP_BattMonitor_SMBus::init();
    if (_dev) {
        _dev->set_retries(2);
    }
    if (_dev && timer_handle) {
        // run twice as fast for two phases
        _dev->adjust_periodic_callback(timer_handle, 50000);
    }
#if DEBUG_BMS
    GCS_SEND_TEXT(MAV_SEVERITY_INFO, "BMS BQ34100 has been initialized..");
#endif
}

void AP_BattMonitor_SMBus_BQ34100::timer(){

    //call the base timer available in generic
    fetch_bms_data();
    // fetch the bms data
}

void AP_BattMonitor_SMBus_BQ34100::fetch_bms_data() {
    uint32_t tnow = AP_HAL::micros();
    _state.voltage = get_voltage();
    _state.current_amps = get_current();
    _state.temperature = get_temperature();
    _state.state_of_charge_pct = get_soc(); // added new variable
    _state.state_of_health_pct = get_soh();
    _state.full_charge_capacity = get_full_capacity(); // added new variable 
    _state.remaining_capacity = get_remaining_capacity(); // added new variable
    const uint32_t dt_us = tnow - _state.last_time_micros;

    // update total current drawn since startup
    update_consumed(_state, dt_us);
    update_health();
    _state.last_time_micros = tnow;    
#if DEBUG_BMS
    GCS_SEND_TEXT(MAV_SEVERITY_INFO, "Voltage: %.3f V, Current: %.5f A, SOC: %d%%, Temp: %.2fC", 
             _state.voltage, _state.current_amps, _state.state_of_charge_pct, _state.temperature);
#endif
}




float AP_BattMonitor_SMBus_BQ34100::get_voltage() {
    uint16_t data = 0;
    if (read_word(REG_VOLTAGE, data)) { // REG_VOLTAGE: 0x08
        last_volt_read_us = AP_HAL::micros();
        return data * 0.001f;           // Convert mV to V
        
    }
    return 0.0f;
}

// returns signed value
float AP_BattMonitor_SMBus_BQ34100::get_current() {
    int16_t raw_current = 0; // Current value as a signed 16-bit integer

    // Read 2 bytes from register 0x10 (current register)
    if (read_block_bare(REG_CURRENT, (uint8_t *)&raw_current, sizeof(raw_current))) {
        return abs(raw_current) / 1000.0f;
        // Convert raw current to amps (divide by 1000)
    }
#if DEBUG_BMS
    GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Failed to read current!");
#endif
    return _state.current_amps; // Fallback to the last valid temperature
}

float AP_BattMonitor_SMBus_BQ34100::get_temperature() {
    uint16_t data = 0;
    if (read_block_bare(REG_BATT_TEMP, (uint8_t *)&data, sizeof(data))) { // REG_BATT_TEMP: 0x0C
        return 0.1f * data - 273.15f;     // Convert to Celsius
    }
#if DEBUG_BMS
    GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Failed to read temperature!");
#endif
    return _state.temperature; // Fallback to the last valid temperature
}

uint8_t AP_BattMonitor_SMBus_BQ34100::get_soc() {
    uint8_t data = 0;
    if (read_block_bare(REG_SOC, (uint8_t *)&data, sizeof(data))) { // REG_SOC: 0x02
        return data;                // Percentage
    }
#if DEBUG_BMS
    GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Failed to read SOC!");
#endif
    return _state.state_of_charge_pct;  // Fallback to last known value
}

uint16_t AP_BattMonitor_SMBus_BQ34100::get_soh() {
    uint16_t data = 0;
    if (read_block_bare(REG_SOH, (uint8_t *)&data, sizeof(data))) { // REG_SOH: 0x2E
        return data;                // Percentage
    }
    return 0;
}

uint16_t AP_BattMonitor_SMBus_BQ34100::get_remaining_capacity() {
    uint16_t data = 0;
    if (read_block_bare(REG_REMAIN_CAP, (uint8_t *)&data, sizeof(data))) { // REG_REMAIN_CAP: 0x04
        return data;                       // Capacity in mAh
    }
    return 0;
}

uint16_t AP_BattMonitor_SMBus_BQ34100::get_full_capacity() {
    uint16_t data = 0;
    if (read_block_bare(REG_FULL_CAP, (uint8_t *)&data, sizeof(data))) { // REG_FULL_CAP: 0x06
        return data;                     // Capacity in mAh
    }
    return 0;
}

bool AP_BattMonitor_SMBus_BQ34100::read_block_bare(uint8_t reg, uint8_t* data, uint8_t len) const
{
    // read bytes
    if (!_dev->read_registers(reg, data, len)) {
        return false;
    }

    // return success
    return true;
}

void AP_BattMonitor_SMBus_BQ34100::update_health()
{
    uint32_t now = AP_HAL::micros();
    _state.healthy = (now - last_volt_read_us < AP_BATTMONITOR_SMBUS_TIMEOUT_MICROS) &&
        (now - _state.last_time_micros < AP_BATTMONITOR_SMBUS_TIMEOUT_MICROS);
}

#endif // AP_BATTERY_SMBUS_BQ34100_ENABLED