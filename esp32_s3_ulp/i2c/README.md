| Supported Targets | ESP32-S2 | ESP32-S3 |
| ----------------- | -------- | -------- |

# ULP RISC-V I2C Example

This example demonstrates how to use the RTC I2C peripheral from the ULP RISC-V coprocessor in deep sleep.

The ULP program is based on the TSL2561 light sensor which has an I2C interface. The main CPU initializes the RTC I2C peripheral, the TSL2561 sensor and loads the ULP program. It then goes into deep sleep.

The ULP program periodically measures the visible Light and infrared Light values from the TSL2561 sensor and wakes up the main CPU when the values are above a certain threshold.
### Hardware Required

* A development board with a SOC which has a RISC-V ULP coprocessor (e.g., ESP32-S2 Saola)
* A TSL2561 sensor module
* A USB cable for power supply and programming

## Example output

Below is the output from this example.

```
ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x1 (POWERON),boot:0x29 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3818,len:0x1260
load:0x403c9700,len:0x4
load:0x403c9704,len:0xa64
load:0x403cc700,len:0x2c14
entry 0x403c9894
W (89) spi_flash: Detected size(8192k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
Not a ULP-RISC V wakeup (cause = 0)
Initializing RTC I2C ...
TSL2561 data: visibleLight = 329, infraredLight = 47
Entering deep sleep

ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x5 (DSLEEP),boot:0x29 (SPI_FAST_FLASH_BOOT)
pro cpu reset by JTAG
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3818,len:0x1260
load:0x403c9700,len:0x4
load:0x403c9704,len:0xa64
load:0x403cc700,len:0x2c14
entry 0x403c9894
W (91) spi_flash: Detected size(8192k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
ULP RISC-V woke up the main CPU
Uncompensated data: visibleLight = 1098, infraredLight = 131
Entering deep sleep

ESP-ROM:esp32s3-20210327
Build:Mar 27 2021
rst:0x5 (DSLEEP),boot:0x29 (SPI_FAST_FLASH_BOOT)
SPIWP:0xee
mode:DIO, clock div:1
load:0x3fce3818,len:0x1260
load:0x403c9700,len:0x4
load:0x403c9704,len:0xa64
load:0x403cc700,len:0x2c14
entry 0x403c9894
W (89) spi_flash: Detected size(8192k) larger than the size in the binary image header(2048k). Using the size in the binary image header.
ULP RISC-V woke up the main CPU
Uncompensated data: visibleLight = 1121, infraredLight = 136
Entering deep sleep
```
