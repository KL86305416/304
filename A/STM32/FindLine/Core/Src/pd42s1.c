#include "pd42s1.h"

#include <string.h>

#define PD42S1_USART              USART2
#define PD42S1_GPIO_PORT          GPIOA
#define PD42S1_TX_PIN             GPIO_PIN_2
#define PD42S1_RX_PIN             GPIO_PIN_3
#define PD42S1_GPIO_AF            GPIO_AF7_USART2

static uint8_t pd42s1_ready = 0U;
static uint8_t pd42s1_addr = PD42S1_DEFAULT_ADDR;
static uint32_t pd42s1_last_command_tick = 0U;

static uint8_t PD42S1_WaitFlag(uint32_t flag, uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while ((PD42S1_USART->ISR & flag) == 0U)
  {
    if ((HAL_GetTick() - start_tick) >= timeout_ms)
    {
      return 0U;
    }
  }

  return 1U;
}

static void PD42S1_ClearUartErrors(void)
{
  uint32_t flags = PD42S1_USART->ISR;

  if ((flags & USART_ISR_ORE) != 0U)
  {
    PD42S1_USART->ICR = USART_ICR_ORECF;
  }
  if ((flags & USART_ISR_FE) != 0U)
  {
    PD42S1_USART->ICR = USART_ICR_FECF;
  }
  if ((flags & USART_ISR_NE) != 0U)
  {
    PD42S1_USART->ICR = USART_ICR_NECF;
  }
}

static void PD42S1_FlushRx(void)
{
  PD42S1_ClearUartErrors();

  while ((PD42S1_USART->ISR & USART_ISR_RXNE_RXFNE) != 0U)
  {
    (void)PD42S1_USART->RDR;
  }
}

static void PD42S1_ApplyCommandGap(void)
{
  uint32_t now = HAL_GetTick();
  uint32_t elapsed = now - pd42s1_last_command_tick;

  if (elapsed < PD42S1_COMMAND_GAP_MS)
  {
    HAL_Delay(PD42S1_COMMAND_GAP_MS - elapsed);
  }
}

static uint8_t PD42S1_CalcChecksum(const uint8_t *data, size_t length)
{
  uint32_t sum = 0U;

  for (size_t i = 0U; i < length; i++)
  {
    sum += data[i];
  }

  return (uint8_t)(sum & 0xFFU);
}

static void PD42S1_PutU16Be(uint8_t *data, uint16_t value)
{
  data[0] = (uint8_t)(value >> 8);
  data[1] = (uint8_t)(value & 0xFFU);
}

static void PD42S1_PutU32Be(uint8_t *data, uint32_t value)
{
  data[0] = (uint8_t)(value >> 24);
  data[1] = (uint8_t)((value >> 16) & 0xFFU);
  data[2] = (uint8_t)((value >> 8) & 0xFFU);
  data[3] = (uint8_t)(value & 0xFFU);
}

static void PD42S1_PutFloatBe(uint8_t *data, float value)
{
  uint32_t raw_value;

  /* 手册要求 float 使用 IEEE754 大端序发送，先复制原始位再按大端写入。 */
  (void)memcpy(&raw_value, &value, sizeof(raw_value));
  PD42S1_PutU32Be(data, raw_value);
}

static uint32_t PD42S1_GetU32Be(const uint8_t *data)
{
  return ((uint32_t)data[0] << 24) |
         ((uint32_t)data[1] << 16) |
         ((uint32_t)data[2] << 8) |
         (uint32_t)data[3];
}

static uint8_t PD42S1_IsMotionArgsValid(uint16_t accel, uint16_t rpm)
{
  return ((accel <= PD42S1_MAX_ACCEL) && (rpm <= PD42S1_MAX_RPM)) ? 1U : 0U;
}

static PD42S1_Result_t PD42S1_RevolutionsToPulses(float revolutions,
                                                   uint32_t *pulses)
{
  float pulse_value;

  if ((pulses == NULL) || (revolutions < 0.0f))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  pulse_value = revolutions * (float)PD42S1_STEPS_PER_REV;
  if (pulse_value > 4294967295.0f)
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  *pulses = (uint32_t)(pulse_value + 0.5f);
  return PD42S1_RESULT_OK;
}

static PD42S1_Result_t PD42S1_SendBytes(const uint8_t *data, size_t length)
{
  if ((data == NULL) || (length == 0U))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  for (size_t i = 0U; i < length; i++)
  {
    if (PD42S1_WaitFlag(USART_ISR_TXE_TXFNF, PD42S1_TX_TIMEOUT_MS) == 0U)
    {
      return PD42S1_RESULT_TX_TIMEOUT;
    }
    PD42S1_USART->TDR = data[i];
  }

  if (PD42S1_WaitFlag(USART_ISR_TC, PD42S1_TX_TIMEOUT_MS) == 0U)
  {
    return PD42S1_RESULT_TX_TIMEOUT;
  }

  return PD42S1_RESULT_OK;
}

static size_t PD42S1_GetExpectedReplyLength(uint8_t code)
{
  switch (code)
  {
    case PD42S1_CODE_READ_POSITION:
      return 10U; /* 帧头 + 地址 + 功能码 + 5 字节数据 + 校验 + 帧尾 */
    case PD42S1_CODE_READ_STATUS:
    case PD42S1_CODE_READ_ARRIVED:
    case PD42S1_CODE_ENABLE_MOTOR:
      return 7U;  /* 2 字节数据 */
    case PD42S1_CODE_SET_SPEED:
    case PD42S1_CODE_MOVE_ABSOLUTE:
    case PD42S1_CODE_MOVE_RELATIVE:
    case PD42S1_CODE_CLEAR_POSITION:
    case PD42S1_CODE_CLEAR_STATE:
    case PD42S1_CODE_STOP_MOTOR:
      return 6U;  /* 1 字节操作结果 */
    default:
      return 0U;
  }
}

static PD42S1_Result_t PD42S1_ReadFrame(uint8_t *frame,
                                        size_t frame_size,
                                        size_t *frame_length,
                                        size_t expected_length)
{
  uint32_t start_tick = HAL_GetTick();
  size_t length = 0U;

  if ((frame == NULL) || (frame_size < 5U) || (frame_length == NULL))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  if ((expected_length != 0U) && (expected_length > frame_size))
  {
    return PD42S1_RESULT_FRAME_TOO_LONG;
  }

  while ((HAL_GetTick() - start_tick) < PD42S1_RX_TIMEOUT_MS)
  {
    PD42S1_ClearUartErrors();

    if ((PD42S1_USART->ISR & USART_ISR_RXNE_RXFNE) != 0U)
    {
      uint8_t byte = (uint8_t)(PD42S1_USART->RDR & 0xFFU);

      /* 协议没有长度字段，因此先同步到帧头，再等待帧尾。 */
      if ((length == 0U) && (byte != PD42S1_FRAME_HEAD))
      {
        continue;
      }

      if (length >= frame_size)
      {
        return PD42S1_RESULT_FRAME_TOO_LONG;
      }

      frame[length++] = byte;
      if (expected_length != 0U)
      {
        if (length == expected_length)
        {
          *frame_length = length;
          return (byte == PD42S1_FRAME_TAIL) ? PD42S1_RESULT_OK : PD42S1_RESULT_BAD_FRAME;
        }
      }
      else if ((length >= 5U) && (byte == PD42S1_FRAME_TAIL))
      {
        *frame_length = length;
        return PD42S1_RESULT_OK;
      }
    }
  }

  return PD42S1_RESULT_RX_TIMEOUT;
}

static PD42S1_Result_t PD42S1_ParseReply(const uint8_t *frame,
                                         size_t frame_length,
                                         uint8_t expected_code,
                                         PD42S1_Reply_t *reply)
{
  uint8_t checksum;
  size_t data_length;

  if ((frame == NULL) || (reply == NULL) || (frame_length < 5U))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  if ((frame[0] != PD42S1_FRAME_HEAD) ||
      (frame[frame_length - 1U] != PD42S1_FRAME_TAIL))
  {
    return PD42S1_RESULT_BAD_FRAME;
  }

  checksum = PD42S1_CalcChecksum(frame, frame_length - 2U);
  if (checksum != frame[frame_length - 2U])
  {
    return PD42S1_RESULT_CHECKSUM_ERROR;
  }

  if (frame[1] != pd42s1_addr)
  {
    return PD42S1_RESULT_ADDR_ERROR;
  }

  if (frame[2] != expected_code)
  {
    return PD42S1_RESULT_CODE_ERROR;
  }

  data_length = frame_length - 5U;
  if (data_length > PD42S1_MAX_DATA_LENGTH)
  {
    return PD42S1_RESULT_FRAME_TOO_LONG;
  }

  reply->addr = frame[1];
  reply->code = frame[2];
  reply->data_length = (uint8_t)data_length;
  if (data_length != 0U)
  {
    (void)memcpy(reply->data, &frame[3], data_length);
  }

  return PD42S1_RESULT_OK;
}

static PD42S1_Result_t PD42S1_CheckOperationOk(const PD42S1_Reply_t *reply)
{
  if ((reply == NULL) || (reply->data_length == 0U))
  {
    return PD42S1_RESULT_BAD_FRAME;
  }

  return (reply->data[0] == 0x01U) ? PD42S1_RESULT_OK : PD42S1_RESULT_OPERATION_FAILED;
}

static PD42S1_Result_t PD42S1_SendOperation(uint8_t code,
                                            const uint8_t *data,
                                            uint8_t data_length)
{
  PD42S1_Reply_t reply;
  PD42S1_Result_t result = PD42S1_SendCommand(code, data, data_length, 1U, &reply);

  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  return PD42S1_CheckOperationOk(&reply);
}

PD42S1_Result_t PD42S1_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t uart_clock_hz;

  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_USART2_CLK_ENABLE();
  __HAL_RCC_USART2_CONFIG(RCC_USART2CLKSOURCE_PCLK1);

  GPIO_InitStruct.Pin = PD42S1_TX_PIN | PD42S1_RX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = PD42S1_GPIO_AF;
  HAL_GPIO_Init(PD42S1_GPIO_PORT, &GPIO_InitStruct);

  PD42S1_USART->CR1 &= ~USART_CR1_UE;
  PD42S1_USART->CR1 = 0U;
  PD42S1_USART->CR2 = 0U;
  PD42S1_USART->CR3 = 0U;

  /* USART2 使用 115200、8 数据位、1 停止位、无校验。 */
  uart_clock_hz = HAL_RCC_GetPCLK1Freq();
  PD42S1_USART->BRR = (uart_clock_hz + (PD42S1_BAUDRATE / 2U)) / PD42S1_BAUDRATE;
  PD42S1_USART->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_TCCF;
  PD42S1_USART->CR1 = USART_CR1_TE | USART_CR1_RE | USART_CR1_UE;

  pd42s1_ready = 1U;
  PD42S1_FlushRx();
  return PD42S1_RESULT_OK;
}

void PD42S1_DeInit(void)
{
  PD42S1_USART->CR1 &= ~USART_CR1_UE;
  __HAL_RCC_USART2_CLK_DISABLE();
  HAL_GPIO_DeInit(PD42S1_GPIO_PORT, PD42S1_TX_PIN | PD42S1_RX_PIN);
  pd42s1_ready = 0U;
}

uint8_t PD42S1_IsReady(void)
{
  return pd42s1_ready;
}

void PD42S1_SetAddress(uint8_t addr)
{
  pd42s1_addr = addr;
}

uint8_t PD42S1_GetAddress(void)
{
  return pd42s1_addr;
}

PD42S1_Result_t PD42S1_BuildFrame(uint8_t addr,
                                  uint8_t code,
                                  const uint8_t *data,
                                  uint8_t data_length,
                                  uint8_t *frame,
                                  size_t frame_size,
                                  size_t *frame_length)
{
  size_t total_length = (size_t)data_length + 5U;

  if ((frame == NULL) || (frame_length == NULL))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  if ((data_length != 0U) && (data == NULL))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  if ((data_length > PD42S1_MAX_DATA_LENGTH) || (total_length > frame_size))
  {
    return PD42S1_RESULT_FRAME_TOO_LONG;
  }

  frame[0] = PD42S1_FRAME_HEAD;
  frame[1] = addr;
  frame[2] = code;
  if (data_length != 0U)
  {
    (void)memcpy(&frame[3], data, data_length);
  }
  frame[3U + data_length] = PD42S1_CalcChecksum(frame, 3U + data_length);
  frame[4U + data_length] = PD42S1_FRAME_TAIL;
  *frame_length = total_length;

  return PD42S1_RESULT_OK;
}

PD42S1_Result_t PD42S1_SendCommand(uint8_t code,
                                   const uint8_t *data,
                                   uint8_t data_length,
                                   uint8_t expect_reply,
                                   PD42S1_Reply_t *reply)
{
  uint8_t frame[PD42S1_MAX_FRAME_LENGTH];
  size_t frame_length = 0U;
  size_t expected_reply_length;
  PD42S1_Result_t result;

  if (pd42s1_ready == 0U)
  {
    return PD42S1_RESULT_NOT_READY;
  }

  result = PD42S1_BuildFrame(pd42s1_addr,
                             code,
                             data,
                             data_length,
                             frame,
                             sizeof(frame),
                             &frame_length);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  PD42S1_ApplyCommandGap();
  PD42S1_FlushRx();

  result = PD42S1_SendBytes(frame, frame_length);
  pd42s1_last_command_tick = HAL_GetTick();
  if ((result != PD42S1_RESULT_OK) || (expect_reply == 0U))
  {
    return result;
  }

  if (reply == NULL)
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  expected_reply_length = PD42S1_GetExpectedReplyLength(code);
  result = PD42S1_ReadFrame(frame, sizeof(frame), &frame_length, expected_reply_length);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  return PD42S1_ParseReply(frame, frame_length, code, reply);
}

PD42S1_Result_t PD42S1_EnableMotor(void)
{
  uint8_t data[1] = {0x00U};

  return PD42S1_SendOperation(PD42S1_CODE_ENABLE_MOTOR, data, sizeof(data));
}

PD42S1_Result_t PD42S1_DisableMotor(void)
{
  uint8_t data[1] = {0x01U};

  return PD42S1_SendOperation(PD42S1_CODE_ENABLE_MOTOR, data, sizeof(data));
}

PD42S1_Result_t PD42S1_ClearPosition(void)
{
  return PD42S1_SendOperation(PD42S1_CODE_CLEAR_POSITION, NULL, 0U);
}

PD42S1_Result_t PD42S1_ClearState(void)
{
  return PD42S1_SendOperation(PD42S1_CODE_CLEAR_STATE, NULL, 0U);
}

PD42S1_Result_t PD42S1_StopMotor(void)
{
  return PD42S1_SendOperation(PD42S1_CODE_STOP_MOTOR, NULL, 0U);
}

PD42S1_Result_t PD42S1_SetSpeed(PD42S1_Direction_t direction,
                                uint16_t accel,
                                uint16_t rpm)
{
  uint8_t data[6];

  if ((direction > PD42S1_DIRECTION_CCW) ||
      (PD42S1_IsMotionArgsValid(accel, rpm) == 0U))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  data[0] = (uint8_t)direction;
  data[1] = (uint8_t)accel;
  /* 0xF1 速度模式：方向、加减速度、float 转速。 */
  PD42S1_PutFloatBe(&data[2], (float)rpm);

  return PD42S1_SendOperation(PD42S1_CODE_SET_SPEED, data, sizeof(data));
}

PD42S1_Result_t PD42S1_MoveRelative(PD42S1_Direction_t direction,
                                    uint16_t accel,
                                    uint16_t rpm,
                                    float revolutions)
{
  uint8_t data[8];
  uint32_t pulses;
  PD42S1_Result_t result;

  if ((direction > PD42S1_DIRECTION_CCW) ||
      (PD42S1_IsMotionArgsValid(accel, rpm) == 0U))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  result = PD42S1_RevolutionsToPulses(revolutions, &pulses);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  data[0] = (uint8_t)direction;
  data[1] = (uint8_t)accel;
  /* 0xF3 相对位置：方向、加减速度、uint16 转速、uint32 脉冲数。 */
  PD42S1_PutU16Be(&data[2], rpm);
  PD42S1_PutU32Be(&data[4], pulses);

  return PD42S1_SendOperation(PD42S1_CODE_MOVE_RELATIVE, data, sizeof(data));
}

PD42S1_Result_t PD42S1_MoveAbsolute(PD42S1_Direction_t direction,
                                    uint16_t accel,
                                    uint16_t rpm,
                                    float revolutions)
{
  uint8_t data[8];
  uint32_t pulses;
  PD42S1_Result_t result;

  if ((direction > PD42S1_DIRECTION_CCW) ||
      (PD42S1_IsMotionArgsValid(accel, rpm) == 0U))
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  result = PD42S1_RevolutionsToPulses(revolutions, &pulses);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  data[0] = (uint8_t)direction;
  data[1] = (uint8_t)accel;
  /* 0xF2 绝对位置：方向、加减速度、uint16 转速、uint32 脉冲数。 */
  PD42S1_PutU16Be(&data[2], rpm);
  PD42S1_PutU32Be(&data[4], pulses);

  return PD42S1_SendOperation(PD42S1_CODE_MOVE_ABSOLUTE, data, sizeof(data));
}

PD42S1_Result_t PD42S1_ReadPosition(int32_t *position)
{
  PD42S1_Reply_t reply;
  PD42S1_Result_t result;
  uint32_t raw_position;

  if (position == NULL)
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  result = PD42S1_SendCommand(PD42S1_CODE_READ_POSITION, NULL, 0U, 1U, &reply);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  result = PD42S1_CheckOperationOk(&reply);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  if (reply.data_length < 5U)
  {
    return PD42S1_RESULT_BAD_FRAME;
  }

  raw_position = PD42S1_GetU32Be(&reply.data[1]);
  *position = (int32_t)raw_position;
  return PD42S1_RESULT_OK;
}

PD42S1_Result_t PD42S1_ReadStatus(uint8_t *status)
{
  PD42S1_Reply_t reply;
  PD42S1_Result_t result;

  if (status == NULL)
  {
    return PD42S1_RESULT_INVALID_ARG;
  }

  result = PD42S1_SendCommand(PD42S1_CODE_READ_STATUS, NULL, 0U, 1U, &reply);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  result = PD42S1_CheckOperationOk(&reply);
  if (result != PD42S1_RESULT_OK)
  {
    return result;
  }

  if (reply.data_length < 2U)
  {
    return PD42S1_RESULT_BAD_FRAME;
  }

  *status = reply.data[1];
  return PD42S1_RESULT_OK;
}

uint32_t PD42S1_CalcMoveTimeoutMs(uint16_t rpm, float revolutions)
{
  float timeout_ms;

  if ((rpm == 0U) || (revolutions <= 0.0f))
  {
    return PD42S1_MIN_MOVE_TIMEOUT_MS;
  }

  /* 运动时间约为 圈数 / RPM * 60s，额外留 20% 和固定余量给加减速过程。 */
  timeout_ms = (revolutions * 60000.0f) / (float)rpm;
  timeout_ms += (timeout_ms * 0.2f) + (float)PD42S1_MOVE_TIMEOUT_MARGIN_MS;

  if (timeout_ms < (float)PD42S1_MIN_MOVE_TIMEOUT_MS)
  {
    return PD42S1_MIN_MOVE_TIMEOUT_MS;
  }
  if (timeout_ms >= 4294967295.0f)
  {
    return 0xFFFFFFFFU;
  }

  return (uint32_t)(timeout_ms + 0.5f);
}

PD42S1_Result_t PD42S1_WaitUntilArrived(uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    PD42S1_Reply_t reply;
    PD42S1_Result_t result = PD42S1_SendCommand(PD42S1_CODE_READ_ARRIVED,
                                                NULL,
                                                0U,
                                                1U,
                                                &reply);
    if (result != PD42S1_RESULT_OK)
    {
      return result;
    }

    result = PD42S1_CheckOperationOk(&reply);
    if (result != PD42S1_RESULT_OK)
    {
      return result;
    }

    if (reply.data_length < 2U)
    {
      return PD42S1_RESULT_BAD_FRAME;
    }

    if (reply.data[1] != 0U)
    {
      return PD42S1_RESULT_OK;
    }

    HAL_Delay(PD42S1_WAIT_POLL_MS);
  }

  return PD42S1_RESULT_WAIT_TIMEOUT;
}

const char *PD42S1_ResultToString(PD42S1_Result_t result)
{
  switch (result)
  {
    case PD42S1_RESULT_OK:
      return "OK";
    case PD42S1_RESULT_NOT_READY:
      return "NOT_READY";
    case PD42S1_RESULT_INVALID_ARG:
      return "INVALID_ARG";
    case PD42S1_RESULT_FRAME_TOO_LONG:
      return "FRAME_LONG";
    case PD42S1_RESULT_TX_TIMEOUT:
      return "TX_TIMEOUT";
    case PD42S1_RESULT_RX_TIMEOUT:
      return "RX_TIMEOUT";
    case PD42S1_RESULT_WAIT_TIMEOUT:
      return "WAIT_TIMEOUT";
    case PD42S1_RESULT_BAD_FRAME:
      return "BAD_FRAME";
    case PD42S1_RESULT_CHECKSUM_ERROR:
      return "BAD_SUM";
    case PD42S1_RESULT_ADDR_ERROR:
      return "BAD_ADDR";
    case PD42S1_RESULT_CODE_ERROR:
      return "BAD_CODE";
    case PD42S1_RESULT_OPERATION_FAILED:
      return "OP_FAIL";
    default:
      return "UNKNOWN";
  }
}
