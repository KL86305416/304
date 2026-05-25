/**
  ******************************************************************************
  * @file    gray_sensor.h
  * @brief   八路灰度传感器驱动头文件（AFA07-S16FCC-00）
  * @note    使用 CLK/DAT 同步串行输出协议：CLK 接 PB10，DAT 接 PB11。
  ******************************************************************************
  */

#ifndef __GRAY_SENSOR_H__
#define __GRAY_SENSOR_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdint.h>

#define GRAY_SENSOR_COUNT         8U
#define GRAY_SENSOR_CLK_DELAY_US  10U
#define GRAY_SENSOR_LINE_LOST     99.0f

#define GRAY_CLK_PIN              GPIO_PIN_10
#define GRAY_CLK_PORT             GPIOB
#define GRAY_DAT_PIN              GPIO_PIN_11
#define GRAY_DAT_PORT             GPIOB

#define GRAY_CLK_HIGH()           HAL_GPIO_WritePin(GRAY_CLK_PORT, GRAY_CLK_PIN, GPIO_PIN_SET)
#define GRAY_CLK_LOW()            HAL_GPIO_WritePin(GRAY_CLK_PORT, GRAY_CLK_PIN, GPIO_PIN_RESET)
#define GRAY_DAT_READ()           HAL_GPIO_ReadPin(GRAY_DAT_PORT, GRAY_DAT_PIN)

typedef struct
{
  uint8_t sensor[GRAY_SENSOR_COUNT];  /* 0：白底，1：黑线 */
  uint8_t value;                      /* bit0~bit7 对应 1~8 路传感器 */
} GraySensor_Data_t;

void GraySensor_Init(void);
void GraySensor_ConfigGPIO(void);
uint8_t GraySensor_Read(void);
GraySensor_Data_t GraySensor_ReadDetail(void);
uint8_t GraySensor_GetLinePosition(void);
float GraySensor_GetLinePositionFloat(uint8_t *active_count);

#ifdef __cplusplus
}
#endif

#endif /* __GRAY_SENSOR_H__ */
