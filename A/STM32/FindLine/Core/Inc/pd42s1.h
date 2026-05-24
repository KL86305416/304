#ifndef __PD42S1_H__
#define __PD42S1_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stddef.h>
#include <stdint.h>

#define PD42S1_DEFAULT_ADDR             0x01U
#define PD42S1_BAUDRATE                 115200U
#define PD42S1_FRAME_HEAD               0xC5U
#define PD42S1_FRAME_TAIL               0x5CU
#define PD42S1_COMMAND_GAP_MS           2U
#define PD42S1_TX_TIMEOUT_MS            20U
#define PD42S1_RX_TIMEOUT_MS            200U
#define PD42S1_WAIT_POLL_MS             20U
#define PD42S1_STEPS_PER_REV            51200U
#define PD42S1_MAX_ACCEL                200U
#define PD42S1_MAX_RPM                  6000U
#define PD42S1_MAX_DATA_LENGTH          24U
#define PD42S1_MAX_FRAME_LENGTH         (PD42S1_MAX_DATA_LENGTH + 5U)
#define PD42S1_MIN_MOVE_TIMEOUT_MS      5000U
#define PD42S1_MOVE_TIMEOUT_MARGIN_MS   5000U

/* 正点原子 PD42S1 自定义协议功能码。若手册版本不同，只需要改这里。 */
#define PD42S1_CODE_READ_POSITION       0x2AU
#define PD42S1_CODE_READ_STATUS         0x2CU
#define PD42S1_CODE_READ_ARRIVED        0x30U
#define PD42S1_CODE_SET_SPEED           0xF1U
#define PD42S1_CODE_MOVE_ABSOLUTE       0xF2U
#define PD42S1_CODE_MOVE_RELATIVE       0xF3U
#define PD42S1_CODE_CLEAR_POSITION      0xF8U
#define PD42S1_CODE_CLEAR_STATE         0xFBU
#define PD42S1_CODE_ENABLE_MOTOR        0xFAU
#define PD42S1_CODE_STOP_MOTOR          0xFCU

typedef enum
{
  PD42S1_RESULT_OK = 0,
  PD42S1_RESULT_NOT_READY,
  PD42S1_RESULT_INVALID_ARG,
  PD42S1_RESULT_FRAME_TOO_LONG,
  PD42S1_RESULT_TX_TIMEOUT,
  PD42S1_RESULT_RX_TIMEOUT,
  PD42S1_RESULT_WAIT_TIMEOUT,
  PD42S1_RESULT_BAD_FRAME,
  PD42S1_RESULT_CHECKSUM_ERROR,
  PD42S1_RESULT_ADDR_ERROR,
  PD42S1_RESULT_CODE_ERROR,
  PD42S1_RESULT_OPERATION_FAILED
} PD42S1_Result_t;

typedef enum
{
  PD42S1_DIRECTION_CW = 0U,
  PD42S1_DIRECTION_CCW = 1U
} PD42S1_Direction_t;

typedef struct
{
  uint8_t addr;
  uint8_t code;
  uint8_t data[PD42S1_MAX_DATA_LENGTH];
  uint8_t data_length;
} PD42S1_Reply_t;

PD42S1_Result_t PD42S1_Init(void);
void PD42S1_DeInit(void);
uint8_t PD42S1_IsReady(void);
void PD42S1_SetAddress(uint8_t addr);
uint8_t PD42S1_GetAddress(void);

PD42S1_Result_t PD42S1_BuildFrame(uint8_t addr,
                                  uint8_t code,
                                  const uint8_t *data,
                                  uint8_t data_length,
                                  uint8_t *frame,
                                  size_t frame_size,
                                  size_t *frame_length);
PD42S1_Result_t PD42S1_SendCommand(uint8_t code,
                                   const uint8_t *data,
                                   uint8_t data_length,
                                   uint8_t expect_reply,
                                   PD42S1_Reply_t *reply);

PD42S1_Result_t PD42S1_EnableMotor(void);
PD42S1_Result_t PD42S1_DisableMotor(void);
PD42S1_Result_t PD42S1_ClearPosition(void);
PD42S1_Result_t PD42S1_ClearState(void);
PD42S1_Result_t PD42S1_StopMotor(void);
PD42S1_Result_t PD42S1_SetSpeed(PD42S1_Direction_t direction,
                                uint16_t accel,
                                uint16_t rpm);
PD42S1_Result_t PD42S1_MoveRelative(PD42S1_Direction_t direction,
                                    uint16_t accel,
                                    uint16_t rpm,
                                    float revolutions);
PD42S1_Result_t PD42S1_MoveAbsolute(PD42S1_Direction_t direction,
                                    uint16_t accel,
                                    uint16_t rpm,
                                    float revolutions);
PD42S1_Result_t PD42S1_ReadPosition(int32_t *position);
PD42S1_Result_t PD42S1_ReadStatus(uint8_t *status);
uint32_t PD42S1_CalcMoveTimeoutMs(uint16_t rpm, float revolutions);
PD42S1_Result_t PD42S1_WaitUntilArrived(uint32_t timeout_ms);
const char *PD42S1_ResultToString(PD42S1_Result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __PD42S1_H__ */
