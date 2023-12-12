/*
 * SPDX-FileCopyrightText: 2022-2023 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* ULP RISC-V RTC I2C example

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

#include <stdio.h>
#include <inttypes.h>
#include <math.h>
#include "esp_sleep.h"
#include "ulp_riscv.h"
#include "ulp_riscv_i2c.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "ulp_main.h"
#include "tsl2561_defs.h"

extern const uint8_t ulp_main_bin_start[] asm("_binary_ulp_main_bin_start");
extern const uint8_t ulp_main_bin_end[]   asm("_binary_ulp_main_bin_end");

uint32_t visibleLight = 0;
uint32_t infraredLight = 0;

/************************************************
 * ULP utility APIs
 ************************************************/
static void init_ulp_program(void);

/************************************************
 * RTC I2C utility APIs
 ************************************************/
static void init_i2c(void);
static void tsl2561_power_on(void);
static void tsl2561_power_off(void);
static void tsl2561_read_data(void);

void app_main(void)
{
    uint8_t data_rd = 0;

    esp_sleep_wakeup_cause_t cause = esp_sleep_get_wakeup_cause();

    /* Not a wakeup from ULP
     * Initialize RTC I2C
     * Setup TSL2561 sensor
     * Store current temperature and pressure values
     * Load the ULP firmware
     * Go to deep sleep
     */
    if (cause != ESP_SLEEP_WAKEUP_ULP) {
        printf("Not a ULP-RISC V wakeup (cause = %d)\n", cause);

        /* Initialize RTC I2C */
        init_i2c();

        tsl2561_power_on();

        ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_REG_ADDR_WHO_AM_I);
        ulp_riscv_i2c_master_read_from_device(&data_rd, 1);
        printf("ID = %u\n", data_rd);

        vTaskDelay(pdMS_TO_TICKS(400));
        
        while (1) {
            tsl2561_read_data();
            printf("TSL2561 data: visibleLight = %"PRIu32", infraredLight = %"PRIu32"\n", visibleLight, infraredLight);
            vTaskDelay(pdMS_TO_TICKS(10));
        }

        tsl2561_read_data();
        printf("TSL2561 data: visibleLight = %"PRIu32", infraredLight = %"PRIu32"\n", visibleLight, infraredLight);
        

        /* Load ULP firmware
         *
         * The ULP is responsible of monitoring the temperature and pressure values
         * periodically. It will wakeup the main CPU if the temperature and pressure
         * values are above a certain threshold.
         */
        init_ulp_program();
    }

    /* ULP RISC-V read and detected a temperature or pressure above the limit */
    if (cause == ESP_SLEEP_WAKEUP_ULP) {
        printf("ULP RISC-V woke up the main CPU\n");

        /* Pause ULP while we are using the RTC I2C from the main CPU */
        ulp_timer_stop();
        ulp_riscv_halt();

        printf("Uncompensated data: visibleLight = %"PRIu32", infraredLight = %"PRIu32"\n", ulp_visibleLight, ulp_infraredLight);

        /* Resume ULP and go to deep sleep again */
        ulp_timer_resume();
    }


    /* Add a delay for everything to the printed before heading in to deep sleep */
    vTaskDelay(100);

    /* Go back to sleep, only the ULP RISC-V will run */
    printf("Entering deep sleep\n\n");

    /* RTC peripheral power domain needs to be kept on to keep RTC I2C related configs during sleep */
    esp_sleep_pd_config(ESP_PD_DOMAIN_RTC_PERIPH, ESP_PD_OPTION_ON);

    ESP_ERROR_CHECK(esp_sleep_enable_ulp_wakeup());

    esp_deep_sleep_start();
}

static void init_i2c(void)
{
    /* Configure RTC I2C */
    printf("Initializing RTC I2C ...\n");
    ulp_riscv_i2c_cfg_t i2c_cfg = ULP_RISCV_I2C_DEFAULT_CONFIG();
    esp_err_t ret = ulp_riscv_i2c_master_init(&i2c_cfg);
    if (ret!= ESP_OK) {
        printf("ERROR: Failed to initialize RTC I2C. Aborting...\n");
        abort();
    }
}

static void tsl2561_power_on(void)
{
    ulp_riscv_i2c_master_set_slave_addr(TSL2561_SENSOR_I2C_ADDR);
    ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_I2C_ADDR);
    uint8_t data_w = TSL2561_SENSOR_POWER_ON_CMD;
    ulp_riscv_i2c_master_write_to_device(&data_w, 1);
}

static void tsl2561_power_off(void)
{
    ulp_riscv_i2c_master_set_slave_addr(TSL2561_SENSOR_I2C_ADDR);
    ulp_riscv_i2c_master_set_slave_reg_addr(TSL2561_SENSOR_I2C_ADDR);
    uint8_t data_w = TSL2561_SENSOR_POWER_OFF_CMD;
    ulp_riscv_i2c_master_write_to_device(&data_w, 1);
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

static void init_ulp_program(void)
{
    esp_err_t err = ulp_riscv_load_binary(ulp_main_bin_start, (ulp_main_bin_end - ulp_main_bin_start));
    ESP_ERROR_CHECK(err);

    /* The first argument is the period index, which is not used by the ULP-RISC-V timer
     * The second argument is the period in microseconds, which gives a wakeup time period of: 40ms
     */
    ulp_set_wakeup_period(0, 500000);

    /* Start the program */
    err = ulp_riscv_run();
    ESP_ERROR_CHECK(err);
}
