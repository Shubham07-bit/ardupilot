
/*
 * This file is free software: you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This file is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
 * See the GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include <hal.h>
#include "SPIDevice.h"
#include "sdcard.h"
#include "bouncebuffer.h"
#include "hwdef/common/spi_hook.h"
#include <AP_BoardConfig/AP_BoardConfig.h>
#include <AP_Filesystem/AP_Filesystem.h>
#include "bouncebuffer.h"
#include "stm32_util.h"

extern const AP_HAL::HAL& hal;

#if HAL_USE_FATFS
static FATFS SDC_FS; // FATFS object
#ifndef HAL_BOOTLOADER_BUILD
static HAL_Semaphore sem;
#endif
static bool sdcard_running;
#endif

#if HAL_USE_SDC
static SDCConfig sdcconfig = {
  SDC_MODE_4BIT,
  0
};
#elif HAL_USE_MMC_SPI
MMCDriver MMCD1;
static AP_HAL::SPIDevice *device;
static MMCConfig mmcconfig;
static SPIConfig lowspeed;
static SPIConfig highspeed;
#endif


// Append content to a file, prefixing with a count (line number). Returns new count or -1 on failure
int sdcard_append_counted_message(const char *filename, const char *content)
{
#if HAL_USE_FATFS
    FIL fh;
    UINT bw = 0;
    int count = 1;
    // Try to open file for reading to count lines
    if (f_open(&fh, filename, FA_READ) == FR_OK) {
        char buf[128];
        UINT br;
        while (f_read(&fh, buf, sizeof(buf), &br) == FR_OK && br > 0) {
            for (UINT i = 0; i < br; i++) {
                if (buf[i] == '\n') count++;
            }
        }
        f_close(&fh);
    }
    // Open for append
    if (f_open(&fh, filename, FA_WRITE | FA_OPEN_ALWAYS) == FR_OK) {
        f_lseek(&fh, f_size(&fh));
        char line[256];
        int n = snprintf(line, sizeof(line), "%d: %s\n", count, content);
        if (f_write(&fh, line, n, &bw) == FR_OK) {
            f_close(&fh);
            return count;
        }
        f_close(&fh);
    }
#endif
    return -1;
}
/*
  initialise microSD card if avaialble. This is called during
  AP_BoardConfig initialisation. The parameter BRD_SD_SLOWDOWN
  controls a scaling factor on the microSD clock
 */
bool sdcard_init()
{
#if HAL_USE_FATFS
#ifndef HAL_BOOTLOADER_BUILD
    WITH_SEMAPHORE(sem);

    uint8_t sd_slowdown = AP_BoardConfig::get_sdcard_slowdown();
#else
    uint8_t sd_slowdown = 0;  // maybe take from a define?
#endif
#if HAL_USE_SDC

#if STM32_SDC_USE_SDMMC2 == TRUE
    auto &sdcd = SDCD2;
#else
    auto &sdcd = SDCD1;
#endif

    if (sdcd.bouncebuffer == nullptr) {
        // allocate 4k bouncebuffer for microSD to match size in
        // AP_Logger
#if defined(STM32H7)
        bouncebuffer_init(&sdcd.bouncebuffer, 4096, true);
#else
        bouncebuffer_init(&sdcd.bouncebuffer, 4096, false);
#endif
    }

    if (sdcard_running) {
        sdcard_stop();
    }

    const uint8_t tries = 3;
    for (uint8_t i=0; i<tries; i++) {
        sdcconfig.slowdown = sd_slowdown;
        sdcStart(&sdcd, &sdcconfig);
        if(sdcConnect(&sdcd) == HAL_FAILED) {
            sdcStop(&sdcd);
            continue;
        }
        if (f_mount(&SDC_FS, "/", 1) != FR_OK) {
            sdcDisconnect(&sdcd);
            sdcStop(&sdcd);
            continue;
        }
        printf("Successfully mounted SDCard (slowdown=%u)\n", (unsigned)sd_slowdown);

        sdcard_running = true;
        // Write a test file after successful mount
        // sdcard_append_counted_message("/writetest.txt", "write test started successfully!");
        return true;
    }
#elif HAL_USE_MMC_SPI
    if (MMCD1.buffer == nullptr) {
        // allocate 16 byte non-cacheable buffer for microSD
        MMCD1.buffer = (uint8_t*)malloc_axi_sram(MMC_BUFFER_SIZE);
    }

    if (sdcard_running) {
        sdcard_stop();
    }

    sdcard_running = true;

    if (device == nullptr) {
        device = AP_HAL::get_HAL().spi->get_device_ptr("sdcard");
        if (!device) {
            printf("No sdcard SPI device found\n");
            sdcard_running = false;
            return false;
        }
    }
    device->set_slowdown(sd_slowdown);

    mmcObjectInit(&MMCD1, MMCD1.buffer);

    mmcconfig.spip = (static_cast<ChibiOS::SPIDevice*>(device))->get_driver();
    mmcconfig.hscfg = &highspeed;
    mmcconfig.lscfg = &lowspeed;

    /*
      try up to 3 times to init microSD interface
     */
    const uint8_t tries = 3;
    for (uint8_t i=0; i<tries; i++) {
        mmcStart(&MMCD1, &mmcconfig);

        if (mmcConnect(&MMCD1) == HAL_FAILED) {
            mmcStop(&MMCD1);
            continue;
        }
        if (f_mount(&SDC_FS, "/", 1) != FR_OK) {
            mmcDisconnect(&MMCD1);
            mmcStop(&MMCD1);
            continue;
        }
        printf("Successfully mounted SDCard (slowdown=%u)\n", (unsigned)sd_slowdown);
        // Write a test file after successful mount
        sdcard_append_counted_message("/writetest.txt", "Bootloader started successfully!");
        return true;
    }
#endif
    sdcard_running = false;
#endif  // HAL_USE_FATFS
    return false;
}

/*
  stop sdcard interface (for reboot)
 */
void sdcard_stop(void)
{
#if HAL_USE_FATFS
    // unmount
    f_mount(nullptr, "/", 1);
#endif
#if HAL_USE_SDC
#if STM32_SDC_USE_SDMMC2 == TRUE
    auto &sdcd = SDCD2;
#else
    auto &sdcd = SDCD1;
#endif
    if (sdcard_running) {
        sdcDisconnect(&sdcd);
        sdcStop(&sdcd);
        sdcard_running = false;
    }
#elif HAL_USE_MMC_SPI
    if (sdcard_running) {
        mmcDisconnect(&MMCD1);
        mmcStop(&MMCD1);
        sdcard_running = false;
    }
#endif
}

bool sdcard_retry(void)
{
#if HAL_USE_FATFS
    if (!sdcard_running) {
        if (sdcard_init()) {
#if AP_FILESYSTEM_FILE_WRITING_ENABLED
            // create APM directory
            AP::FS().mkdir("/APM");
#endif
        }
    }
    return sdcard_running;
#endif
    return false;
}

#if HAL_USE_MMC_SPI

/*
  hooks to allow hal_mmc_spi.c to work with HAL_ChibiOS SPI
  layer. This provides bounce buffers for DMA, DMA channel sharing and
  bus locking
 */

void spiStartHook(SPIDriver *spip, const SPIConfig *config)
{
    device->set_speed(config == &lowspeed ?
        AP_HAL::Device::SPEED_LOW : AP_HAL::Device::SPEED_HIGH);
}

void spiStopHook(SPIDriver *spip)
{
}

__RAMFUNC__ void spiAcquireBusHook(SPIDriver *spip)
{
    if (sdcard_running) {
        ChibiOS::SPIDevice *devptr = static_cast<ChibiOS::SPIDevice*>(device);
        devptr->acquire_bus(true, true);
    }
}

__RAMFUNC__ void spiReleaseBusHook(SPIDriver *spip)
{
    if (sdcard_running) {
        ChibiOS::SPIDevice *devptr = static_cast<ChibiOS::SPIDevice*>(device);
        devptr->acquire_bus(false, true);
    }
}

__RAMFUNC__ void spiSelectHook(SPIDriver *spip)
{
    if (sdcard_running) {
        device->get_semaphore()->take_blocking();
        device->set_chip_select(true);
    }
}

__RAMFUNC__ void spiUnselectHook(SPIDriver *spip)
{
    if (sdcard_running) {
        device->set_chip_select(false);
        device->get_semaphore()->give();
    }
}

void spiIgnoreHook(SPIDriver *spip, size_t n)
{
    if (sdcard_running) {
        device->clock_pulse(n);
    }
}

__RAMFUNC__ void spiSendHook(SPIDriver *spip, size_t n, const void *txbuf)
{
    if (sdcard_running) {
        device->transfer((const uint8_t *)txbuf, n, nullptr, 0);
    }
}

__RAMFUNC__ void spiReceiveHook(SPIDriver *spip, size_t n, void *rxbuf)
{
    if (sdcard_running) {
        device->transfer(nullptr, 0, (uint8_t *)rxbuf, n);
    }
}

#endif
