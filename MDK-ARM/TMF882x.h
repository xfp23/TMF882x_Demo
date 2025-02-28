#ifndef TMF882X_H
#define TMF882X_H

#pragma anon_unions

/**
 * @file TMF882x.h
 * @author pjh
 * @brief TMF882x传感器驱动
 * @version 0.1
 * @date 2025-02-25
 *
 * @copyright Copyright (c) 2025
 *
 */
#include "TMF_Firmare.h"
#include "main.h"
#include "i2c.h"
#include "string.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define TMF_I2C_HANDLE &hi2c1                                                      // i2c句柄
#define TMF_I2C_ADDR (0x41 << 1)                                                   // 地址
#define TMF_TIMEOUT 100                                                            // 超时时间 : ms
#define TMF_EN_SET HAL_GPIO_WritePin(TMF_EN_GPIO_Port, TMF_EN_Pin, GPIO_PIN_SET)   // 拉高EN
#define TMF_EN_CLR HAL_GPIO_WritePin(TMF_EN_GPIO_Port, TMF_EN_Pin, GPIO_PIN_RESET) // 拉低EN

    typedef enum
    {
        DEF_STARTBOOTLOADER = 0x00 << 4, // Default – start bootloader
        STARTBOOT_NOSLEEP = 0x01 << 4,   // bootloader not go to sleep
        STARTAPP_INRAM = 0X02 << 4,      // 在ram里启动程序
    } TMF_powerup_t;

    typedef enum
    {
        APPID_Register = 0x00,              // RO
        BL_CMD_STAT = 0x08,                 // RW
        ENABLE_Register = 0xE0,             // 使能寄存器 TMF_powerup_t
        APPLICATION_STATUS_Register = 0x04, // app sta
    } TMF_Reg_t;                            // 寄存器

    typedef struct
    {
        uint8_t intrTigger : 1; // 中断触发标志
    } TMF_flag_t;

    typedef enum
    {
		APPERROR = 0x00,  // error
        BOOTLOADER = 0x80,  // bootloader
        APPLICATION = 0x03, // application running
    } TMF_sta_t;

    /**
     * @brief TMF882X 设备命令枚举
     *
     * 该枚举定义了 TMF882X 设备的主要指令，用于固件加载、RAM 操作和自检测试。
     */
    typedef enum
    {
        RAMREMAP_RESET = 0x11, /**< 重新映射 RAM 到地址 0 并复位 */
        DOWNLOAD_INIT = 0x14,  /**< 初始化从主机到 TMF8820/21/28 的 RAM 下载 */
        RAM_BIST = 0x2A,       /**< RAM 内建自检（模式测试） */
        I2C_BIST = 0x2C,       /**< I?C RAM 内建自检（模式测试） */
        W_RAM = 0x41,          /**< 写入 RAM 区域（未编码数据，例如非 Intel Hex 格式） */
        ADDR_RAM = 0x43        /**< 设置读/写 RAM 指针到指定地址 */
    } TMF_cmd_t;

    typedef union
    {
        struct
        {
            TMF_flag_t flag;
			TMF_sta_t appid;
			uint8_t intr_flag;
			uint8_t result_buff[132];
			int filtered_values[9]; // 这里装载了最终到的结果，单位应该是mm，至于输出一个结果的你们可以自己写个函数求下平均值把mm换成cm
        };
    } TMF882x_t;

    extern void TMF882x_Init();
	
	extern void read_measurement_results();
	extern void Bootloader_running();
	extern void readappid();
	extern void Write_RAM_Data(uint16_t offset, uint16_t chunk_size);
	extern void startMeasure();
	extern void TMF882x_callBack();
	void TMF882x_Read();
#ifdef __cplusplus
}
#endif
#endif // !TMF882X_H
