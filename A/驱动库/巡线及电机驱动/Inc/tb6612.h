#ifndef __TB6612_H__
#define __TB6612_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdint.h>

#define TB6612_PWM_FREQUENCY_HZ  20000U
#define TB6612_DUTY_MAX          1000U

typedef enum
{
  TB6612_MOTOR_A = 0,
  TB6612_MOTOR_B = 1
} TB6612_Motor_t;

typedef enum
{
  TB6612_DIR_FORWARD = 0,
  TB6612_DIR_BACKWARD = 1
} TB6612_Direction_t;

void TB6612_Init(void);
void TB6612_SetMotor(TB6612_Motor_t motor, TB6612_Direction_t direction, uint16_t duty);
void TB6612_SetSpeed(TB6612_Motor_t motor, int16_t speed);
void TB6612_Coast(TB6612_Motor_t motor);
void TB6612_Brake(TB6612_Motor_t motor);
void TB6612_Stop(TB6612_Motor_t motor);
void TB6612_CoastAll(void);
void TB6612_BrakeAll(void);
void TB6612_StopAll(void);

#ifdef __cplusplus
}
#endif

#endif /* __TB6612_H__ */
