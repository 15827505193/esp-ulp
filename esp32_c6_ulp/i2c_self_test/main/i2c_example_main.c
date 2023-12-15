/*
 * SPDX-FileCopyrightText: 2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Unlicense OR CC0-1.0
 */
/* i2c - Example

   For other examples please check:
   https://github.com/espressif/esp-idf/tree/master/examples

   See README.md file to get detailed usage of this example.

   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/
#include <stdio.h>
#include "esp_log.h"
#include "driver/i2c.h"
#include "sdkconfig.h"

static const char *TAG = "i2c-example";

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_SCL_IO           2                                       /*!< gpio number for I2C master clock */
#define I2C_MASTER_SDA_IO           3                                       /*!< gpio number for I2C master data  */
#define I2C_MASTER_NUM              I2C_NUMBER(CONFIG_I2C_MASTER_PORT_NUM)  /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ          CONFIG_I2C_MASTER_FREQUENCY             /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE   0                                       /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE   0                                       /*!< I2C master doesn't need buffer */

#define WRITE_BIT                   I2C_MASTER_WRITE                        /*!< I2C master write */
#define READ_BIT                    I2C_MASTER_READ                         /*!< I2C master read */
#define ACK_CHECK_EN                0x1                                     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS               0x0                                     /*!< I2C master will not check ack from slave */
#define ACK_VAL                     0x0                                     /*!< I2C ack value */
#define NACK_VAL                    0x1                                     /*!< I2C nack value */

#define TSL2561_SENSOR_ADDR         0x39
#define TSL2561_CMD_START           0x03


/**
 * @brief i2c master initialization
 */
static esp_err_t i2c_master_init(void)
{
    int i2c_master_port = I2C_MASTER_NUM;
    i2c_config_t conf = {
        .mode = I2C_MODE_MASTER,
        .sda_io_num = I2C_MASTER_SDA_IO,
        .sda_pullup_en = GPIO_PULLUP_ENABLE,
        .scl_io_num = I2C_MASTER_SCL_IO,
        .scl_pullup_en = GPIO_PULLUP_ENABLE,
        .master.clk_speed = I2C_MASTER_FREQ_HZ,
        // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
    };
    esp_err_t err = i2c_param_config(i2c_master_port, &conf);
    if (err != ESP_OK) {
        return err;
    }
    return i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);
}

/**
 * @brief Initialize the TSL2561 sensor
 *
 * This function initializes the TSL2561 sensor, including initializing the I2C bus,
 * sending the TSL2561 startup command, and waiting for 400 milliseconds.
 *
 * @return
 *     - ESP_OK: Initialization successful
 *     - ESP_FAIL: Initialization failed
 *
 * @note
 * Initialization Steps:
 * 1. Initialize the I2C bus.
 * 2. Send the TSL2561 startup command.
 *    _______________________________________
 *    | start | slave_addr + wr_bit + ack | write command register byte + ack |  write startup byte + ack | stop |
 *    |-------|---------------------------|-----------------------------------|---------------------------|------|
 * 3. Wait for 500 milliseconds.
 */
static esp_err_t tsl2561_sensor_startup(i2c_port_t i2c_num)
{
    int ret = i2c_master_init();
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "i2c master init failed.");
    }

    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, TSL2561_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x80, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, 0x03, ACK_CHECK_EN);
    i2c_master_stop(cmd);

    ret = i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    vTaskDelay(500 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief Read a single bit from the TSL2561 sensor using I2C communication
 *
 * This function reads a single bit from the TSL2561 sensor by initiating I2C communication, sending a command, and receiving the data.
 *
 * @param[in] i2c_num I2C port number
 * @param[in] commend Command byte to be sent to the sensor
 *
 * @return The read bit value
 */
static uint8_t i2c_master_read_bit(i2c_port_t i2c_num, uint8_t commend)
{
    uint8_t data = 0;
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();

    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, TSL2561_SENSOR_ADDR << 1 | WRITE_BIT, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, commend, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, TSL2561_SENSOR_ADDR << 1 | READ_BIT, ACK_CHECK_EN);
    i2c_master_read_byte(cmd, &data, NACK_VAL);
    i2c_master_stop(cmd);

    i2c_master_cmd_begin(i2c_num, cmd, 1000 / portTICK_PERIOD_MS);
    i2c_cmd_link_delete(cmd);
    return data;
}

/**
 * @brief Read data from the TSL2561 sensor's two channels using I2C communication
 *
 * This function reads data from the TSL2561 sensor's two channels (CH0 and CH1) by calling the i2c_master_read_bit function for each channel.
 *
 * @param[in] i2c_num I2C port number
 * @param[out] ch_0_data Pointer to store the data read from channel CH0
 * @param[out] ch_1_data Pointer to store the data read from channel CH1
 */
static void read_tsl2561_data(i2c_port_t i2c_num, uint16_t *ch_0_data, uint16_t *ch_1_data)
{
    *ch_0_data = i2c_master_read_bit(i2c_num, 0x8C);
    *ch_0_data += (i2c_master_read_bit(i2c_num, 0x8D) << 8);
    *ch_1_data = i2c_master_read_bit(i2c_num, 0x8E);
    *ch_1_data += (i2c_master_read_bit(i2c_num, 0x8F) << 8);
}

void app_main(void)
{
    int ret;

    uint16_t visible_light  = 0;
    uint16_t infrared_light  = 0;

    ret = tsl2561_sensor_startup(I2C_MASTER_NUM);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "TSL2561 sensor init failed.");
    } else {
        ESP_LOGI(TAG, "TSL2561 sensor has been successfully initialized.");
    }

    while (1) {
        read_tsl2561_data(I2C_MASTER_NUM, &visible_light, &infrared_light);
        ESP_LOGI(TAG, "TSL2561 sensor data: visible light: %d, infrared light: %d", visible_light, infrared_light);
        vTaskDelay(500 / portTICK_PERIOD_MS);
    }
}
