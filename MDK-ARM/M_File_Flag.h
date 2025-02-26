#ifndef M_FILE_FLAG_H
#define M_FILE_FLAG_H

#pragma anon_unions

/**
 * @file M_File_Flag.h
 * @author pjh
 * @brief flag文件
 * @version 0.1
 * @date 2025-02-25
 * 
 * @copyright Copyright (c) 2025
 * 
 */
#include "stdint.h"
#include "string.h"
#include <stdbool.h>
#include "TMF882x.h"

#ifdef __cplusplus
extern "C"
{
#endif

#define ON (1)
#define OFF (0)

typedef union {
	struct {
	uint8_t system_1ms :1;
	uint8_t system_10ms :1;
	uint8_t system_100ms :1;
	uint8_t system_500ms :1;
	uint8_t Reverve_bit :4;
	};
}System_Tim_Flag;


extern volatile System_Tim_Flag systemTim;


#ifdef __cplusplus
}
#endif
#endif
