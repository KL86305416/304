#ifndef __OPENMV_UART_H__
#define __OPENMV_UART_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stddef.h>
#include <stdint.h>

#define OPENMV_UART_BAUDRATE       115200U
#define OPENMV_UART_RX_TIMEOUT_MS  20U
#define OPENMV_BLOB_FRAME_LENGTH   21U

typedef enum
{
  OPENMV_UART_OK = 0,
  OPENMV_UART_NOT_READY,
  OPENMV_UART_INVALID_ARG,
  OPENMV_UART_RX_TIMEOUT,
  OPENMV_UART_BAD_FRAME,
  OPENMV_UART_CHECKSUM_ERROR
} OpenMV_UART_Result_t;

typedef struct
{
  uint8_t seq;
  uint8_t detected;
  int16_t err_x;
  uint16_t cx;
  uint16_t cy;
  uint16_t w;
  uint16_t h;
  uint32_t pixels;
} OpenMV_BlobData_t;

OpenMV_UART_Result_t OpenMV_UART_Init(void);
void OpenMV_UART_DeInit(void);
uint8_t OpenMV_UART_IsReady(void);
void OpenMV_UART_FlushRx(void);
uint32_t OpenMV_UART_GetRxByteCount(void);
OpenMV_UART_Result_t OpenMV_UART_ReceiveByte(uint8_t *byte, uint32_t timeout_ms);
OpenMV_UART_Result_t OpenMV_UART_ReceiveBytes(uint8_t *data,
                                              size_t length,
                                              uint32_t timeout_ms);
OpenMV_UART_Result_t OpenMV_UART_ReceiveLine(char *line, size_t line_size, uint32_t timeout_ms);
OpenMV_UART_Result_t OpenMV_UART_ParseBlobFrame(const uint8_t *raw,
                                                size_t length,
                                                OpenMV_BlobData_t *blob);
OpenMV_UART_Result_t OpenMV_UART_ReceiveBlobData(OpenMV_BlobData_t *blob,
                                                 uint32_t timeout_ms);
const char *OpenMV_UART_ResultToString(OpenMV_UART_Result_t result);

#ifdef __cplusplus
}
#endif

#endif /* __OPENMV_UART_H__ */
