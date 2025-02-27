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
    // HAL_I2C_Master_Transmit_DMA(TMF_I2C_HANDLE, TMF_I2C_ADDR, byte, len);
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
uint8_t calculate_checksum(uint8_t cmd_stat, uint8_t size, const uint8_t *data, uint8_t data_length)
{
    uint16_t sum = cmd_stat + size; // 初始化求和变量，包含 cmd_stat 和 size

    for (int i = 0; i < data_length; i++)
    {
        sum += data[i]; // 将 data 数组的每个字节累加到 sum
    }

    return ~(sum & 0xFF); // 取 sum 的最低 8 位，并按位取反（补码），作为校验值
}

/**
 * @brief 检查OK
 * 
 */
void cc()
{
    uint8_t data[3] = {0};
    uint8_t temp_reg = BL_CMD_STAT;
    do
    {
        Write_byte(&temp_reg, 1); // 发送命令
        Read_byte(data, 3);        // 读取状态寄存器
        HAL_Delay(6);             // 延时，确保状态更新
    } while (data[2] != 0xFF);
}

uint8_t Firmware_download[12] = {BL_CMD_STAT, W_RAM, 8};  // 每个数据包大小为 8 字节
uint16_t Write_counts = (uint16_t)(TOF_BIN_IMAGE_LENGTH / 8); // 计算完整包的次数
uint16_t remain_bytes = TOF_BIN_IMAGE_LENGTH % 8;             // 计算剩余字节数
void Write_Firmware()
{


    // 发送完整的 8 字节数据包
    for (int i = 0; i < Write_counts; i++)
    {
        //memset(Firmware_download, 0, 12);  // 清空数据包
        memcpy(Firmware_download + 3, tmf882x_image + i * 8, 8);  // 从固件数据中复制 8 字节
        Firmware_download[11] = calculate_checksum(W_RAM, 8, tmf882x_image + i * 8, 8); // 计算校验和
        Write_byte(Firmware_download, 12);  // 发送 12 字节

        // 检查寄存器状态
        cc();  // 检查状态，确保 Bootloader 准备好
    }

    // 处理剩余不足 8 字节的数据
    if (remain_bytes > 0)
    {
        Firmware_download[2] = remain_bytes;  // 更新数据包大小
        memcpy(Firmware_download + 3, tmf882x_image + Write_counts * 8, remain_bytes);  // 复制剩余字节
        Firmware_download[3 + remain_bytes] = calculate_checksum(W_RAM, remain_bytes, Firmware_download + 3, remain_bytes);  // 计算校验和
        Write_byte(Firmware_download, 4 + remain_bytes);  // 发送实际大小的数据

        // 检查寄存器状态
        cc();  // 检查状态，确保 Bootloader 准备好
    }
}

uint8_t value = 0;
/**
 * @brief 进入bootloader
 * @note 1. 上拉en
 * 2. Write 0x01 to register ENABLE_Register = 0x01
 * 3. 轮询EN_REG直到读到 0x41
 * 4. 读取 APPID_Register 寄存器
 *
 */
void Bootloader_running()
{

    uint8_t firmware_init_reg[5] = {0x80, 0x14, 0x01, 0x29, 0xc1};
    uint8_t set_addr_ram[6] = {0x08, 0x43, 0x02, 0x00, 0x00, 0xBA};
    uint8_t reset_comm[4] = {0x08, 0x11, 0x00, 0xee};

    // 拉高 EN 脚，启动 Bootloader
    TMF_EN_SET; // Assuming it's a macro that pulls EN pin high
    Write_Reg(ENABLE_Register, 0x01);

    // 等待 ENABLE 寄存器为 0x41
    while (Read_Reg(ENABLE_Register) != 0x41)
        ;

    // 读取 APPID 寄存器，检查是否为 Bootloader 模式
    value = Read_Reg(APPID_Register);
    if (value == BOOTLOADER)
    {
        // 发送固件初始化命令
        // Write_Reg(BL_CMD_STAT, DOWNLOAD_INIT);
        Write_byte(firmware_init_reg, 5);

        // 等待固件初始化完成
        cc();

        // 设置 RAM 地址
        Write_byte(set_addr_ram, 6);
        cc();
        Write_Firmware();

        Write_byte(reset_comm, 4);
        HAL_Delay(3); // 延时确保重置完成

        // 再次检查 APPID 寄存器，确认是否完成
        value = Read_Reg(APPID_Register);
    }

    HAL_Delay(1000);
    value = Read_Reg(APPID_Register);
}

void TMF882x_Init()
{

    // HAL_Delay(6);
    // value = Read_Reg(APPID_Register); // 读取设备状态
    // HAL_Delay(6);
    Bootloader_running(); // 运行bootloader
    //    HAL_Delay(6);
    //    value = 0;
    //    value = Read_Reg(APPID_Register); // 读取设备状态
}

void readappid()
{
    value = Read_Reg(APPID_Register);
}