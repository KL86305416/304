#ifndef __OLED_H__
#define __OLED_H__

#ifdef __cplusplus
extern "C" {
#endif

#include "main.h"

#include <stdint.h>

#define OLED_WIDTH        128U
#define OLED_HEIGHT       64U
#define OLED_I2C_ADDRESS  (0x3CU << 1)

typedef enum
{
  OLED_COLOR_BLACK = 0,
  OLED_COLOR_WHITE = 1
} OLED_Color_t;

HAL_StatusTypeDef OLED_Init(void);
HAL_StatusTypeDef OLED_UpdateScreen(void);
HAL_StatusTypeDef OLED_DisplayOn(void);
HAL_StatusTypeDef OLED_DisplayOff(void);
HAL_StatusTypeDef OLED_SetContrast(uint8_t contrast);

void OLED_Fill(OLED_Color_t color);
void OLED_Clear(void);
void OLED_DrawPixel(uint8_t x, uint8_t y, OLED_Color_t color);
void OLED_SetCursor(uint8_t x, uint8_t y);
void OLED_WriteChar(char ch, OLED_Color_t color);
void OLED_WriteString(const char *str, OLED_Color_t color);

#ifdef __cplusplus
}
#endif

#endif /* __OLED_H__ */
