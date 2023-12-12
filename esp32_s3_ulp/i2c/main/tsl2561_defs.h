#pragma once

#ifdef __cplusplus
extern "C" {
#endif

/***************************************************
 * TSL2561 Register Addresses
 ***************************************************/
#define TSL2561_SENSOR_I2C_ADDR                         0x39
#define TSL2561_SENSOR_REG_ADDR_WHO_AM_I                0x8A

/***************************************************
 * TSL2561 Control Commands
 ***************************************************/
#define TSL2561_SENSOR_READ_CHANNEL0_DATA_LOW_CMD       0x8C
#define TSL2561_SENSOR_READ_CHANNEL0_DATA_HIGH_CMD      0x8D
#define TSL2561_SENSOR_READ_CHANNEL1_DATA_LOW_CMD       0x8E
#define TSL2561_SENSOR_READ_CHANNEL1_DATA_HIGH_CMD      0x8F
#define TSL2561_SENSOR_POWER_ON_CMD                     0x03
#define TSL2561_SENSOR_POWER_OFF_CMD                    0x01

#define EXAMPLE_UV_THRESHOLD    800

static void tsl2561_read_data(void);

#ifdef __cplusplus
}
#endif
