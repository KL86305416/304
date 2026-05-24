#ifndef __LINE_FOLLOWER_H__
#define __LINE_FOLLOWER_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdint.h>

typedef struct
{
  uint8_t raw_value;
  uint8_t active_count;
  uint8_t line_lost;
  uint8_t curve_active;
  int16_t error_x100;
  int16_t left_speed;
  int16_t right_speed;
} LineFollower_Status_t;

void LineFollower_Init(void);
LineFollower_Status_t LineFollower_Update(void);
void LineFollower_Stop(void);

#ifdef __cplusplus
}
#endif

#endif /* __LINE_FOLLOWER_H__ */
