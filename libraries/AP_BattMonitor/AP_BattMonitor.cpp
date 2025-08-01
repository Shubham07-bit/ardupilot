#include "AP_BattMonitor_config.h"

#if AP_BATTERY_ENABLED

#include "AP_BattMonitor.h"

#include "AP_BattMonitor_Analog.h"
#include "AP_BattMonitor_SMBus.h"
#include "AP_BattMonitor_SMBus_Solo.h"
#include "AP_BattMonitor_SMBus_Generic.h"
#include "AP_BattMonitor_SMBus_Maxell.h"
#include "AP_BattMonitor_SMBus_Rotoye.h"
#include "AP_BattMonitor_Bebop.h"
#include "AP_BattMonitor_ESC.h"
#include "AP_BattMonitor_SMBus_SUI.h"
#include "AP_BattMonitor_SMBus_NeoDesign.h"
#include "AP_BattMonitor_Sum.h"
#include "AP_BattMonitor_FuelFlow.h"
#include "AP_BattMonitor_FuelLevel_PWM.h"
#include "AP_BattMonitor_Generator.h"
#include "AP_BattMonitor_EFI.h"
#include "AP_BattMonitor_INA2xx.h"
#include "AP_BattMonitor_INA239.h"
#include "AP_BattMonitor_INA3221.h"
#include "AP_BattMonitor_LTC2946.h"
#include "AP_BattMonitor_Torqeedo.h"
#include "AP_BattMonitor_FuelLevel_Analog.h"
#include "AP_BattMonitor_Synthetic_Current.h"
#include "AP_BattMonitor_AD7091R5.h"
#include "AP_BattMonitor_Scripting.h"
#include "AP_BattMonitor_SMBus_BQ34100.h"

#include <AP_HAL/AP_HAL.h>

#if HAL_ENABLE_DRONECAN_DRIVERS
#include "AP_BattMonitor_DroneCAN.h"
#endif

#include <AP_Vehicle/AP_Vehicle_Type.h>
#include <AP_Logger/AP_Logger.h>
#include <GCS_MAVLink/GCS.h>
#include <AP_Notify/AP_Notify.h>

extern const AP_HAL::HAL& hal;

AP_BattMonitor *AP_BattMonitor::_singleton;

const AP_Param::GroupInfo AP_BattMonitor::var_info[] = {
    // 0 - 18, 20- 22 used by old parameter indexes

    // Monitor 1

    // @Group: _
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[0], "_", 23, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: _
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: _
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: _
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: _
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: _
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: _
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: _
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: _
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: _
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: _
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: _
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[0], "_", 41, AP_BattMonitor, backend_var_info[0]),

#if AP_BATT_MONITOR_MAX_INSTANCES > 1
    // @Group: 2_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[1], "2_", 24, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 2_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 2_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[1], "2_", 42, AP_BattMonitor, backend_var_info[1]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 2
    // @Group: 3_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[2], "3_", 25, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 3_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 3_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[2], "3_", 43, AP_BattMonitor, backend_var_info[2]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 3
    // @Group: 4_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[3], "4_", 26, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 4_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 4_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[3], "4_", 44, AP_BattMonitor, backend_var_info[3]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 4
    // @Group: 5_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[4], "5_", 27, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 5_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 5_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[4], "5_", 45, AP_BattMonitor, backend_var_info[4]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 5
    // @Group: 6_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[5], "6_", 28, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 6_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 6_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[5], "6_", 46, AP_BattMonitor, backend_var_info[5]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 6
    // @Group: 7_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[6], "7_", 29, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 7_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 7_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[6], "7_", 47, AP_BattMonitor, backend_var_info[6]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 7
    // @Group: 8_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[7], "8_", 30, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 8_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 8_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[7], "8_", 48, AP_BattMonitor, backend_var_info[7]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 8
    // @Group: 9_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[8], "9_", 31, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: 9_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: 9_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[8], "9_", 49, AP_BattMonitor, backend_var_info[8]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 9
    // @Group: A_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[9], "A_", 32, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: A_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: A_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[9], "A_", 50, AP_BattMonitor, backend_var_info[9]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 10
    // @Group: B_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[10], "B_", 33, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: B_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: B_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[10], "B_", 51, AP_BattMonitor, backend_var_info[10]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 11
    // @Group: C_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[11], "C_", 34, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: C_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: C_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[11], "C_", 52, AP_BattMonitor, backend_var_info[11]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 12
    // @Group: D_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[12], "D_", 35, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: D_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: D_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[12], "D_", 53, AP_BattMonitor, backend_var_info[12]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 13
    // @Group: E_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[13], "E_", 36, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: E_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: E_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[13], "E_", 54, AP_BattMonitor, backend_var_info[13]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 14
    // @Group: F_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[14], "F_", 37, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: F_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: F_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[14], "F_", 55, AP_BattMonitor, backend_var_info[14]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 15
    // @Group: G_
    // @Path: AP_BattMonitor_Params.cpp
    AP_SUBGROUPINFO(_params[15], "G_", 38, AP_BattMonitor, AP_BattMonitor_Params),

    // @Group: G_
    // @Path: AP_BattMonitor_Analog.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_SMBus.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_Sum.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_DroneCAN.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_FuelLevel_Analog.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_Synthetic_Current.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_INA2xx.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_ESC.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_INA239.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_INA3221.cpp
    // @Group: G_
    // @Path: AP_BattMonitor_AD7091R5.cpp
    AP_SUBGROUPVARPTR(drivers[15], "G_", 56, AP_BattMonitor, backend_var_info[15]),
#endif

#if AP_BATT_MONITOR_MAX_INSTANCES > 16
    #error "AP_BATT_MONITOR_MAX_INSTANCES too large, reset_remaining_mask() will cause an assert above 16"
#endif

    AP_GROUPEND
};

const AP_Param::GroupInfo *AP_BattMonitor::backend_var_info[AP_BATT_MONITOR_MAX_INSTANCES];

// Default constructor.
// Note that the Vector/Matrix constructors already implicitly zero
// their values.
//
AP_BattMonitor::AP_BattMonitor(uint32_t log_battery_bit, battery_failsafe_handler_fn_t battery_failsafe_handler_fn, const int8_t *failsafe_priorities) :
    _log_battery_bit(log_battery_bit),
    _battery_failsafe_handler_fn(battery_failsafe_handler_fn),
    _failsafe_priorities(failsafe_priorities)
{
    AP_Param::setup_object_defaults(this, var_info);

    if (_singleton != nullptr) {
        AP_HAL::panic("AP_BattMonitor must be singleton");
    }
    _singleton = this;
}

// init - instantiate the battery monitors
void
AP_BattMonitor::init()
{
    // check init has not been called before
    if (_num_instances != 0) {
        return;
    }

    _highest_failsafe_priority = INT8_MAX;

#ifdef HAL_BATT_MONITOR_DEFAULT
    _params[0]._type.set_default(int8_t(HAL_BATT_MONITOR_DEFAULT));
#endif
#ifdef HAL_BATT2_MONITOR_DEFAULT
    _params[1]._type.set_default(int8_t(HAL_BATT2_MONITOR_DEFAULT));
#endif

    // create each instance
    for (uint8_t instance=0; instance<AP_BATT_MONITOR_MAX_INSTANCES; instance++) {
        // clear out the cell voltages
        memset(&state[instance].cell_voltages, 0xFF, sizeof(cells));
        state[instance].instance = instance;

        const auto allocation_type = configured_type(instance);
        
        switch (allocation_type) {
#if AP_BATTERY_ANALOG_ENABLED
            case Type::ANALOG_VOLTAGE_ONLY:
            case Type::ANALOG_VOLTAGE_AND_CURRENT:
            case Type::ANALOG_CURRENT_ONLY:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Analog(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SMBUS_SOLO_ENABLED
            case Type::SOLO:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_Solo(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SMBUS_GENERIC_ENABLED
            case Type::SMBus_Generic:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_Generic(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SMBUS_SUI_ENABLED
            case Type::SUI3:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_SUI(*this, state[instance], _params[instance], 3);
                break;
            case Type::SUI6:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_SUI(*this, state[instance], _params[instance], 6);
                break;
#endif
#if AP_BATTERY_SMBUS_MAXELL_ENABLED
            case Type::MAXELL:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_Maxell(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SMBUS_ROTOYE_ENABLED
            case Type::Rotoye:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_Rotoye(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SMBUS_NEODESIGN_ENABLED
            case Type::NeoDesign:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_SMBus_NeoDesign(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_BEBOP_ENABLED
            case Type::BEBOP:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Bebop(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_UAVCAN_BATTERYINFO_ENABLED
            case Type::UAVCAN_BatteryInfo:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_DroneCAN(*this, state[instance], AP_BattMonitor_DroneCAN::UAVCAN_BATTERY_INFO, _params[instance]);
                break;
#endif
#if AP_BATTERY_ESC_ENABLED
            case Type::BLHeliESC:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_ESC(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SUM_ENABLED
            case Type::Sum:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Sum(*this, state[instance], _params[instance], instance);
                break;
#endif
#if AP_BATTERY_FUELFLOW_ENABLED
            case Type::FuelFlow:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_FuelFlow(*this, state[instance], _params[instance]);
                break;
#endif // AP_BATTERY_FUELFLOW_ENABLED
#if AP_BATTERY_FUELLEVEL_PWM_ENABLED
            case Type::FuelLevel_PWM:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_FuelLevel_PWM(*this, state[instance], _params[instance]);
                break;
#endif // AP_BATTERY_FUELLEVEL_PWM_ENABLED
#if AP_BATTERY_FUELLEVEL_ANALOG_ENABLED
            case Type::FuelLevel_Analog:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_FuelLevel_Analog(*this, state[instance], _params[instance]);
                break;
#endif // AP_BATTERY_FUELLEVEL_ANALOG_ENABLED
#if HAL_GENERATOR_ENABLED
            case Type::GENERATOR_ELEC:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Generator_Elec(*this, state[instance], _params[instance]);
                break;
            case Type::GENERATOR_FUEL:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Generator_FuelLevel(*this, state[instance], _params[instance]);
                break;
#endif // HAL_GENERATOR_ENABLED
#if AP_BATTERY_INA2XX_ENABLED
            case Type::INA2XX:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_INA2XX(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_LTC2946_ENABLED
            case Type::LTC2946:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_LTC2946(*this, state[instance], _params[instance]);
                break;
#endif
#if HAL_TORQEEDO_ENABLED
            case Type::Torqeedo:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Torqeedo(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_SYNTHETIC_CURRENT_ENABLED
            case Type::Analog_Volt_Synthetic_Current:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Synthetic_Current(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_INA239_ENABLED
            case Type::INA239_SPI:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_INA239(*this, state[instance], _params[instance]);
                break;
#endif
#if AP_BATTERY_EFI_ENABLED
            case Type::EFI:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_EFI(*this, state[instance], _params[instance]);
                break;
#endif // AP_BATTERY_EFI_ENABLED
#if AP_BATTERY_AD7091R5_ENABLED
            case Type::AD7091R5:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_AD7091R5(*this, state[instance], _params[instance]);
                break;
#endif// AP_BATTERY_AD7091R5_ENABLED
#if AP_BATTERY_SCRIPTING_ENABLED
            case Type::Scripting:
                GCS_SEND_TEXT(MAV_SEVERITY_DEBUG, "Set to Scripting");
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_Scripting(*this, state[instance], _params[instance]);
                break;
#endif // AP_BATTERY_SCRIPTING_ENABLED
#if AP_BATTERY_INA3221_ENABLED
            case Type::INA3221:
                drivers[instance] = NEW_NOTHROW AP_BattMonitor_INA3221(*this, state[instance], _params[instance]);
                break;
#endif  // AP_BATTERY_INA3221_ENABLED
            case Type::NONE:
            default:
                break;
        }
        if (drivers[instance] != nullptr) {
            state[instance].type = allocation_type;
        }
    // if the backend has some local parameters then make those available in the tree
    if (drivers[instance] && state[instance].var_info) {
        backend_var_info[instance] = state[instance].var_info;
        AP_Param::load_object_from_eeprom(drivers[instance], backend_var_info[instance]);

        // param count could have changed
        AP_Param::invalidate_count();
    }

        // call init function for each backend
        if (drivers[instance] != nullptr) {
            drivers[instance]->init();
            // _num_instances is actually the index for looping over instances
            // the user may have BATT_MONITOR=0 and BATT2_MONITOR=7, in which case
            // there will be a gap, but as we always check for drivers[instances] being nullptr
            // this is safe
            _num_instances = instance + 1;

            // Convert the old analog & Bus parameters to the new dynamic parameter groups
            convert_dynamic_param_groups(instance);
        }
    }
}

void AP_BattMonitor::convert_dynamic_param_groups(uint8_t instance)
{
    AP_Param::ConversionInfo info;
    if (!AP_Param::find_top_level_key_by_pointer(this, info.old_key)) {
        return;
    }

    char param_prefix[6] {};
    char param_name[17] {};
    info.new_name = param_name;

    const uint8_t param_instance = instance + 1;
    // first battmonitor does not have '1' in the param name
    if(param_instance == 1) {
        hal.util->snprintf(param_prefix, sizeof(param_prefix), "BATT");
    } else {
        hal.util->snprintf(param_prefix, sizeof(param_prefix), "BATT%X", param_instance);
    }
    param_prefix[sizeof(param_prefix)-1] = '\0';

    hal.util->snprintf(param_name, sizeof(param_name), "%s_%s", param_prefix, "MONITOR");
    param_name[sizeof(param_name)-1] = '\0';

    // Find the index of the BATTn_MONITOR which is not moving to index the moving parameters off from
    AP_Param::ParamToken token = AP_Param::ParamToken {};
    ap_var_type type;
    AP_Param* param = AP_Param::find_by_name(param_name, &type, &token);
    const uint8_t battmonitor_index = 1;
    if( param == nullptr) {
        // BATTn_MONITOR not found
        return;
    }

    const struct convert_table {
        uint32_t old_group_element;
        ap_var_type type;
        const char* new_name;
    }  conversion_table[] = {
        // PARAMETER_CONVERSION - Added: Aug-2021
            { 2,  AP_PARAM_INT8,  "VOLT_PIN"  },
            { 3,  AP_PARAM_INT8,  "CURR_PIN"  },
            { 4,  AP_PARAM_FLOAT, "VOLT_MULT" },
            { 5,  AP_PARAM_FLOAT, "AMP_PERVLT"},
            { 6,  AP_PARAM_FLOAT, "AMP_OFFSET"},
            { 20, AP_PARAM_INT8,  "I2C_BUS"   },
        };

    for (const auto & elem : conversion_table) {
        info.old_group_element = token.group_element + ((elem.old_group_element - battmonitor_index) * 64);
        info.type = elem.type;

        hal.util->snprintf(param_name, sizeof(param_name), "%s_%s", param_prefix, elem.new_name);
        AP_Param::convert_old_parameter(&info, 1.0f, 0);
    }
}

// read - For all active instances read voltage & current; log BAT, BCL, POWR, MCU
void AP_BattMonitor::read()
{
#if HAL_LOGGING_ENABLED
    AP_Logger *logger = AP_Logger::get_singleton();
    if (logger != nullptr && logger->should_log(_log_battery_bit)) {
        logger->Write_Power();
    }
#endif

    const uint32_t now_ms = AP_HAL::millis();
    for (uint8_t i=0; i<_num_instances; i++) {
            if (drivers[i] == nullptr) {
                continue;
            }
            if (allocated_type(i) != configured_type(i)) {
                continue;
            }
            // allow run-time disabling; this is technically redundant
            if (configured_type(i) == Type::NONE) {
                continue;
            }
            drivers[i]->read();
            drivers[i]->update_resistance_estimate();

#if AP_BATTERY_ESC_TELEM_OUTBOUND_ENABLED
            drivers[i]->update_esc_telem_outbound();
#endif

            // Update last heathy timestamp
            if (state[i].healthy) {
                state[i].last_healthy_ms = now_ms;
            }

#if HAL_LOGGING_ENABLED
            if (logger != nullptr && logger->should_log(_log_battery_bit)) {
                const uint64_t time_us = AP_HAL::micros64();
                drivers[i]->Log_Write_BAT(i, time_us);
                drivers[i]->Log_Write_BCL(i, time_us);
            }
#endif
    }

    check_failsafes();
    
    checkPoweringOff();
}

// healthy - returns true if monitor is functioning
// bool AP_BattMonitor::healthy(uint8_t instance) const {
//     return instance < _num_instances && state[instance].healthy;
// }

bool AP_BattMonitor::healthy(uint8_t instance) const {
    return (instance < _num_instances && state[instance].healthy);
}


/// voltage - returns battery voltage in volts
float AP_BattMonitor::voltage(uint8_t instance) const
{
    if (instance < _num_instances) {
        return state[instance].voltage;
    } else {
        return 0.0f;
    }
}

/// get voltage with sag removed (based on battery current draw and resistance)
/// this will always be greater than or equal to the raw voltage
float AP_BattMonitor::voltage_resting_estimate(uint8_t instance) const
{
    if (instance < _num_instances && drivers[instance] != nullptr) {
        return drivers[instance]->voltage_resting_estimate();
    } else {
        return 0.0f;
    }
}

/// voltage - returns battery voltage in volts for GCS, may be resting voltage if option enabled
float AP_BattMonitor::gcs_voltage(uint8_t instance) const
{
    if (instance >= _num_instances || drivers[instance] == nullptr) {
        return 0.0f;
    }
    if (drivers[instance]->option_is_set(AP_BattMonitor_Params::Options::GCS_Resting_Voltage)) {
        return voltage_resting_estimate(instance);
    }
    return state[instance].voltage;
}

bool AP_BattMonitor::option_is_set(uint8_t instance, AP_BattMonitor_Params::Options option) const
{
    if (instance >= _num_instances || drivers[instance] == nullptr) {
        return false;
    }
    return drivers[instance]->option_is_set(option);
}

/// current_amps - returns the instantaneous current draw in amperes
bool AP_BattMonitor::current_amps(float &current, uint8_t instance) const {
    if ((instance < _num_instances) && (drivers[instance] != nullptr) && drivers[instance]->has_current()) {
        current = state[instance].current_amps;
        return true;
    } else {
        return false;
    }
}

/// consumed_mah - returns total current drawn since start-up in milliampere.hours
bool AP_BattMonitor::consumed_mah(float &mah, const uint8_t instance) const {
    if ((instance < _num_instances) && (drivers[instance] != nullptr) && drivers[instance]->has_current()) {
        const float consumed_mah = state[instance].consumed_mah;
        if (isnan(consumed_mah)) {
            return false;
        }
        mah = consumed_mah;
        return true;
    } else {
        return false;
    }
}

/// consumed_wh - returns energy consumed since start-up in Watt.hours
bool AP_BattMonitor::consumed_wh(float &wh, const uint8_t instance) const {
    if (instance < _num_instances && drivers[instance] != nullptr && drivers[instance]->has_consumed_energy()) {
        wh = state[instance].consumed_wh;
        return true;
    } else {
        return false;
    }
}

/// capacity_remaining_pct - returns true if the percentage is valid and writes to percentage argument
bool AP_BattMonitor::capacity_remaining_pct(uint8_t &percentage, uint8_t instance) const
{
    if (instance < _num_instances && drivers[instance] != nullptr) {
        return drivers[instance]->capacity_remaining_pct(percentage);
    }
    return false;
}

/// time_remaining - returns remaining battery time
bool AP_BattMonitor::time_remaining(uint32_t &seconds, uint8_t instance) const
{
    if (instance < _num_instances && drivers[instance] != nullptr && state[instance].has_time_remaining) {
        seconds = state[instance].time_remaining;
        return true;
    }
    return false;
}

/// pack_capacity_mah - returns the capacity of the battery pack in mAh when the pack is full
int32_t AP_BattMonitor::pack_capacity_mah(uint8_t instance) const
{
    if (instance < AP_BATT_MONITOR_MAX_INSTANCES) {
        return _params[instance]._pack_capacity;
    } else {
        return 0;
    }
}

void AP_BattMonitor::check_failsafes(void)
{
    if (hal.util->get_soft_armed()) {
        for (uint8_t i = 0; i < _num_instances; i++) {
            if (drivers[i] == nullptr) {
                continue;
            }

            const Failsafe type = drivers[i]->update_failsafes();
            if (type <= state[i].failsafe) {
                continue;
            }

            int8_t action = 0;
            const char *type_str = nullptr;
            switch (type) {
                case Failsafe::None:
                    continue; // should not have been called in this case
                case Failsafe::Unhealthy:
                    // Report only for unhealthy, could add action param in the future
                    action = 0;
                    type_str = "missing, last:";
                    break;
                case Failsafe::Low:
                    action = _params[i]._failsafe_low_action;
                    type_str = "low";
                    break;
                case Failsafe::Critical:
                    action = _params[i]._failsafe_critical_action;
                    type_str = "critical";
                    break;
            }

            GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Battery %d is %s %.2fV used %.0f mAh", i + 1, type_str,
                            (double)voltage(i), (double)state[i].consumed_mah);
            _has_triggered_failsafe = true;
#ifndef HAL_BUILD_AP_PERIPH
            AP_Notify::flags.failsafe_battery = true;
#endif
            state[i].failsafe = type;

            // map the desired failsafe action to a priority level
            int8_t priority = 0;
            if (_failsafe_priorities != nullptr) {
                while (_failsafe_priorities[priority] != -1) {
                    if (_failsafe_priorities[priority] == action) {
                        break;
                    }
                    priority++;
                }

            }

            // trigger failsafe if the action was equal or higher priority
            // It's valid to retrigger the same action if a different battery provoked the event
            if (priority <= _highest_failsafe_priority) {
                _battery_failsafe_handler_fn(type_str, action);
                _highest_failsafe_priority = priority;
            }
        }
    }
}

// return true if any battery is pushing too much power
bool AP_BattMonitor::overpower_detected() const
{
#if AP_BATTERY_WATT_MAX_ENABLED && APM_BUILD_TYPE(APM_BUILD_ArduPlane)
    for (uint8_t instance = 0; instance < _num_instances; instance++) {
        if (overpower_detected(instance)) {
            return true;
        }
    }
#endif
    return false;
}

bool AP_BattMonitor::overpower_detected(uint8_t instance) const
{
#if AP_BATTERY_WATT_MAX_ENABLED && APM_BUILD_TYPE(APM_BUILD_ArduPlane)
    if (instance < _num_instances && _params[instance]._watt_max > 0) {
        const float power = state[instance].current_amps * state[instance].voltage;
        return state[instance].healthy && (power > _params[instance]._watt_max);
    }
#endif
    return false;
}

bool AP_BattMonitor::has_cell_voltages(const uint8_t instance) const
{
    if (instance < _num_instances && drivers[instance] != nullptr) {
        return drivers[instance]->has_cell_voltages();
    }

    return false;
}

// return the current cell voltages, returns the first monitor instances cells if the instance is out of range
const AP_BattMonitor::cells & AP_BattMonitor::get_cell_voltages(const uint8_t instance) const
{
    if (instance >= AP_BATT_MONITOR_MAX_INSTANCES) {
        return state[AP_BATT_PRIMARY_INSTANCE].cell_voltages;
    } else {
        return state[instance].cell_voltages;
    }
}

// get once cell voltage (for scripting)
bool AP_BattMonitor::get_cell_voltage(uint8_t instance, uint8_t cell, float &voltage) const
{
    if (!has_cell_voltages(instance) ||
        cell >= AP_BATT_MONITOR_CELLS_MAX) {
        return false;
    }
    const auto &cell_voltages = get_cell_voltages(instance);
    const uint16_t voltage_mv = cell_voltages.cells[cell];
    if (voltage_mv == 0 || voltage_mv == UINT16_MAX) {
        // UINT16_MAX is used as invalid indicator
        return false;
    }
    voltage = voltage_mv*0.001;
    return true;
}

// returns true if there is a temperature reading
bool AP_BattMonitor::get_temperature(float &temperature, const uint8_t instance) const
{
    if (instance >= _num_instances || drivers[instance] == nullptr) {
        return false;
    }

    return drivers[instance]->get_temperature(temperature);
}

#if AP_TEMPERATURE_SENSOR_ENABLED
// return true when successfully setting a battery temperature from an external source by instance
bool AP_BattMonitor::set_temperature(const float temperature, const uint8_t instance)
{
    if (instance >= _num_instances || drivers[instance] == nullptr) {
        return false;
    }
    state[instance].temperature_external = temperature;
    state[instance].temperature_external_use = true;
    return true;
}

// return true when successfully setting a battery temperature from an external source by serial_number
bool AP_BattMonitor::set_temperature_by_serial_number(const float temperature, const int32_t serial_number)
{
    bool success = false;
    for (uint8_t i = 0; i < _num_instances; i++) {
        if (drivers[i] != nullptr && get_serial_number(i) == serial_number) {
            success |= set_temperature(temperature, i);
        }
    }
    return success;
}
#endif // AP_TEMPERATURE_SENSOR_ENABLED

// return true if cycle count can be provided and fills in cycles argument
bool AP_BattMonitor::get_cycle_count(uint8_t instance, uint16_t &cycles) const
{
    if (instance >= _num_instances || (drivers[instance] == nullptr)) {
        return false;
    }
    return drivers[instance]->get_cycle_count(cycles);
}

bool AP_BattMonitor::arming_checks(size_t buflen, char *buffer) const
{
    char temp_buffer[MAVLINK_MSG_STATUSTEXT_FIELD_TEXT_LEN+1] {};

    for (uint8_t i = 0; i < AP_BATT_MONITOR_MAX_INSTANCES; i++) {
        const auto expected_type = configured_type(i);

        if (drivers[i] == nullptr && expected_type == Type::NONE) {
            continue;
        }

#if !AP_BATTERY_SUM_ENABLED
        // CONVERSION - Added Sep 2024 for ArduPilot 4.6 as we are
        // removing the SUM backend on 1MB boards.  Give a
        // more-specific error for the sum backend:
        if (expected_type == Type::Sum) {
            hal.util->snprintf(buffer, buflen, "Battery %d %s", i + 1, "feature BATTERY_SUM not available");
            return false;
        }
#endif

        if (drivers[i] == nullptr || allocated_type(i) != expected_type) {
            hal.util->snprintf(buffer, buflen, "Battery %d %s", i + 1, "unhealthy");
            return false;
        }

        if (!drivers[i]->arming_checks(temp_buffer, sizeof(temp_buffer))) {
            hal.util->snprintf(buffer, buflen, "Battery %d %s", i + 1, temp_buffer);
            return false;
        }
    }

    return true;
}

// Check's each smart battery instance for its powering off state and broadcasts notifications
void AP_BattMonitor::checkPoweringOff(void)
{
    for (uint8_t i = 0; i < _num_instances; i++) {
        if (state[i].is_powering_off && !state[i].powerOffNotified) {
#ifndef HAL_BUILD_AP_PERIPH
            // Set the AP_Notify flag, which plays the power off tones
            AP_Notify::flags.powering_off = true;
#endif

            // Send a Mavlink broadcast announcing the shutdown
#if HAL_GCS_ENABLED
            mavlink_command_long_t cmd_msg{};
            cmd_msg.command = MAV_CMD_POWER_OFF_INITIATED;
            cmd_msg.param1 = i+1;
            GCS_MAVLINK::send_to_components(MAVLINK_MSG_ID_COMMAND_LONG, (char*)&cmd_msg, sizeof(cmd_msg));
            GCS_SEND_TEXT(MAV_SEVERITY_WARNING, "Vehicle %d battery %d is powering off", mavlink_system.sysid, i+1);
#endif

            // only send this once
            state[i].powerOffNotified = true;
        }
    }
}

/*
  reset battery remaining percentage for batteries that integrate to
  calculate percentage remaining
*/
bool AP_BattMonitor::reset_remaining_mask(uint16_t battery_mask, float percentage)
{
    static_assert(AP_BATT_MONITOR_MAX_INSTANCES <= 16, "More batteries are enabled then can be reset");
    bool ret = true;
    Failsafe highest_failsafe = Failsafe::None;
    for (uint8_t i = 0; i < _num_instances; i++) {
        if ((1U<<i) & battery_mask) {
            if (drivers[i] != nullptr) {
                ret &= drivers[i]->reset_remaining(percentage);
            } else {
                ret = false;
            }
        }
        if (state[i].failsafe > highest_failsafe) {
            highest_failsafe = state[i].failsafe;
        }
    }

    // If all backends are not in failsafe then set overall failsafe state
    if (highest_failsafe == Failsafe::None) {
        _highest_failsafe_priority = INT8_MAX;
        _has_triggered_failsafe = false;
        // and reset notify flag
        AP_Notify::flags.failsafe_battery = false;
    }
    return ret;
}

// Returns the mavlink charge state. The following mavlink charge states are not used
// MAV_BATTERY_CHARGE_STATE_EMERGENCY , MAV_BATTERY_CHARGE_STATE_FAILED
// MAV_BATTERY_CHARGE_STATE_UNHEALTHY, MAV_BATTERY_CHARGE_STATE_CHARGING
MAV_BATTERY_CHARGE_STATE AP_BattMonitor::get_mavlink_charge_state(const uint8_t instance) const 
{
    if (instance >= _num_instances) {
        return MAV_BATTERY_CHARGE_STATE_UNDEFINED;
    }

    switch (state[instance].failsafe) {

    case Failsafe::None:
    case Failsafe::Unhealthy:
        if (get_mavlink_fault_bitmask(instance) != 0 || !healthy()) {
            return MAV_BATTERY_CHARGE_STATE_UNHEALTHY;
        }
        return MAV_BATTERY_CHARGE_STATE_OK;

    case Failsafe::Low:
        return MAV_BATTERY_CHARGE_STATE_LOW;

    case Failsafe::Critical:
        return MAV_BATTERY_CHARGE_STATE_CRITICAL;
    }

    // Should not reach this
    return MAV_BATTERY_CHARGE_STATE_UNDEFINED;
}

// Returns mavlink fault state
uint32_t AP_BattMonitor::get_mavlink_fault_bitmask(const uint8_t instance) const
{
    if (instance >= _num_instances || drivers[instance] == nullptr) {
        return 0;
    }
    return drivers[instance]->get_mavlink_fault_bitmask();
}

// return true if state of health (as a percentage) can be provided and fills in soh_pct argument
bool AP_BattMonitor::get_state_of_health_pct(uint8_t instance, uint8_t &soh_pct) const
{
    if (instance >= _num_instances || drivers[instance] == nullptr) {
        return false;
    }
    return drivers[instance]->get_state_of_health_pct(soh_pct);
}

// Enable/Disable (Turn on/off) MPPT power to all backends who are MPPTs
void AP_BattMonitor::MPPT_set_powered_state_to_all(const bool power_on)
{
    for (uint8_t i=0; i < _num_instances; i++) {
        MPPT_set_powered_state(i, power_on);
    }
}

// Enable/Disable (Turn on/off) MPPT power. When disabled, the MPPT does not
// supply energy to the system regardless if it's capable to or not. When enabled
// it will supply energy if available.
void AP_BattMonitor::MPPT_set_powered_state(const uint8_t instance, const bool power_on)
{
    if (instance < _num_instances) {
        drivers[instance]->mppt_set_powered_state(power_on);
    }
}

/*
  check that all configured battery monitors are healthy
 */
bool AP_BattMonitor::healthy() const
{
    for (uint8_t i=0; i< _num_instances; i++) {
        if (allocated_type(i) != configured_type(i)) {
            return false;
        }
        // allow run-time disabling; this is technically redundant
        if (configured_type(i) == Type::NONE) {
            continue;
        }
        if (!healthy(i)) {
            return false;
        }
    }
    return true;
}

#if AP_BATTERY_SCRIPTING_ENABLED
/*
  handle state update from a lua script
 */
bool AP_BattMonitor::handle_scripting(uint8_t idx, const BattMonitorScript_State &_state)
{
    if (idx >= _num_instances) {
        return false;
    }
    if (drivers[idx] == nullptr) {
        return false;
    }
    return drivers[idx]->handle_scripting(_state);
}
#endif

namespace AP {

AP_BattMonitor &battery()
{
    return *AP_BattMonitor::get_singleton();
}

};

#endif  // AP_BATTERY_ENABLED
