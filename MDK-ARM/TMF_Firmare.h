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
#define TOF_BIN_IMAGE_TERMINATION 0x00200089
#define TOF_BIN_IMAGE_START       0x00200000
#define TOF_BIN_IMAGE_FINISH      0x002009AC
#define TOF_BIN_IMAGE_LENGTH      0x000009AC

extern const uint8_t tmf882x_image[];

#ifdef __cplusplus
}
#endif

#endif // !TMF_FIRMARE_H
