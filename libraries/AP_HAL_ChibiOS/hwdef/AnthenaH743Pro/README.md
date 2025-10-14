# ANTHENA-H743 PRO Flight Controller

The ANTHENA-H743 PRO is a flight controller based on the STM32H743 microcontroller.

## Features

* STM32H743 microcontroller
* BMI088 IMU
* MS5611 barometer
* Built-in safety switch
* microSD card slot
* 8 PWM outputs
* Multiple UARTs, I2C, and SPI ports
* USB-C connector
* CAN bus port
* Integrated current sensor
* External FRAM storage
* RGB LED status indicator

## UART Mapping

The UARTs are marked Rn and Tn in the board, where n is the UART number.
The rx/tx pins for each uart are shown below.

* UART1 -> USB (Primary)
* UART2 -> TELEM1 (DMA-enabled)
* UART3 -> GPS3
* UART4 -> GPS1
* UART5 -> SPARE
* UART6 -> TELEM2
* UART7 -> DEBUG
* UART8 -> GPS2

## RC Input
 
RC input is configured on the PPM pin by default.

## PWM Output

The ANTHENA-H743 PRO supports up to 8 PWM outputs. The PWM outputs are available on the PWM output connectors.

The PWM is in 4 groups:

* PWM 1-4: Group 1 - Timer 2
* PWM 5-8: Group 2 - Timer 4 

## Building Firmware

To build for this target, use:

```bash
./waf configure --board AnthenaH743Pro
./waf copter    # for quadcopter
./waf plane     # for plane
./waf rover     # for rover
```

The bootloader can be built with:

```bash
./waf bootloader --board AnthenaH743Pro
```

## Loading Firmware

The board comes pre-installed with a bootloader. The firmware can be loaded using any ArduPilot compatible ground station software.

Initial firmware load can be done with DFU by plugging in USB with the boot button pressed. 

## Configuration

The default parameter file for this board can be found in Tools/Frame_params/

Please refer to the ArduPilot documentation for configuration and usage instructions.