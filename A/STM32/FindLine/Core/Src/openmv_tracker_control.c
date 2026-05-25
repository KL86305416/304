#include "openmv_tracker_control.h"

#include <stdint.h>

static OpenMV_TrackerControl_Status_t tracker_status = {
  .initialized = 0U,
  .openmv_result = OPENMV_UART_NOT_READY,
  .motor_result = PD42S1_RESULT_NOT_READY
};

void OpenMV_TrackerControl_Init(void)
{
  tracker_status.initialized = 0U;

  if (OpenMV_UART_IsReady() == 0U)
  {
    tracker_status.openmv_result = OpenMV_UART_Init();
  }
  else
  {
    tracker_status.openmv_result = OPENMV_UART_OK;
  }

  if (tracker_status.openmv_result == OPENMV_UART_OK)
  {
    OpenMV_UART_FlushRx();
  }

  if (PD42S1_IsReady() == 0U)
  {
    tracker_status.motor_result = PD42S1_Init();
  }
  else
  {
    tracker_status.motor_result = PD42S1_RESULT_OK;
  }

  if (tracker_status.motor_result == PD42S1_RESULT_OK)
  {
    tracker_status.motor_result = PD42S1_EnableMotor();
  }

  if (tracker_status.motor_result == PD42S1_RESULT_OK)
  {
    tracker_status.motor_result = PD42S1_ClearState();
  }

  if ((tracker_status.openmv_result == OPENMV_UART_OK) &&
      (tracker_status.motor_result == PD42S1_RESULT_OK))
  {
    tracker_status.initialized = 1U;
  }
}

void OpenMV_TrackerControl_Update(void)
{
  OpenMV_BlobData_t blob;
  OpenMV_UART_Result_t result;

  result = OpenMV_UART_ReceiveBlobData(&blob, 30U);

  int32_t fix_err = blob.err_x ;
  int32_t abs_err = fix_err;
  int32_t rpm = 0;

  if (abs_err < 0)
  {
    abs_err = -abs_err;
  }

  rpm = (40 + abs_err)/6;

  if (rpm > 100)
  {
    rpm = 25;
  }

  if (result != OPENMV_UART_OK)
  {
    return;
  }

  if (blob.detected == 0U)
  {
    PD42S1_SetSpeed(PD42S1_DIRECTION_CW, 8, rpm);
  }

  if (fix_err > 12)
  {
    PD42S1_SetSpeed(PD42S1_DIRECTION_CCW, 8, rpm);
  }
  else if (fix_err < -12)
  {
    PD42S1_SetSpeed(PD42S1_DIRECTION_CW, 8, rpm);
  }
  else
  {   
    PD42S1_SetSpeed(PD42S1_DIRECTION_CW, 8, 0);
  }
}

void OpenMV_TrackerControl_DeInit(void)
{
    (void)PD42S1_DisableMotor();
}

const OpenMV_TrackerControl_Status_t *OpenMV_TrackerControl_GetStatus(void)
{
  return &tracker_status;
}
