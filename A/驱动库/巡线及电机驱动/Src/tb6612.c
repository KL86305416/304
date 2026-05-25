#include "tb6612.h"

#define TB6612_AIN1_GPIO_Port  GPIOC
#define TB6612_AIN1_Pin        GPIO_PIN_6
#define TB6612_AIN2_GPIO_Port  GPIOC
#define TB6612_AIN2_Pin        GPIO_PIN_7

#define TB6612_BIN1_GPIO_Port  GPIOB
#define TB6612_BIN1_Pin        GPIO_PIN_15
#define TB6612_BIN2_GPIO_Port  GPIOB
#define TB6612_BIN2_Pin        GPIO_PIN_14

#define TB6612_PWMA_GPIO_Port  GPIOA
#define TB6612_PWMA_Pin        GPIO_PIN_0
#define TB6612_PWMB_GPIO_Port  GPIOA
#define TB6612_PWMB_Pin        GPIO_PIN_1

static uint32_t tb6612_pwm_period = 799U;

static uint16_t TB6612_ClampDuty(uint16_t duty)
{
  return (duty > TB6612_DUTY_MAX) ? TB6612_DUTY_MAX : duty;
}

static void TB6612_SetCompare(TB6612_Motor_t motor, uint16_t duty)
{
  uint16_t limited_duty = TB6612_ClampDuty(duty);
  uint32_t compare = (uint32_t)(((uint64_t)limited_duty * (tb6612_pwm_period + 1U)) /
                                TB6612_DUTY_MAX);

  if (motor == TB6612_MOTOR_A)
  {
    TIM2->CCR1 = compare;
  }
  else if (motor == TB6612_MOTOR_B)
  {
    TIM2->CCR2 = compare;
  }
}

static void TB6612_SetDirectionPins(TB6612_Motor_t motor,
                                    GPIO_PinState in1,
                                    GPIO_PinState in2)
{
  if (motor == TB6612_MOTOR_A)
  {
    HAL_GPIO_WritePin(TB6612_AIN1_GPIO_Port, TB6612_AIN1_Pin, in1);
    HAL_GPIO_WritePin(TB6612_AIN2_GPIO_Port, TB6612_AIN2_Pin, in2);
  }
  else if (motor == TB6612_MOTOR_B)
  {
    HAL_GPIO_WritePin(TB6612_BIN1_GPIO_Port, TB6612_BIN1_Pin, in1);
    HAL_GPIO_WritePin(TB6612_BIN2_GPIO_Port, TB6612_BIN2_Pin, in2);
  }
}

static void TB6612_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();
  __HAL_RCC_GPIOC_CLK_ENABLE();

  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_6 | GPIO_PIN_7, GPIO_PIN_RESET);
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_RESET);

  GPIO_InitStruct.Pin = GPIO_PIN_6 | GPIO_PIN_7;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = GPIO_PIN_14 | GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  GPIO_InitStruct.Pin = TB6612_PWMA_Pin | TB6612_PWMB_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = GPIO_AF1_TIM2;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);
}

static void TB6612_TIM2_PWM_Init(void)
{
  uint32_t timer_clock_hz = HAL_RCC_GetPCLK1Freq();
  uint32_t pwm_ticks;

  if ((RCC->CFGR & RCC_CFGR_PPRE1) != RCC_CFGR_PPRE1_DIV1)
  {
    timer_clock_hz *= 2U;
  }

  pwm_ticks = timer_clock_hz / TB6612_PWM_FREQUENCY_HZ;
  if (pwm_ticks == 0U)
  {
    pwm_ticks = 1U;
  }

  tb6612_pwm_period = pwm_ticks - 1U;

  __HAL_RCC_TIM2_CLK_ENABLE();

  TIM2->CR1 = 0U;
  TIM2->CR2 = 0U;
  TIM2->SMCR = 0U;
  TIM2->DIER = 0U;
  TIM2->CCER = 0U;
  TIM2->CCMR1 = 0U;
  TIM2->PSC = 0U;
  TIM2->ARR = tb6612_pwm_period;
  TIM2->CCR1 = 0U;
  TIM2->CCR2 = 0U;
  TIM2->CNT = 0U;

  TIM2->CCMR1 = TIM_CCMR1_OC1PE |
                TIM_CCMR1_OC1M_1 |
                TIM_CCMR1_OC1M_2 |
                TIM_CCMR1_OC2PE |
                TIM_CCMR1_OC2M_1 |
                TIM_CCMR1_OC2M_2;
  TIM2->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
  TIM2->CR1 = TIM_CR1_ARPE;
  TIM2->EGR = TIM_EGR_UG;
  TIM2->CR1 |= TIM_CR1_CEN;
}

void TB6612_Init(void)
{
  TB6612_GPIO_Init();
  TB6612_TIM2_PWM_Init();
  TB6612_StopAll();
}

void TB6612_SetMotor(TB6612_Motor_t motor, TB6612_Direction_t direction, uint16_t duty)
{
  TB6612_SetCompare(motor, 0U);

  if (direction == TB6612_DIR_FORWARD)
  {
    TB6612_SetDirectionPins(motor, GPIO_PIN_SET, GPIO_PIN_RESET);
  }
  else
  {
    TB6612_SetDirectionPins(motor, GPIO_PIN_RESET, GPIO_PIN_SET);
  }

  TB6612_SetCompare(motor, duty);
}

void TB6612_SetSpeed(TB6612_Motor_t motor, int16_t speed)
{
  if (speed > (int16_t)TB6612_DUTY_MAX)
  {
    speed = (int16_t)TB6612_DUTY_MAX;
  }
  else if (speed < -(int16_t)TB6612_DUTY_MAX)
  {
    speed = -(int16_t)TB6612_DUTY_MAX;
  }

  if (speed > 0)
  {
    TB6612_SetMotor(motor, TB6612_DIR_FORWARD, (uint16_t)speed);
  }
  else if (speed < 0)
  {
    TB6612_SetMotor(motor, TB6612_DIR_BACKWARD, (uint16_t)(-speed));
  }
  else
  {
    TB6612_Coast(motor);
  }
}

void TB6612_Coast(TB6612_Motor_t motor)
{
  TB6612_SetCompare(motor, 0U);
  TB6612_SetDirectionPins(motor, GPIO_PIN_RESET, GPIO_PIN_RESET);
}

void TB6612_Brake(TB6612_Motor_t motor)
{
  TB6612_SetDirectionPins(motor, GPIO_PIN_SET, GPIO_PIN_SET);
  TB6612_SetCompare(motor, TB6612_DUTY_MAX);
}

void TB6612_Stop(TB6612_Motor_t motor)
{
  TB6612_Coast(motor);
}

void TB6612_CoastAll(void)
{
  TB6612_Coast(TB6612_MOTOR_A);
  TB6612_Coast(TB6612_MOTOR_B);
}

void TB6612_BrakeAll(void)
{
  TB6612_Brake(TB6612_MOTOR_A);
  TB6612_Brake(TB6612_MOTOR_B);
}

void TB6612_StopAll(void)
{
  TB6612_CoastAll();
}
