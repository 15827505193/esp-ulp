#include "i2c_sht3x.h"


#define ACK_CHECK_EN   0x1     /*!< I2C master will check ack from slave*/
#define ACK_CHECK_DIS  0x0     /*!< I2C master will not check ack from slave */
#define ACK_VAL    0x0         /*!< I2C ack value */
#define NACK_VAL   0x1         /*!< I2C nack value */

/* 
1. 在使用SHT3X传感器的驱动之前，需要先调用 
esp_err_t i2c_driver_install(i2c_port_t i2c_num, i2c_mode_t mode, size_t slv_rx_buf_len, size_t slv_tx_buf_len,int intr_alloc_flags) 
初始化I2C
2. 在 i2c_sht3x.h 中根据实际使用的I2C总线修改宏定义 SHT3X_I2C_BUS 的值 

*/
/**
 * @brief  I2Cx-写从设备的寄存器值
 *      - 带有写器件寄存器的方式，适用于 MPU6050、ADXL345、HMC5983、MS5611、BMP280等绝大多数I2C设备
 *      - 例：i2c_master_write_slave_reg(I2C_NUM_0, 0x68, 0x75, &test, 1, 100 / portTICK_RATE_MS);
 * 
 * ____________________________________________________________________________________
 * | start | slave_addr + wr_bit + ack | reg_addr + ack | write n bytes + ack  | stop |
 * --------|---------------------------|----------------|----------------------|------|
 * 
 * @param  i2c_num I2C端口号。I2C_NUM_0 / I2C_NUM_1
 * @param  slave_addr I2C写从机的器件地址
 * @param  reg_addr I2C写从机的寄存器地址
 * @param  data_wr 写入的值的指针，存放写入进的数据
 * @param  size 写入的寄存器数目
 * @param  ticks_to_wait 超时等待时间
 * 
 * @return
 *     - esp_err_t
 */
esp_err_t i2c_master_write_slave_reg(i2c_port_t i2c_num, uint8_t slave_addr, uint8_t reg_addr, uint8_t *data_wr, size_t size, TickType_t ticks_to_wait)
{
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    i2c_master_write(cmd, data_wr, size, ACK_CHECK_EN);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);
    return ret;
}

/**
 * @brief  I2Cx-读从设备的寄存器值（寄存器地址 或 命令 为2字节的器件）
 *      - 带有读器件寄存器的方式，适用于 SHT20、GT911 这种寄存器地址为16位的I2C设备
 *      - 例：i2c_master_read_slave_reg_16bit(I2C_NUM_0, 0x44, 0xE000, &test, 6, 100 / portTICK_RATE_MS);
 * 
 * ____________________________________________________________________________________________________________________________________________________
 * | start | slave_addr + rd_bit + ack | reg_addr(2byte) + ack | start | slave_addr + wr_bit + ack | read n-1 bytes + ack | read 1 byte + nack | stop |
 * --------|---------------------------|-------------------------------|---------------------------|----------------------|--------------------|------|
 * 
 * @param  i2c_num I2C端口号。I2C_NUM_0 / I2C_NUM_1
 * @param  slave_addr I2C读从机的器件地址
 * @param  reg_addr I2C读从机的寄存器地址(2byte)
 * @param  data_rd 读出的值的指针，存放读取出的数据
 * @param  size 读取的寄存器数目
 * @param  ticks_to_wait 超时等待时间
 * 
 * @return
 *     - esp_err_t
 */
esp_err_t i2c_master_read_slave_reg_16bit(i2c_port_t i2c_num, uint8_t slave_addr, uint16_t reg_addr, uint8_t *data_rd, size_t size, TickType_t ticks_to_wait)
{
    if (size == 0) {
        return ESP_OK;
    }
    i2c_cmd_handle_t cmd = i2c_cmd_link_create();
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_WRITE, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr>>8, ACK_CHECK_EN);
    i2c_master_write_byte(cmd, reg_addr, ACK_CHECK_EN);
    i2c_master_start(cmd);
    i2c_master_write_byte(cmd, (slave_addr << 1) | I2C_MASTER_READ, ACK_CHECK_EN);
    if (size > 1) {
        i2c_master_read(cmd, data_rd, size - 1, ACK_VAL);
    }
    i2c_master_read_byte(cmd, data_rd + size - 1, NACK_VAL);
    i2c_master_stop(cmd);
    esp_err_t ret = i2c_master_cmd_begin(i2c_num, cmd, ticks_to_wait);
    i2c_cmd_link_delete(cmd);
    return ret;
}


/**
 * @brief    向SHT3x发送一条指令(16bit)
 * 
 * @param    cmd —— SHT3x指令（在SHT3x_MODE中枚举定义）
 * 
 * @retval    成功返回HAL_OK(ESP_OK)
*/
static uint8_t SHT3X_Send_Cmd(SHT30_CMD cmd)
{
    uint8_t cmd_buffer[2];
    cmd_buffer[0] = cmd >> 8;
    cmd_buffer[1] = cmd;
    return i2c_master_write_slave_reg(SHT3X_I2C_BUS, SHT3X_SLAVE_ADDRESS, cmd_buffer[0], cmd_buffer+1, 1, SHT3X_TICKS_TO_WAIT);
}

/**
 * @brief    复位SHT3X
 * 
 * @param    none
 * 
 * @retval    none
*/
void sht3x_reset(void)
{
    SHT3X_Send_Cmd(SOFT_RESET_CMD);
    vTaskDelay(20 / portTICK_PERIOD_MS);
}

/**
 * @brief    初始化SHT30
 * 
 * @param    none
 * 
 * @retval    成功返回HAL_OK(ESP_OK)
 * 
 * @note    周期测量模式
*/
esp_err_t sht3x_init(void)
{
    return SHT3X_Send_Cmd(MEDIUM_2_CMD);
}


/**
 * @brief    从SHT3X读取一次数据
 * 
 * @param    dat —— 存储读取数据的地址（6个字节数组）
 * 
 * @retval    成功 —— 返回HAL_OK(ESP_OK)
*/
esp_err_t sht3x_read_th_raw_dat(uint8_t* dat)
{
    return i2c_master_read_slave_reg_16bit(SHT3X_I2C_BUS, SHT3X_SLAVE_ADDRESS, READOUT_FOR_PERIODIC_MODE, dat, 6, SHT3X_TICKS_TO_WAIT);
}

#define CRC8_POLYNOMIAL 0x31

static uint8_t SHT3X_CheckCrc8(uint8_t* const message, uint8_t initial_value)
{
    uint8_t  remainder;        //余数
    uint8_t  i = 0, j = 0;  //循环变量

    /* 初始化 */
    remainder = initial_value;

    for(j = 0; j < 2;j++)
    {
        remainder ^= message[j];

        /* 从最高位开始依次计算  */
        for (i = 0; i < 8; i++)
        {
            if (remainder & 0x80)
            {
                remainder = (remainder << 1)^CRC8_POLYNOMIAL;
            }
            else
            {
                remainder = (remainder << 1);
            }
        }
    }

    /* 返回计算的CRC码 */
    return remainder;
}

/**
 * @brief    将SHT30接收的6个字节数据进行CRC校验，并转换为温度值和湿度值
 * 
 * @param    dat  —— 存储接收数据的地址（6个字节数组）
 * 
 * @retval    校验成功  —— 返回0
 *            校验失败  —— 返回1，并设置温度值和湿度值为0
*/
uint8_t sht3x_dat2float(uint8_t* const dat, float* temperature, float* humidity)
{
    uint16_t recv_temperature = 0;
    uint16_t recv_humidity = 0;

    /* 校验温度数据和湿度数据是否接收正确 */
    if(SHT3X_CheckCrc8(dat, 0xFF) != dat[2] || SHT3X_CheckCrc8(&dat[3], 0xFF) != dat[5])
        return 1;

    /* 转换温度数据 */
    recv_temperature = ((uint16_t)dat[0]<<8)|dat[1];
    *temperature = -45 + 175*((float)recv_temperature/65535);

    /* 转换湿度数据 */
    recv_humidity = ((uint16_t)dat[3]<<8)|dat[4];
    *humidity = 100 * ((float)recv_humidity / 65535);

    return 0;
}
