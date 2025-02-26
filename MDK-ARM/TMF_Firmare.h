#ifndef TMF_FIRMARE_H
#define TMF_FIRMARE_H

/**
 * @file TMF_Firmare.h
 * @author pjh
 * @brief 芯片固件
 * @version 0.1
 * @date 2025-02-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "stdint.h"

#ifdef __cplusplus
extern "C"
{
#endif
// 固件数据的终止标志（termination value），用于校验固件的完整性
extern const unsigned long tmf8828_image_termination; 

// 固件数据的起始地址（start address），表示固件在存储器中的起始位置
extern const unsigned long tmf8828_image_start; 

// 固件数据的结束地址（finish address），表示固件数据的末尾位置
extern const unsigned long tmf8828_image_finish; 

// 固件数据的长度（length），用于计算数据大小
extern const unsigned long tmf8828_image_length; 

// 固件终止标志的宏定义，与 `tmf8828_image_termination` 作用相同
#define TMF8828_IMAGE_TERMINATION 0x00200089 

// 固件起始地址的宏定义，与 `tmf8828_image_start` 作用相同
#define TMF8828_IMAGE_START       0x00200000 

// 固件结束地址的宏定义，与 `tmf8828_image_finish` 作用相同
#define TMF8828_IMAGE_FINISH      0x00201C30 

// 固件数据长度的宏定义，与 `tmf8828_image_length` 作用相同
#define TMF8828_IMAGE_LENGTH      0x00001C30 

extern const uint8_t tmf8828_image[];

#ifdef __cplusplus
}
#endif

#endif // !TMF_FIRMARE_H
