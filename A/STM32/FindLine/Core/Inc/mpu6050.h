#ifndef __MPU6050_H__
#define __MPU6050_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdint.h>

typedef enum
{
  MPU6050_RESULT_OK = 0,
  MPU6050_RESULT_NOT_FOUND,
  MPU6050_RESULT_BAD_WHO_AM_I,
  MPU6050_RESULT_I2C_ERROR,
  MPU6050_RESULT_INVALID_ARG
} MPU6050_Result_t;

typedef struct
{
  int16_t accel_raw_x;
  int16_t accel_raw_y;
  int16_t accel_raw_z;
  int16_t temperature_raw;
  int16_t gyro_raw_x;
  int16_t gyro_raw_y;
  int16_t gyro_raw_z;
  int16_t gyro_dps_x10_x;
  int16_t gyro_dps_x10_y;
  int16_t gyro_dps_x10_z;
  uint8_t int_status;
  uint8_t who_am_i;
  uint8_t address_7bit;
} MPU6050_Data_t;

MPU6050_Result_t MPU6050_Init(void);
MPU6050_Result_t MPU6050_ReadData(MPU6050_Data_t *data);
uint8_t MPU6050_IsReady(void);
uint8_t MPU6050_GetAddress7Bit(void);
uint8_t MPU6050_GetWhoAmI(void);
const char *MPU6050_ResultToString(MPU6050_Result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __MPU6050_H__ */
