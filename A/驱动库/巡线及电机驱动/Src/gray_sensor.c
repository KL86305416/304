/**
  ******************************************************************************
  * @file    gray_sensor.c
  * @brief   八路灰度传感器驱动实现（AFA07-S16FCC-00）
  * @note    使用 CLK/DAT 同步串行输出协议：CLK 接 PB10，DAT 接 PB11。
  ******************************************************************************
  */

#include "gray_sensor.h"

static uint8_t sensor_data[GRAY_SENSOR_COUNT] = {0U};

static void GraySensor_DelayUs(uint32_t us);

void GraySensor_Init(void)
{
  GraySensor_ConfigGPIO();

  GRAY_CLK_HIGH();
  GraySensor_DelayUs(100U);

  (void)GraySensor_Read();
}

void GraySensor_ConfigGPIO(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOB_CLK_ENABLE();

  HAL_GPIO_WritePin(GRAY_CLK_PORT, GRAY_CLK_PIN, GPIO_PIN_SET);

  GPIO_InitStruct.Pin = GRAY_CLK_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  HAL_GPIO_Init(GRAY_CLK_PORT, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GRAY_DAT_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GRAY_DAT_PORT, &GPIO_InitStruct);
}

uint8_t GraySensor_Read(void)
{
  uint8_t data = 0U;

  GRAY_CLK_LOW();
  GraySensor_DelayUs(GRAY_SENSOR_CLK_DELAY_US);

  for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++)
  {
    GRAY_CLK_HIGH();
    GraySensor_DelayUs(GRAY_SENSOR_CLK_DELAY_US);

    if (GRAY_DAT_READ() == GPIO_PIN_RESET)
    {
      data |= (uint8_t)(1U << i);
      sensor_data[i] = 1U;
    }
    else
    {
      sensor_data[i] = 0U;
    }

    GRAY_CLK_LOW();
    GraySensor_DelayUs(GRAY_SENSOR_CLK_DELAY_US);
  }

  GRAY_CLK_HIGH();
  GraySensor_DelayUs(GRAY_SENSOR_CLK_DELAY_US);

  return data;
}

GraySensor_Data_t GraySensor_ReadDetail(void)
{
  GraySensor_Data_t result;

  result.value = GraySensor_Read();
  for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++)
  {
    result.sensor[i] = sensor_data[i];
  }

  return result;
}

uint8_t GraySensor_GetLinePosition(void)
{
  uint8_t line_count = 0U;
  uint8_t weighted_sum = 0U;

  for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++)
  {
    if (sensor_data[i] != 0U)
    {
      line_count++;
      weighted_sum = (uint8_t)(weighted_sum + i + 1U);
    }
  }

  if (line_count == 0U)
  {
    return 0U;
  }

  return (uint8_t)(weighted_sum / line_count);
}

float GraySensor_GetLinePositionFloat(uint8_t *active_count)
{
  static const float position_table[GRAY_SENSOR_COUNT] = {
    -3.5f, -2.5f, -1.5f, -0.5f, 0.5f, 1.5f, 2.5f, 3.5f
  };

  uint8_t count = 0U;
  float weighted_sum = 0.0f;

  for (uint8_t i = 0U; i < GRAY_SENSOR_COUNT; i++)
  {
    if (sensor_data[i] != 0U)
    {
      weighted_sum += position_table[i];
      count++;
    }
  }

  if (active_count != 0)
  {
    *active_count = count;
  }

  if (count == 0U)
  {
    return GRAY_SENSOR_LINE_LOST;
  }

  return weighted_sum / (float)count;
}

static void GraySensor_DelayUs(uint32_t us)
{
  uint32_t cycles_per_us = HAL_RCC_GetHCLKFreq() / 1000000U;
  uint32_t loops = (cycles_per_us / 4U) + 1U;

  while (us-- > 0U)
  {
    for (uint32_t i = 0U; i < loops; i++)
    {
      __NOP();
    }
  }
}
