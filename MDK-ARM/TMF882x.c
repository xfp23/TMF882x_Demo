#include "TMF882x.h"

/**
 * @brief 写字节（发送数据）
 * @param byte 要写入的数据缓冲区
 * @param len 数据长度
 * @return HAL 状态
 */
static HAL_StatusTypeDef Write_byte(uint8_t *byte, uint16_t len)
{
    return HAL_I2C_Master_Transmit(TMF_I2C_HANDLE, TMF_I2C_ADDR, byte, len, TMF_TIMEOUT);
}

/**
 * @brief 读取字节（接收数据）
 * @param buf 接收缓冲区
 * @param len 需要接收的字节数
 * @return HAL 状态
 */
static HAL_StatusTypeDef Read_byte(uint8_t *buf, uint16_t len)
{
    return HAL_I2C_Master_Receive(TMF_I2C_HANDLE, TMF_I2C_ADDR, buf, len, TMF_TIMEOUT);
}
/**
 * @brief 读取寄存器
 * @param reg 要读取的寄存器地址
 * @return 读取到的数据
 */
uint8_t Read_Reg(TMF_Reg_t reg)
{
    uint8_t byte = 0xFF;

    if (Write_byte((uint8_t *)&reg, 1) != HAL_OK)
    {
        return 0XFF;
    }

    if (Read_byte(&byte, 1) != HAL_OK)
    {
        return 0XFF;
    }

    return byte;
}

/**
 * @brief 向寄存器写单个字节
 *
 * @param reg 寄存器
 * @param cmd 命令
 * @return uint8_t 写成功返回 0x00 ,失败返回0xff
 */
uint8_t Write_Reg(TMF_Reg_t reg, uint8_t cmd)
{
    uint8_t data[2] = {(uint8_t)reg, cmd};
    if (Write_byte(data, 2) != HAL_OK)
    {
        return 0xFF;
    }
    return 0x00;
}

/**
 * @brief 计算数据包的校验和（Checksum）
 *
 * 该函数用于计算一个 8 位的校验字节，以检测数据包在传输过程中的完整性。
 * 校验规则是对 `cmd_stat`、`size` 和 `data` 中所有字节求和后，取最低 8 位并按位取反（补码）。
 *
 * @param cmd_stat     命令状态字节（通常表示当前指令或标志位）
 * @param size         数据包大小（表示 `data` 数组的期望长度）
 * @param data         数据指针，指向需要校验的数据
 * @param data_length  数据长度，即 `data` 数组中的字节数
 * @return uint8_t     计算得到的 8 位校验值
 */
uint8_t calculate_checksum(uint8_t cmd_stat, uint8_t size, uint8_t *data, uint8_t data_length)
{
    uint16_t sum = cmd_stat + size; // 初始化求和变量，包含 cmd_stat 和 size

    for (int i = 0; i < data_length; i++)
    {
        sum += data[i]; // 将 data 数组的每个字节累加到 sum
    }

    return ~(sum & 0xFF); // 取 sum 的最低 8 位，并按位取反（补码），作为校验值
}

/**
 * @brief 烧写固件
 */
void Write_Firmware()
{
    uint8_t Firmware_download[23] = {BL_CMD_STAT, W_RAM, 20};
    uint16_t Write_counts = (uint16_t)(tmf8828_image_length / 20); // 计算完整包的次数
    uint16_t remain_bytes = tmf8828_image_length % 20;             // 计算剩余字节数

    // 发送完整的 20 字节数据包
    for (int i = 0; i < Write_counts; i++)
    {
        memcpy(Firmware_download + 3, tmf8828_image + i * 20, 20);
        Firmware_download[22] = calculate_checksum(W_RAM, 20, Firmware_download + 3, 20);
        Write_byte(Firmware_download, 23); // 发送 23 字节
    }

    // 处理剩余不足 20 字节的数据
    if (remain_bytes > 0)
    {
        Firmware_download[2] = remain_bytes; // 更新数据包大小
        memcpy(Firmware_download + 3, tmf8828_image + Write_counts * 20, remain_bytes);
        Firmware_download[3 + remain_bytes] = calculate_checksum(W_RAM, remain_bytes, Firmware_download + 3, remain_bytes);
        Write_byte(Firmware_download, 4 + remain_bytes); // 发送实际大小的数据
    }
}

uint8_t value = 0;
uint8_t status = 0;
void Bootloader_running()
{
    uint8_t data[5] = {BL_CMD_STAT,DOWNLOAD_INIT,0x01,0x29,0xc1};
    uint8_t data2[6] = {BL_CMD_STAT,ADDR_RAM,0x02,0x00,0x00,0xBA};
    uint8_t cmd[4] = {0x08,0x11,0x00,0xee};
    uint8_t buf[3] = {0};
    Write_Reg(ENABLE_Register, 0x01);
    HAL_Delay(10);
    value = Read_Reg(ENABLE_Register); 
    HAL_Delay(10);

    if (Read_Reg(APPID_Register) == BOOTLOADER) // Bootloader模式
    {

        // Write_Reg(BL_CMD_STAT, DOWNLOAD_INIT); // 下载初始化
        Write_byte(data,5);
        
        HAL_Delay(500);
        //status = Read_Reg(BL_CMD_STAT);
        while(buf[2] != 0xff)
        {
            Read_byte(buf,3);
        }
        HAL_Delay(1000);
        // Write_Reg(BL_CMD_STAT, ADDR_RAM);
        Write_byte(data2,6);
        HAL_Delay(600);


        Write_Firmware(); // 烧录固件
        HAL_Delay(6);

        // Write_Reg(BL_CMD_STAT, RAMREMAP_RESET);
        // HAL_Delay(1000);

        //  Write_Reg(ENABLE_Register, STARTAPP_INRAM | 0x01); // 启动应用
        //  HAL_Delay(10);
        Write_byte(cmd,4);
        HAL_Delay(6);
        value = Read_Reg(APPID_Register); // 读取设备状态
        HAL_Delay(1000);
        status = Read_Reg(APPLICATION_STATUS_Register);

//        if (Read_Reg(APPID_Register) == APPLICATION)
//        {
//            value = APPLICATION;
//            return;
//        }
//        if (value == 0x00)
//        {
//            status = Read_Reg(APPLICATION_STATUS_Register);
//            value = 0XFF;
//        }
    }
}

void TMF882x_Init()
{
    TMF_EN_SET;
    HAL_Delay(6);
    value = Read_Reg(APPID_Register); // 读取设备状态
    HAL_Delay(6);
    Bootloader_running(); // 运行bootloader
    HAL_Delay(6);
	value = 0;
	value = Read_Reg(APPID_Register); // 读取设备状态
}
