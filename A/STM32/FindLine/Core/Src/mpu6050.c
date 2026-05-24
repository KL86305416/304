#include "mpu6050.h"
#include "i2c.h"

#define MPU6050_ADDRESS_LOW            (0x68U << 1)
#define MPU6050_ADDRESS_HIGH           (0x69U << 1)
#define MPU6050_EXPECTED_WHO_AM_I      0x68U

#define MPU6050_REG_SMPLRT_DIV         0x19U
#define MPU6050_REG_CONFIG             0x1AU
#define MPU6050_REG_GYRO_CONFIG        0x1BU
#define MPU6050_REG_ACCEL_CONFIG       0x1CU
#define MPU6050_REG_INT_PIN_CFG        0x37U
#define MPU6050_REG_INT_ENABLE         0x38U
#define MPU6050_REG_INT_STATUS         0x3AU
#define MPU6050_REG_ACCEL_XOUT_H       0x3BU
#define MPU6050_REG_PWR_MGMT_1         0x6BU
#define MPU6050_REG_WHO_AM_I           0x75U

#define MPU6050_I2C_TIMEOUT_MS         50U
#define MPU6050_I2C_READY_TRIALS       2U

static uint8_t mpu6050_ready = 0U;
static uint8_t mpu6050_address = MPU6050_ADDRESS_LOW;
static uint8_t mpu6050_who_am_i = 0U;

static int16_t MPU6050_ReadInt16Be(const uint8_t *data)
{
  return (int16_t)((uint16_t)data[0] << 8 | data[1]);
}

static MPU6050_Result_t MPU6050_WriteReg(uint8_t reg, uint8_t value)
{
  if (HAL_I2C_Mem_Write(&hi2c3,
                        mpu6050_address,
                        reg,
                        I2C_MEMADD_SIZE_8BIT,
                        &value,
                        1U,
                        MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return MPU6050_RESULT_I2C_ERROR;
  }

  return MPU6050_RESULT_OK;
}

static MPU6050_Result_t MPU6050_ReadReg(uint8_t reg, uint8_t *value)
{
  if (value == NULL)
  {
    return MPU6050_RESULT_INVALID_ARG;
  }

  if (HAL_I2C_Mem_Read(&hi2c3,
                       mpu6050_address,
                       reg,
                       I2C_MEMADD_SIZE_8BIT,
                       value,
                       1U,
                       MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return MPU6050_RESULT_I2C_ERROR;
  }

  return MPU6050_RESULT_OK;
}

static MPU6050_Result_t MPU6050_ReadRegs(uint8_t reg, uint8_t *data, uint16_t length)
{
  if ((data == NULL) || (length == 0U))
  {
    return MPU6050_RESULT_INVALID_ARG;
  }

  if (HAL_I2C_Mem_Read(&hi2c3,
                       mpu6050_address,
                       reg,
                       I2C_MEMADD_SIZE_8BIT,
                       data,
                       length,
                       MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return MPU6050_RESULT_I2C_ERROR;
  }

  return MPU6050_RESULT_OK;
}

static MPU6050_Result_t MPU6050_ProbeAddress(uint8_t address)
{
  if (HAL_I2C_IsDeviceReady(&hi2c3,
                            address,
                            MPU6050_I2C_READY_TRIALS,
                            MPU6050_I2C_TIMEOUT_MS) != HAL_OK)
  {
    return MPU6050_RESULT_NOT_FOUND;
  }

  mpu6050_address = address;
  return MPU6050_RESULT_OK;
}

MPU6050_Result_t MPU6050_Init(void)
{
  MPU6050_Result_t result;

  mpu6050_ready = 0U;
  mpu6050_who_am_i = 0U;

  result = MPU6050_ProbeAddress(MPU6050_ADDRESS_LOW);
  if (result != MPU6050_RESULT_OK)
  {
    result = MPU6050_ProbeAddress(MPU6050_ADDRESS_HIGH);
    if (result != MPU6050_RESULT_OK)
    {
      return MPU6050_RESULT_NOT_FOUND;
    }
  }

  result = MPU6050_ReadReg(MPU6050_REG_WHO_AM_I, &mpu6050_who_am_i);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  if (mpu6050_who_am_i != MPU6050_EXPECTED_WHO_AM_I)
  {
    return MPU6050_RESULT_BAD_WHO_AM_I;
  }

  result = MPU6050_WriteReg(MPU6050_REG_PWR_MGMT_1, 0x01U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }
  HAL_Delay(10U);

  result = MPU6050_WriteReg(MPU6050_REG_SMPLRT_DIV, 0x07U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  result = MPU6050_WriteReg(MPU6050_REG_CONFIG, 0x03U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  result = MPU6050_WriteReg(MPU6050_REG_GYRO_CONFIG, 0x00U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  result = MPU6050_WriteReg(MPU6050_REG_ACCEL_CONFIG, 0x00U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  result = MPU6050_WriteReg(MPU6050_REG_INT_PIN_CFG, 0x20U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  result = MPU6050_WriteReg(MPU6050_REG_INT_ENABLE, 0x01U);
  if (result != MPU6050_RESULT_OK)
  {
    return result;
  }

  mpu6050_ready = 1U;
  return MPU6050_RESULT_OK;
}

MPU6050_Result_t MPU6050_ReadData(MPU6050_Data_t *data)
{
  uint8_t raw[14];
  MPU6050_Result_t result;

  if (data == NULL)
  {
    return MPU6050_RESULT_INVALID_ARG;
  }

  if (mpu6050_ready == 0U)
  {
    return MPU6050_RESULT_NOT_FOUND;
  }

  result = MPU6050_ReadReg(MPU6050_REG_INT_STATUS, &data->int_status);
  if (result != MPU6050_RESULT_OK)
  {
    mpu6050_ready = 0U;
    return result;
  }

  result = MPU6050_ReadRegs(MPU6050_REG_ACCEL_XOUT_H, raw, sizeof(raw));
  if (result != MPU6050_RESULT_OK)
  {
    mpu6050_ready = 0U;
    return result;
  }

  data->accel_raw_x = MPU6050_ReadInt16Be(&raw[0]);
  data->accel_raw_y = MPU6050_ReadInt16Be(&raw[2]);
  data->accel_raw_z = MPU6050_ReadInt16Be(&raw[4]);
  data->temperature_raw = MPU6050_ReadInt16Be(&raw[6]);
  data->gyro_raw_x = MPU6050_ReadInt16Be(&raw[8]);
  data->gyro_raw_y = MPU6050_ReadInt16Be(&raw[10]);
  data->gyro_raw_z = MPU6050_ReadInt16Be(&raw[12]);

  data->gyro_dps_x10_x = (int16_t)(((int32_t)data->gyro_raw_x * 10) / 131);
  data->gyro_dps_x10_y = (int16_t)(((int32_t)data->gyro_raw_y * 10) / 131);
  data->gyro_dps_x10_z = (int16_t)(((int32_t)data->gyro_raw_z * 10) / 131);
  data->who_am_i = mpu6050_who_am_i;
  data->address_7bit = (uint8_t)(mpu6050_address >> 1);

  return MPU6050_RESULT_OK;
}

uint8_t MPU6050_IsReady(void)
{
  return mpu6050_ready;
}

uint8_t MPU6050_GetAddress7Bit(void)
{
  return (uint8_t)(mpu6050_address >> 1);
}

uint8_t MPU6050_GetWhoAmI(void)
{
  return mpu6050_who_am_i;
}

const char *MPU6050_ResultToString(MPU6050_Result_t result)
{
  switch (result)
  {
    case MPU6050_RESULT_OK:
      return "OK";
    case MPU6050_RESULT_NOT_FOUND:
      return "No Device";
    case MPU6050_RESULT_BAD_WHO_AM_I:
      return "Bad WHOAMI";
    case MPU6050_RESULT_I2C_ERROR:
      return "I2C Error";
    case MPU6050_RESULT_INVALID_ARG:
    default:
      return "Arg Error";
  }
}
