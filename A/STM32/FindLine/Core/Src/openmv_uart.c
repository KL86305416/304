#include "openmv_uart.h"

#define OPENMV_UART             USART1
#define OPENMV_UART_GPIO_PORT   GPIOC
#define OPENMV_UART_RX_PIN      GPIO_PIN_5
#define OPENMV_UART_GPIO_AF     GPIO_AF7_USART1
#define OPENMV_BLOB_HEAD_0      0xAAU
#define OPENMV_BLOB_HEAD_1      0x55U
#define OPENMV_BLOB_TYPE        0x11U
#define OPENMV_BLOB_COLOR_BLUE  0x42U

static uint8_t openmv_uart_ready = 0U;
static uint32_t openmv_uart_rx_byte_count = 0U;

static uint8_t OpenMV_UART_CalcBlobChecksum(const uint8_t *raw)
{
  uint32_t sum = 0U;

  for (uint8_t i = 2U; i < 20U; i++)
  {
    sum += raw[i];
  }

  return (uint8_t)(sum & 0xFFU);
}

static uint16_t OpenMV_UART_GetU16Le(const uint8_t *data)
{
  return (uint16_t)data[0] | ((uint16_t)data[1] << 8);
}

static uint32_t OpenMV_UART_GetU32Le(const uint8_t *data)
{
  return (uint32_t)data[0] |
         ((uint32_t)data[1] << 8) |
         ((uint32_t)data[2] << 16) |
         ((uint32_t)data[3] << 24);
}

static void OpenMV_UART_ClearErrors(void)
{
  uint32_t flags = OPENMV_UART->ISR;

  if ((flags & USART_ISR_ORE) != 0U)
  {
    OPENMV_UART->ICR = USART_ICR_ORECF;
  }
  if ((flags & USART_ISR_FE) != 0U)
  {
    OPENMV_UART->ICR = USART_ICR_FECF;
  }
  if ((flags & USART_ISR_NE) != 0U)
  {
    OPENMV_UART->ICR = USART_ICR_NECF;
  }
}

OpenMV_UART_Result_t OpenMV_UART_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  uint32_t uart_clock_hz;

  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_USART1_CLK_ENABLE();
  __HAL_RCC_USART1_CONFIG(RCC_USART1CLKSOURCE_PCLK2);

  GPIO_InitStruct.Pin = OPENMV_UART_RX_PIN;
  GPIO_InitStruct.Mode = GPIO_MODE_AF_PP;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_HIGH;
  GPIO_InitStruct.Alternate = OPENMV_UART_GPIO_AF;
  HAL_GPIO_Init(OPENMV_UART_GPIO_PORT, &GPIO_InitStruct);

  OPENMV_UART->CR1 &= ~USART_CR1_UE;
  OPENMV_UART->CR1 = 0U;
  OPENMV_UART->CR2 = 0U;
  OPENMV_UART->CR3 = 0U;

  /* OpenMV 串口接收：PC5=USART1_RX，115200，8N1，无校验。 */
  uart_clock_hz = HAL_RCC_GetPCLK2Freq();
  OPENMV_UART->BRR = (uart_clock_hz + (OPENMV_UART_BAUDRATE / 2U)) / OPENMV_UART_BAUDRATE;
  OPENMV_UART->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NECF | USART_ICR_TCCF;
  OPENMV_UART->CR1 = USART_CR1_RE | USART_CR1_UE;

  openmv_uart_ready = 1U;
  OpenMV_UART_FlushRx();
  return OPENMV_UART_OK;
}

void OpenMV_UART_DeInit(void)
{
  OPENMV_UART->CR1 &= ~USART_CR1_UE;
  __HAL_RCC_USART1_CLK_DISABLE();
  HAL_GPIO_DeInit(OPENMV_UART_GPIO_PORT, OPENMV_UART_RX_PIN);
  openmv_uart_ready = 0U;
}

uint8_t OpenMV_UART_IsReady(void)
{
  return openmv_uart_ready;
}

void OpenMV_UART_FlushRx(void)
{
  OpenMV_UART_ClearErrors();

  while ((OPENMV_UART->ISR & USART_ISR_RXNE_RXFNE) != 0U)
  {
    (void)OPENMV_UART->RDR;
  }
}

uint32_t OpenMV_UART_GetRxByteCount(void)
{
  return openmv_uart_rx_byte_count;
}

OpenMV_UART_Result_t OpenMV_UART_ReceiveByte(uint8_t *byte, uint32_t timeout_ms)
{
  uint32_t start_tick = HAL_GetTick();

  if (openmv_uart_ready == 0U)
  {
    return OPENMV_UART_NOT_READY;
  }

  if (byte == NULL)
  {
    return OPENMV_UART_INVALID_ARG;
  }

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    OpenMV_UART_ClearErrors();
    if ((OPENMV_UART->ISR & USART_ISR_RXNE_RXFNE) != 0U)
    {
      *byte = (uint8_t)(OPENMV_UART->RDR & 0xFFU);
      openmv_uart_rx_byte_count++;
      return OPENMV_UART_OK;
    }
  }

  return OPENMV_UART_RX_TIMEOUT;
}

OpenMV_UART_Result_t OpenMV_UART_ReceiveBytes(uint8_t *data,
                                              size_t length,
                                              uint32_t timeout_ms)
{
  size_t index = 0U;
  uint32_t start_tick = HAL_GetTick();

  if ((data == NULL) || (length == 0U))
  {
    return OPENMV_UART_INVALID_ARG;
  }

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    OpenMV_UART_Result_t result = OpenMV_UART_ReceiveByte(&data[index], 1U);

    if (result == OPENMV_UART_RX_TIMEOUT)
    {
      continue;
    }
    if (result != OPENMV_UART_OK)
    {
      return result;
    }

    index++;
    if (index >= length)
    {
      return OPENMV_UART_OK;
    }
  }

  return OPENMV_UART_RX_TIMEOUT;
}

OpenMV_UART_Result_t OpenMV_UART_ReceiveLine(char *line, size_t line_size, uint32_t timeout_ms)
{
  size_t index = 0U;
  uint32_t start_tick = HAL_GetTick();

  if ((line == NULL) || (line_size == 0U))
  {
    return OPENMV_UART_INVALID_ARG;
  }

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    uint8_t byte;
    OpenMV_UART_Result_t result = OpenMV_UART_ReceiveByte(&byte, 1U);

    if (result == OPENMV_UART_RX_TIMEOUT)
    {
      continue;
    }
    if (result != OPENMV_UART_OK)
    {
      line[index] = '\0';
      return result;
    }

    if ((byte == '\n') || (byte == '\r'))
    {
      line[index] = '\0';
      return OPENMV_UART_OK;
    }

    if (index < (line_size - 1U))
    {
      line[index++] = (char)byte;
    }
  }

  line[index] = '\0';
  return OPENMV_UART_RX_TIMEOUT;
}

OpenMV_UART_Result_t OpenMV_UART_ParseBlobFrame(const uint8_t *raw,
                                                size_t length,
                                                OpenMV_BlobData_t *blob)
{
  if ((raw == NULL) || (blob == NULL))
  {
    return OPENMV_UART_INVALID_ARG;
  }

  if (length != OPENMV_BLOB_FRAME_LENGTH)
  {
    return OPENMV_UART_BAD_FRAME;
  }

  if ((raw[0] != OPENMV_BLOB_HEAD_0) ||
      (raw[1] != OPENMV_BLOB_HEAD_1) ||
      (raw[2] != OPENMV_BLOB_TYPE) ||
      (raw[3] != OPENMV_BLOB_COLOR_BLUE))
  {
    return OPENMV_UART_BAD_FRAME;
  }

  if (OpenMV_UART_CalcBlobChecksum(raw) != raw[20])
  {
    return OPENMV_UART_CHECKSUM_ERROR;
  }

  blob->seq = raw[4];
  blob->detected = raw[5];
  blob->err_x = (int16_t)OpenMV_UART_GetU16Le(&raw[6]);
  blob->cx = OpenMV_UART_GetU16Le(&raw[8]);
  blob->cy = OpenMV_UART_GetU16Le(&raw[10]);
  blob->w = OpenMV_UART_GetU16Le(&raw[12]);
  blob->h = OpenMV_UART_GetU16Le(&raw[14]);
  blob->pixels = OpenMV_UART_GetU32Le(&raw[16]);

  return OPENMV_UART_OK;
}

OpenMV_UART_Result_t OpenMV_UART_ReceiveBlobData(OpenMV_BlobData_t *blob,
                                                 uint32_t timeout_ms)
{
  static uint8_t raw[OPENMV_BLOB_FRAME_LENGTH];
  static uint8_t index = 0U;
  uint32_t start_tick = HAL_GetTick();
  OpenMV_UART_Result_t last_error = OPENMV_UART_RX_TIMEOUT;

  if (openmv_uart_ready == 0U)
  {
    return OPENMV_UART_NOT_READY;
  }

  if (blob == NULL)
  {
    return OPENMV_UART_INVALID_ARG;
  }

  while ((HAL_GetTick() - start_tick) < timeout_ms)
  {
    uint8_t byte;
    OpenMV_UART_Result_t result = OpenMV_UART_ReceiveByte(&byte, 1U);

    if (result == OPENMV_UART_RX_TIMEOUT)
    {
      continue;
    }
    if (result != OPENMV_UART_OK)
    {
      return result;
    }

    if (index == 0U)
    {
      if (byte == OPENMV_BLOB_HEAD_0)
      {
        raw[index++] = byte;
      }
      continue;
    }

    if (index == 1U)
    {
      if (byte == OPENMV_BLOB_HEAD_1)
      {
        raw[index++] = byte;
      }
      else
      {
        index = (byte == OPENMV_BLOB_HEAD_0) ? 1U : 0U;
        raw[0] = byte;
      }
      continue;
    }

    raw[index++] = byte;
    if (index < OPENMV_BLOB_FRAME_LENGTH)
    {
      continue;
    }

    result = OpenMV_UART_ParseBlobFrame(raw, sizeof(raw), blob);
    index = 0U;
    if (result == OPENMV_UART_OK)
    {
      return OPENMV_UART_OK;
    }

    last_error = result;
  }

  return last_error;
}

const char *OpenMV_UART_ResultToString(OpenMV_UART_Result_t result)
{
  switch (result)
  {
    case OPENMV_UART_OK:
      return "OK";
    case OPENMV_UART_NOT_READY:
      return "NOT_READY";
    case OPENMV_UART_INVALID_ARG:
      return "INVALID_ARG";
    case OPENMV_UART_RX_TIMEOUT:
      return "RX_TIMEOUT";
    case OPENMV_UART_BAD_FRAME:
      return "BAD_FRAME";
    case OPENMV_UART_CHECKSUM_ERROR:
      return "BAD_SUM";
    default:
      return "UNKNOWN";
  }
}
