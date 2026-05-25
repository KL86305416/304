#ifndef __OPENMV_TRACKER_CONTROL_H__
#define __OPENMV_TRACKER_CONTROL_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "openmv_uart.h"
#include "pd42s1.h"

#include <stdint.h>

typedef struct
{
  uint8_t initialized;
  OpenMV_UART_Result_t openmv_result;
  PD42S1_Result_t motor_result;
} OpenMV_TrackerControl_Status_t;

void OpenMV_TrackerControl_Init(void);
void OpenMV_TrackerControl_Update(void);
void OpenMV_TrackerControl_DeInit(void);
const OpenMV_TrackerControl_Status_t *OpenMV_TrackerControl_GetStatus(void);

#ifdef __cplusplus
}
#endif

#endif /* __OPENMV_TRACKER_CONTROL_H__ */
