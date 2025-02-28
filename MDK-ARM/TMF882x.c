#include "TMF882x.h"

volatile TMF882x_t tmf882x = {0};

#ifndef ON
#define ON (1)
#endif

#ifndef OFF
#define OFF (0)
#endif

void TMF882x_callBack()
{
	if(tmf882x.flag.intrTigger == OFF)
	{
		tmf882x.flag.intrTigger = ON;
	}

}


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
        Read_byte(data, 3);       // 读取状态寄存器
        HAL_Delay(6);             // 延时，确保状态更新
    } while (data[2] != 0xFF);
}

// 烧写固件
void Write_Firmware()
{
uint8_t Firmware_download[12] = {BL_CMD_STAT, W_RAM, 8};      // 每个数据包大小为 8 字节
uint16_t Write_counts = (uint16_t)(TOF_BIN_IMAGE_LENGTH / 8); // 计算完整包的次数
uint16_t remain_bytes = TOF_BIN_IMAGE_LENGTH % 8;             // 计算剩余字节数
    // 发送完整的 8 字节数据包
    for (int i = 0; i < Write_counts; i++)
    {
        // memset(Firmware_download, 0, 12);  // 清空数据包
        memcpy(Firmware_download + 3, tmf882x_image + i * 8, 8);                        // 从固件数据中复制 8 字节
        Firmware_download[11] = calculate_checksum(W_RAM, 8, tmf882x_image + i * 8, 8); // 计算校验和
        Write_byte(Firmware_download, 12);                                              // 发送 12 字节

        // 检查寄存器状态
        cc(); // 检查状态，确保 Bootloader 准备好
    }

    // 处理剩余不足 8 字节的数据
    if (remain_bytes > 0)
    {
        Firmware_download[2] = remain_bytes;                                                                                // 更新数据包大小
        memcpy(Firmware_download + 3, tmf882x_image + Write_counts * 8, remain_bytes);                                      // 复制剩余字节
        Firmware_download[3 + remain_bytes] = calculate_checksum(W_RAM, remain_bytes, Firmware_download + 3, remain_bytes); // 计算校验和
        Write_byte(Firmware_download, 4 + remain_bytes);                                                                    // 发送实际大小的数据

        // 检查寄存器状态
        cc(); // 检查状态，确保 Bootloader 准备好
    }
}

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
    tmf882x.appid = Read_Reg(APPID_Register);
    if (tmf882x.appid == BOOTLOADER)
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
        tmf882x.appid = Read_Reg(APPID_Register);
    }

    HAL_Delay(1000);
    tmf882x.appid = Read_Reg(APPID_Register);
}

void c(uint8_t expected_value)
{
    uint8_t data = BL_CMD_STAT;
    uint8_t cmd_stat = 0; // 用于存储读取的值

    do
    {
        Write_byte(&data, 1);             // 写入命令
        cmd_stat = Read_Reg(BL_CMD_STAT); // 读取状态寄存器
    } while (cmd_stat != expected_value); // 等待 CMD_STAT 为期望的值（此处为 0x00）
}


void TMF882x_Init()
{
    uint8_t data[4] = {0}; // 用于存储命令和数据
    uint8_t temp = 0x20; // 临时命令，用于读取配置状态

    // 运行 Bootloader，烧写固件
    Bootloader_running();

    // 步骤 1: 发送默认配置命令 w(0x08, 0x16)
    data[0] = 0x08;
    data[1] = 0x16;
    Write_byte(data, 2);

    // 步骤 2: 检查 CMD_STAT 寄存器，等待返回 0x00
    c(0x00);

    // 步骤 3: 检查配置页面是否被正确载入，rs(0x20, data)，data0 = 0x16, data2 = 0xbc, data3 = 0x00
    do
    {
        Write_byte(&temp, 1); // 发送 0x20 命令
        Read_byte(data, 4);   // 读取数据
    } while (data[0] != 0x16 || data[2] != 0xbc || data[3] != 0x00); // 等待配置正确载入

    // 步骤 4: 设置测量周期为 100ms，ws(0x24, {0x64, 0x00})
    memset(data, 0, 4);
    data[0] = 0x24;
    data[1] = 0x64;
    data[2] = 0x00;
    Write_byte(data, 3);
    HAL_Delay(6);

    // 步骤 5: 设置 SPAD 类型，w(0x34, 0x06)
    memset(data, 0, 4);
    data[0] = 0x34;
    data[1] = 0x06;
    Write_byte(data, 2);
    HAL_Delay(6);
    // 步骤 6: 配置 GPIO0 使 LED 随测量闪烁，w(0x31, 0x03)
    memset(data, 0, 4);
    data[0] = 0x31;
    data[1] = 0x03; // 配置 GPIO0 使 LED 闪烁
    Write_byte(data, 2);
    HAL_Delay(6);
    // 步骤 7: 将配置写入传感器，w(0x08, 0x15)
    memset(data, 0, 4);
    data[0] = 0x08;
    data[1] = 0x15;
    Write_byte(data, 2);
    HAL_Delay(6);
    // 步骤 8: 检查 CMD_STAT 寄存器，等待返回 0x00
    c(0x00);
    HAL_Delay(6);
    // 步骤 9: 使能中断，w(0xe2, 0x02)
    memset(data, 0, 4);
    data[0] = 0xe2;
    data[1] = 0x02; // 启用中断
    Write_byte(data, 2);
    HAL_Delay(6);
    // 步骤 10: 清除中断，w(0xe1, 0xff)
    memset(data, 0, 4);
    data[0] = 0xe1;
    data[1] = 0xff; // 清除中断
	
    Write_byte(data, 2);
    HAL_Delay(6);
    // 至此配置完成，可以开始测量
    startMeasure();
}

void readappid()
{
    tmf882x.appid = Read_Reg(APPID_Register);
}

/**
* @brief 开始测量
*/
void startMeasure()
{
	uint8_t data[2] = {0x08,0x10};
    Write_byte(data, 2);
    c(0x01);
}

/**
* @brief 停止测量
*/
void stopMeasure()
{
	uint8_t data[2] = {0x08,0xFF};
	Write_byte(data, 2);
	c(0x00);
}

//void clearIntr()
//{
//}


void TMF882x_Read()
{
	uint8_t data = 0x20;
	if(tmf882x.flag.intrTigger == ON)
	{
		tmf882x.flag.intrTigger = OFF;
		
		tmf882x.intr_flag = Read_Reg(0xe1); // 读中断
		Write_Reg(0xe1,0xff);
		Write_byte(&data,1);
		data = 0x24;
		Write_byte(&data,1);
		Read_byte((uint8_t *)tmf882x.result_buff,132);
		startMeasure();
	}
	
}
void read_measurement_results()
{
	if(tmf882x.flag.intrTigger == ON)
	{
	
	tmf882x.flag.intrTigger = OFF;
    uint8_t result_id = 0x00;
    uint8_t id_reg = 0x20;
    uint8_t reg[2] = {0xe1, 0x00};
    Write_byte(reg, 1);  // 发送数据到设备
    Read_byte(&reg[1], 1);  // 读取数据

    Write_byte(reg, 2);  // 发送数据到设备

    Write_byte(&id_reg, 1);  // 发送寄存器地址
    Read_byte(&result_id, 1);  // 读取测量结果的 ID

    if (result_id == 0x10) {  // 检查是否为测量结果
        Write_byte(&id_reg, 1);  // 发送寄存器地址
        Read_byte((uint8_t *)tmf882x.result_buff, sizeof(tmf882x.result_buff));  // 读取数据到 data 数组

        uint8_t dm, dl;
        uint16_t combined_values[9];  // 保存所有合并后的16位数据

        // 读取和合并数据
        id_reg = 0x3A;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x39;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[0] = (dm << 8) | dl;

        id_reg = 0x3D;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x3C;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[1] = (dm << 8) | dl;

        id_reg = 0x40;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x3F;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[2] = (dm << 8) | dl;

        id_reg = 0x43;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x42;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[3] = (dm << 8) | dl;

        id_reg = 0x46;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x45;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[4] = (dm << 8) | dl;

        id_reg = 0x49;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x48;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[5] = (dm << 8) | dl;

        id_reg = 0x4C;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x4B;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[6] = (dm << 8) | dl;

        id_reg = 0x4F;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x4E;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[7] = (dm << 8) | dl;

        id_reg = 0x52;
        Write_byte(&id_reg, 1);
        Read_byte(&dm, 1);
        id_reg = 0x51;
        Write_byte(&id_reg, 1);
        Read_byte(&dl, 1);
        combined_values[8] = (dm << 8) | dl;

        for (int i = 0; i < 9; i++) {
            tmf882x.filtered_values[i] = 0.1 * combined_values[i] + 0.9 * tmf882x.filtered_values[i];

        }

       
    }
}

}
