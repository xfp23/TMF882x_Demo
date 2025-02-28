#ifndef TMF882X_H
#define TMF882X_H

#pragma anon_unions

/**
 * @file TMF882x.h
 * @author pjh
 * @brief TMF882x����������
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

#define TMF_I2C_HANDLE &hi2c1                                                      // i2c���
#define TMF_I2C_ADDR (0x41 << 1)                                                   // ��ַ
#define TMF_TIMEOUT 100                                                            // ��ʱʱ�� : ms
#define TMF_EN_SET HAL_GPIO_WritePin(TMF_EN_GPIO_Port, TMF_EN_Pin, GPIO_PIN_SET)   // ����EN
#define TMF_EN_CLR HAL_GPIO_WritePin(TMF_EN_GPIO_Port, TMF_EN_Pin, GPIO_PIN_RESET) // ����EN

    typedef enum
    {
        DEF_STARTBOOTLOADER = 0x00 << 4, // Default �C start bootloader
        STARTBOOT_NOSLEEP = 0x01 << 4,   // bootloader not go to sleep
        STARTAPP_INRAM = 0X02 << 4,      // ��ram����������
    } TMF_powerup_t;

    typedef enum
    {
        APPID_Register = 0x00,              // RO
        BL_CMD_STAT = 0x08,                 // RW
        ENABLE_Register = 0xE0,             // ʹ�ܼĴ��� TMF_powerup_t
        APPLICATION_STATUS_Register = 0x04, // app sta
    } TMF_Reg_t;                            // �Ĵ���

    typedef struct
    {
        uint8_t intrTigger : 1; // �жϴ�����־
    } TMF_flag_t;

    typedef enum
    {
		APPERROR = 0x00,  // error
        BOOTLOADER = 0x80,  // bootloader
        APPLICATION = 0x03, // application running
    } TMF_sta_t;

    /**
     * @brief TMF882X �豸����ö��
     *
     * ��ö�ٶ����� TMF882X �豸����Ҫָ����ڹ̼����ء�RAM �������Լ���ԡ�
     */
    typedef enum
    {
        RAMREMAP_RESET = 0x11, /**< ����ӳ�� RAM ����ַ 0 ����λ */
        DOWNLOAD_INIT = 0x14,  /**< ��ʼ���������� TMF8820/21/28 �� RAM ���� */
        RAM_BIST = 0x2A,       /**< RAM �ڽ��Լ죨ģʽ���ԣ� */
        I2C_BIST = 0x2C,       /**< I?C RAM �ڽ��Լ죨ģʽ���ԣ� */
        W_RAM = 0x41,          /**< д�� RAM ����δ�������ݣ������ Intel Hex ��ʽ�� */
        ADDR_RAM = 0x43        /**< ���ö�/д RAM ָ�뵽ָ����ַ */
    } TMF_cmd_t;

    typedef union
    {
        struct
        {
            TMF_flag_t flag;
			TMF_sta_t appid;
			uint8_t intr_flag;
			uint8_t result_buff[132];
			int filtered_values[9]; // ����װ�������յ��Ľ������λӦ����mm���������һ����������ǿ����Լ�д����������ƽ��ֵ��mm����cm
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
