/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ULP RISC-V RTC I2C example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.

   This code runs on ULP RISC-V coprocessor
*/

#include <stdint.h>
#include "ulp_riscv.h"
#include "ulp_riscv_utils.h"
#include "ulp_riscv_i2c_ulp_core.h"
#include "../tsl2561_defs.h"

/************************************************
 * Shared data between main CPU and ULP
 ************************************************/

uint32_t visibleLight = 0;
uint32_t infraredLight = 0;

int main (void)
{
    /* Read TSL2561 Data */
    tsl2561_read_data();

    /* Wakeup the main CPU if either the uncompensated visibleLight values
     * are more than their respective threshold values.
     */
    if ( visibleLight > EXAMPLE_UV_THRESHOLD )
    {
        ulp_riscv_wakeup_main_processor();
    }
    
    return 0;
}

static void tsl2561_read_data(void)
{
    uint8_t lowByte = 0;
    uint8_t highByte = 0;
    ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_READ_CHANNEL0_DATA_LOW_CMD);
    ulp_riscv_i2c_master_read_from_device(&lowByte, 1);

    ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_READ_CHANNEL0_DATA_HIGH_CMD);
    ulp_riscv_i2c_master_read_from_device(&highByte, 1);
    visibleLight = lowByte + highByte * 256;

    ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_READ_CHANNEL1_DATA_LOW_CMD);
    ulp_riscv_i2c_master_read_from_device(&lowByte, 1);

    ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_READ_CHANNEL1_DATA_HIGH_CMD);
    ulp_riscv_i2c_master_read_from_device(&highByte, 1);
    infraredLight = lowByte + highByte * 256;
}
