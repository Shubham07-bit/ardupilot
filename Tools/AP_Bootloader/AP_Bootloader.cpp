/*
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
/*
  ArduPilot bootloader. This implements the same protocol originally
  developed for PX4, but builds on top of the ChibiOS HAL

  It does not use the full AP_HAL API in order to keep the firmware
  size below the maximum of 16kByte required for F4 based
  boards. Instead it uses the ChibiOS APIs directly
 */

#include <AP_HAL/AP_HAL.h>
#include "ch.h"
#include "hal.h"
#include "hwdef.h"
#include <AP_HAL_ChibiOS/hwdef/common/usbcfg.h>
#include <AP_HAL_ChibiOS/hwdef/common/stm32_util.h>
#include <AP_HAL_ChibiOS/hwdef/common/watchdog.h>
#include "support.h"
#include "bl_protocol.h"
#include "flash_from_sd.h"
#include "can.h"
#include <stdio.h>
#if EXT_FLASH_SIZE_MB
#include <AP_FlashIface/AP_FlashIface_JEDEC.h>
#endif
#include <AP_CheckFirmware/AP_CheckFirmware.h>
#include "network.h"

#include <AP_HAL_ChibiOS/sdcard.h>

extern "C" {
    int main(void);
}

struct boardinfo board_info = {
    .board_type = APJ_BOARD_ID,
    .board_rev = 0,
    .fw_size = (BOARD_FLASH_SIZE - (FLASH_BOOTLOADER_LOAD_KB + FLASH_RESERVE_END_KB + APP_START_OFFSET_KB))*1024,
    .extf_size = (EXT_FLASH_SIZE_MB * 1024 * 1024) - (EXT_FLASH_RESERVE_START_KB + EXT_FLASH_RESERVE_END_KB) * 1024
};

#ifndef HAL_BOOTLOADER_TIMEOUT
#define HAL_BOOTLOADER_TIMEOUT 5000
#endif

#ifndef HAL_STAY_IN_BOOTLOADER_VALUE
#define HAL_STAY_IN_BOOTLOADER_VALUE 0
#endif

#if EXT_FLASH_SIZE_MB
AP_FlashIface_JEDEC ext_flash;
#endif

#if AP_BOOTLOADER_NETWORK_ENABLED
static BL_Network network;
#endif

int main(void)
{
#ifdef AP_BOOTLOADER_CUSTOM_HERE4
    custom_startup();
#endif

    flash_init();

#if defined(STM32H7) && CH_CFG_USE_HEAP
    check_ecc_errors();
#endif
    
#ifdef STM32F427xx
    if (BOARD_FLASH_SIZE > 1024 && check_limit_flash_1M()) {
        board_info.fw_size = (1024 - (FLASH_BOOTLOADER_LOAD_KB + APP_START_OFFSET_KB))*1024;
    }
#endif

    bool try_boot = false;
    uint32_t timeout = HAL_BOOTLOADER_TIMEOUT;

#ifdef HAL_BOARD_AP_PERIPH_ZUBAXGNSS
    // setup remapping register for ZubaxGNSS
    uint32_t mapr = AFIO->MAPR;
    mapr &= ~AFIO_MAPR_SWJ_CFG;
    mapr |= AFIO_MAPR_SWJ_CFG_JTAGDISABLE;
    AFIO->MAPR = mapr | AFIO_MAPR_CAN_REMAP_REMAP2 | AFIO_MAPR_SPI3_REMAP;
#endif

#if HAL_FLASH_PROTECTION
    stm32_flash_unprotect_flash();
#endif

#if AP_BOOTLOADER_NETWORK_ENABLED
    network.save_comms_ip();
#endif

#if AP_FASTBOOT_ENABLED
    enum rtc_boot_magic m = check_fast_reboot();
    bool was_watchdog = stm32_was_watchdog_reset();
    if (was_watchdog) {
        try_boot = true;
        timeout = 0;
    } else if (m == RTC_BOOT_HOLD) {
        timeout = 0;
    } else if (m == RTC_BOOT_FAST) {
        try_boot = true;
        timeout = 0;
    }
#if HAL_USE_CAN == TRUE || HAL_NUM_CAN_IFACES
    else if ((m & 0xFFFFFF00) == RTC_BOOT_CANBL) {
        try_boot = false;
        timeout = 10000;
        can_set_node_id(m & 0xFF);
    }
    if (can_check_update()) {
        // trying to update firmware, stay in bootloader
        try_boot = false;
        timeout = 0;
    }
#if AP_CHECK_FIRMWARE_ENABLED
    const auto ok = check_good_firmware();
    if (ok != check_fw_result_t::CHECK_FW_OK) {
        // bad firmware CRC, don't try and boot
        timeout = 0;
        try_boot = false;
        led_set(LED_BAD_FW);
        // log to SD card
    }
#if AP_BOOTLOADER_NETWORK_ENABLED
    if (ok == check_fw_result_t::CHECK_FW_OK) {
        const auto *app_descriptor = get_app_descriptor();
        if (app_descriptor != nullptr) {
            network.status_printf("Firmware OK: %ld.%ld.%lx\n", app_descriptor->version_major,
                                                                app_descriptor->version_minor,
                                                                app_descriptor->git_hash);
        }
    } else {
        network.status_printf("Firmware Error: %d\n", (int)ok);
    }
#endif
#endif  // AP_CHECK_FIRMWARE_ENABLED
#ifndef BOOTLOADER_DEV_LIST
    else if (timeout == HAL_BOOTLOADER_TIMEOUT) {
        // fast boot for good firmware if we haven't been told to stay
        // in bootloader
        try_boot = true;
        timeout = 1000;
    }
#endif
    if (was_watchdog && m != RTC_BOOT_FWOK) {
        // we've had a watchdog within 30s of booting main CAN
        // firmware. We will stay in bootloader to allow the user to
        // load a fixed firmware
        stm32_watchdog_clear_reason();
        try_boot = false;
        timeout = 0;
    }
#elif AP_CHECK_FIRMWARE_ENABLED
    const auto ok = check_good_firmware();
    if (ok != check_fw_result_t::CHECK_FW_OK) {
        // bad firmware, don't try and boot
        timeout = 0;
        try_boot = false;
        led_set(LED_BAD_FW);
    }
#endif

#if defined(HAL_GPIO_PIN_VBUS) && defined(HAL_ENABLE_VBUS_CHECK)
#if HAL_USE_SERIAL_USB == TRUE
    else if (palReadLine(HAL_GPIO_PIN_VBUS) == 0)  {
        try_boot = true;
        timeout = 0;
    }
#endif
#endif

    // if we fail to boot properly we want to pause in bootloader to give
    // a chance to load new app code
    set_fast_reboot(RTC_BOOT_OFF);
#endif  // AP_FASTBOOT_ENABLED

#ifdef HAL_GPIO_PIN_STAY_IN_BOOTLOADER
    // optional "stay in bootloader" pin
    if (palReadLine(HAL_GPIO_PIN_STAY_IN_BOOTLOADER) == HAL_STAY_IN_BOOTLOADER_VALUE) {
        try_boot = false;
        timeout = 0;
    }
#endif

#if EXT_FLASH_SIZE_MB
    while (!ext_flash.init()) {
        // keep trying until we get it working
        // there's no future without it
        chThdSleep(chTimeMS2I(20));
    }
#endif

    if (try_boot) {
        jump_to_app();
    }

#if defined(BOOTLOADER_DEV_LIST)
    init_uarts();
#endif
#if HAL_USE_CAN == TRUE || HAL_NUM_CAN_IFACES
    can_start();
#endif

#if AP_BOOTLOADER_NETWORK_ENABLED
    network.init();
#endif

#if AP_BOOTLOADER_FLASH_FROM_SD_ENABLED
    if (flash_from_sd()) {
        jump_to_app();
    }
#endif

#if defined(BOOTLOADER_DEV_LIST)
    while (true) {
        bootloader(timeout);
        jump_to_app();
    }
#else
    // CAN and network only
    while (true) {
        uint32_t t0 = AP_HAL::millis();
        while (timeout == 0 || AP_HAL::millis() - t0 <= timeout) {
            can_update();
            chThdSleep(chTimeMS2I(1));
        }
        jump_to_app();
    }
#endif
    peripheral_power_enable();
    if (sdcard_init()) {
        if (ok != check_fw_result_t::CHECK_FW_OK) {
            sdcard_append_counted_message("/bootlog.txt", "BAD FIRMWARE: CRC or signature check failed");
        } else {
            sdcard_append_counted_message("/bootlog.txt", "GOOD FIRMWARE: CRC and signature OK");
        }
        sdcard_stop();
    }
}


