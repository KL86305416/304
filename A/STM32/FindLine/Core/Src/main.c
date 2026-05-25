/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "i2c.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "gray_sensor.h"
#include "line_follower.h"
#include "line_follower_config.h"
#include "mpu6050.h"
#include "oled.h"
#include "openmv_tracker_control.h"
#include "openmv_uart.h"
#include "pd42s1_debug.h"
#include "stm32g431xx.h"
#include "stm32g4xx_hal_gpio.h"
#include "tb6612.h"

#include <stddef.h>
#include <stdio.h>

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum
{
  APP_SCREEN_MENU = 0,
  APP_SCREEN_LINE_FOLLOWER = 1,
  APP_SCREEN_MPU6050_DEBUG = 2,
  APP_SCREEN_PD42S1_DEBUG = 3,
  APP_SCREEN_BLOB_TRACK = 4
} App_Screen_t;

typedef enum
{
  APP_BUTTON_UP = 0,
  APP_BUTTON_DOWN,
  APP_BUTTON_OK,
  APP_BUTTON_BACK,
  APP_BUTTON_COUNT
} App_ButtonId_t;

typedef struct
{
  GPIO_TypeDef *port;
  uint16_t pin;
  GPIO_PinState stable_state;
  GPIO_PinState sample_state;
  uint32_t sample_tick;
} App_Button_t;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */
#define APP_BUTTON_ACTIVE_STATE  GPIO_PIN_RESET
#define APP_BUTTON_DEBOUNCE_MS   35U
#define APP_MENU_ITEM_COUNT      3U
#define APP_MPU6050_UPDATE_MS             100U
#define APP_MPU6050_RETRY_MS              1000U
#define APP_LINE_TRACKER_UPDATE_MS        50U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */
static uint8_t oled_ready = 0U;
static uint32_t oled_last_update_tick = 0U;
static App_Screen_t app_screen = APP_SCREEN_MENU;
static uint8_t menu_selected_index = 0U;
static uint8_t menu_dirty = 1U;
static MPU6050_Data_t mpu6050_data;
static MPU6050_Result_t mpu6050_status = MPU6050_RESULT_NOT_FOUND;
static uint32_t mpu6050_read_count = 0U;
static uint32_t mpu6050_error_count = 0U;
static uint32_t mpu6050_last_retry_tick = 0U;
static uint32_t line_tracker_last_update_tick = 0U;
static uint8_t mpu6050_int_pin = 0U;
static App_Button_t app_buttons[APP_BUTTON_COUNT] = {
  {GPIOC, GPIO_PIN_3, GPIO_PIN_SET, GPIO_PIN_SET, 0U},
  {GPIOC, GPIO_PIN_2, GPIO_PIN_SET, GPIO_PIN_SET, 0U},
  {GPIOC, GPIO_PIN_1, GPIO_PIN_SET, GPIO_PIN_SET, 0U},
  {GPIOC, GPIO_PIN_0, GPIO_PIN_SET, GPIO_PIN_SET, 0U}
};

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */
static void App_ButtonInit(void);
static uint8_t App_ButtonScan(void);
static void App_EnterMenu(void);
static void App_EnterLineFollower(void);
static void App_EnterPd42s1Debug(void);
static void App_EnterBlobTrack(void);
static void App_HandleMenuButtons(uint8_t button_events);
static void App_ShowMenuScreen(void);
static void App_ShowLineStatus(const LineFollower_Status_t *status);
static void App_ShowPd42s1DebugScreen(void);
static void App_ShowBlobTrackScreen(void);
static void App_UpdateMpu6050Debug(void);
static void App_ShowMpu6050DebugScreen(void);
static void App_FormatSignedX10(char *line,
                                 size_t line_size,
                                 const char *prefix,
                                 int32_t value_x10);

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  App_ButtonInit();
  GraySensor_Init();
  TB6612_Init();
  LineFollower_Init();
  MX_I2C2_Init();
  MX_I2C3_Init();
  (void)OpenMV_UART_Init();
  /* USER CODE BEGIN 2 */
  if (OLED_Init() == HAL_OK)
  {
    oled_ready = 1U;
    App_ShowMenuScreen();
  }
  oled_last_update_tick = HAL_GetTick();

  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
    uint32_t loop_delay_ms = LINE_FOLLOWER_CONTROL_PERIOD_MS;
    uint8_t button_events = App_ButtonScan();

    if (app_screen == APP_SCREEN_MENU)
    {
      App_HandleMenuButtons(button_events);

      if ((oled_ready != 0U) && (menu_dirty != 0U))
      {
        App_ShowMenuScreen();
      }
    }
    else
    {
      if ((button_events & (1U << APP_BUTTON_BACK)) != 0U)
      {
        App_EnterMenu();
      }
      else if (app_screen == APP_SCREEN_LINE_FOLLOWER)
      {
        LineFollower_Status_t line_status;
        uint32_t now = HAL_GetTick();

        line_status = LineFollower_Update();

        if ((now - line_tracker_last_update_tick) >= APP_LINE_TRACKER_UPDATE_MS)
        {
          OpenMV_TrackerControl_Update();
          line_tracker_last_update_tick = now;
        }

        if ((oled_ready != 0U) &&
            ((now - oled_last_update_tick) >= LINE_FOLLOWER_OLED_UPDATE_MS))
        {
          App_ShowLineStatus(&line_status);
          oled_last_update_tick = now;
        }
      }
      else if (app_screen == APP_SCREEN_MPU6050_DEBUG)
      {
        App_UpdateMpu6050Debug();
      }
      else if (app_screen == APP_SCREEN_PD42S1_DEBUG)
      {
        PD42S1_Debug_Update();
      }
      else if (app_screen == APP_SCREEN_BLOB_TRACK)
      {
        OpenMV_TrackerControl_Update();
      }
    }

    HAL_Delay(loop_delay_ms);
  }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  HAL_PWREx_ControlVoltageScaling(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = RCC_PLLM_DIV1;
  RCC_OscInitStruct.PLL.PLLN = 16;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV7;
  RCC_OscInitStruct.PLL.PLLQ = RCC_PLLQ_DIV2;
  RCC_OscInitStruct.PLL.PLLR = RCC_PLLR_DIV8;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */
static void App_ButtonInit(void)
{
  uint32_t now = HAL_GetTick();

  for (uint8_t i = 0U; i < APP_BUTTON_COUNT; i++)
  {
    GPIO_PinState state = HAL_GPIO_ReadPin(app_buttons[i].port, app_buttons[i].pin);

    app_buttons[i].stable_state = state;
    app_buttons[i].sample_state = state;
    app_buttons[i].sample_tick = now;
  }
}

static uint8_t App_ButtonScan(void)
{
  uint8_t events = 0U;
  uint32_t now = HAL_GetTick();

  for (uint8_t i = 0U; i < APP_BUTTON_COUNT; i++)
  {
    GPIO_PinState sample = HAL_GPIO_ReadPin(app_buttons[i].port, app_buttons[i].pin);

    if (sample != app_buttons[i].sample_state)
    {
      app_buttons[i].sample_state = sample;
      app_buttons[i].sample_tick = now;
    }
    else if (((now - app_buttons[i].sample_tick) >= APP_BUTTON_DEBOUNCE_MS) &&
             (sample != app_buttons[i].stable_state))
    {
      app_buttons[i].stable_state = sample;
      if (sample == APP_BUTTON_ACTIVE_STATE)
      {
        events |= (uint8_t)(1U << i);
      }
    }
  }

  return events;
}

static void App_EnterMenu(void)
{
  LineFollower_Stop();
  if (app_screen == APP_SCREEN_PD42S1_DEBUG)
  {
    PD42S1_Debug_DeInit();
  }
  else if (app_screen == APP_SCREEN_LINE_FOLLOWER)
  {
    OpenMV_TrackerControl_DeInit();
  }
  else if (app_screen == APP_SCREEN_BLOB_TRACK)
  {
    OpenMV_TrackerControl_DeInit();
  }
  app_screen = APP_SCREEN_MENU;
  menu_dirty = 1U;
}

static void App_EnterLineFollower(void)
{
  LineFollower_Init();
  OpenMV_TrackerControl_Init();
  app_screen = APP_SCREEN_LINE_FOLLOWER;
  oled_last_update_tick = 0U;
  line_tracker_last_update_tick = 0U;
}

static void App_EnterPd42s1Debug(void)
{
  LineFollower_Stop();
  PD42S1_Debug_Init();
  app_screen = APP_SCREEN_PD42S1_DEBUG;
  oled_last_update_tick = 0U;

  if (oled_ready != 0U)
  {
    App_ShowPd42s1DebugScreen();
  }
}

static void App_EnterBlobTrack(void)
{
  LineFollower_Stop();
  OpenMV_TrackerControl_Init();
  app_screen = APP_SCREEN_BLOB_TRACK;
  oled_last_update_tick = 0U;

  if (oled_ready != 0U)
  {
    App_ShowBlobTrackScreen();
  }
}

static void App_HandleMenuButtons(uint8_t button_events)
{
  if (((button_events & (1U << APP_BUTTON_UP)) != 0U) && (APP_MENU_ITEM_COUNT > 1U))
  {
    if (menu_selected_index == 0U)
    {
      menu_selected_index = APP_MENU_ITEM_COUNT - 1U;
    }
    else
    {
      menu_selected_index--;
    }
    menu_dirty = 1U;
  }

  if (((button_events & (1U << APP_BUTTON_DOWN)) != 0U) && (APP_MENU_ITEM_COUNT > 1U))
  {
    menu_selected_index++;
    if (menu_selected_index >= APP_MENU_ITEM_COUNT)
    {
      menu_selected_index = 0U;
    }
    menu_dirty = 1U;
  }

  if ((button_events & (1U << APP_BUTTON_OK)) != 0U)
  {
    if (menu_selected_index == 0U)
    {
      App_EnterLineFollower();
    }
    else if (menu_selected_index == 1U)
    {
      App_EnterPd42s1Debug();
    }
    else if (menu_selected_index == 2U)
    {
      App_EnterBlobTrack();
    }
  }
}

static void App_ShowMenuScreen(void)
{
  char line[22];

  OLED_Clear();
  OLED_SetCursor(0, 0);
  OLED_WriteString("Main Menu", OLED_COLOR_WHITE);

  (void)snprintf(line, sizeof(line), "%c Line Follow",
                 (menu_selected_index == 0U) ? '>' : ' ');
  OLED_SetCursor(0, 16);
  OLED_WriteString(line, OLED_COLOR_WHITE);

  (void)snprintf(line, sizeof(line), "%c Motor Debug",
                 (menu_selected_index == 1U) ? '>' : ' ');
  OLED_SetCursor(0, 24);
  OLED_WriteString(line, OLED_COLOR_WHITE);

  (void)snprintf(line, sizeof(line), "%c Blob Track",
                 (menu_selected_index == 2U) ? '>' : ' ');
  OLED_SetCursor(0, 32);
  OLED_WriteString(line, OLED_COLOR_WHITE);

  (void)OLED_UpdateScreen();

  menu_dirty = 0U;
}

static void App_ShowLineStatus(const LineFollower_Status_t *status)
{
  char line[22];

  OLED_Clear();
  OLED_SetCursor(0, 0);
  if (status->line_lost != 0U)
  {
    OLED_WriteString("LINE: LOST", OLED_COLOR_WHITE);
  }
  else
  {
    OLED_WriteString((status->curve_active != 0U) ? "LINE: CURVE" : "LINE: TRACK",
                     OLED_COLOR_WHITE);
  }

  (void)snprintf(line, sizeof(line), "RAW:0x%02X CNT:%u",
                 (unsigned int)status->raw_value,
                 (unsigned int)status->active_count);
  OLED_SetCursor(0, 8);
  OLED_WriteString(line, OLED_COLOR_WHITE);

  (void)snprintf(line, sizeof(line), "ERR:%4d", status->error_x100);
  OLED_SetCursor(0, 16);
  OLED_WriteString(line, OLED_COLOR_WHITE);

  (void)snprintf(line, sizeof(line), "L:%4d R:%4d",
                 status->left_speed,
                 status->right_speed);
  OLED_SetCursor(0, 24);
  OLED_WriteString(line, OLED_COLOR_WHITE);

  (void)OLED_UpdateScreen();
}

static void App_ShowPd42s1DebugScreen(void)
{
  OLED_Clear();
  OLED_SetCursor(0, 0);
  OLED_WriteString("Motor Debug", OLED_COLOR_WHITE);
  (void)OLED_UpdateScreen();
}

static void App_ShowBlobTrackScreen(void)
{
  OLED_Clear();
  OLED_SetCursor(0, 0);
  OLED_WriteString("Blob Track", OLED_COLOR_WHITE);
  (void)OLED_UpdateScreen();
}

static void App_UpdateMpu6050Debug(void)
{
  uint32_t now = HAL_GetTick();

  if ((mpu6050_status != MPU6050_RESULT_OK) &&
      ((now - mpu6050_last_retry_tick) >= APP_MPU6050_RETRY_MS))
  {
    mpu6050_status = MPU6050_Init();
    mpu6050_last_retry_tick = now;
  }

  if ((now - oled_last_update_tick) < APP_MPU6050_UPDATE_MS)
  {
    return;
  }

  mpu6050_int_pin = (HAL_GPIO_ReadPin(GPIOA, GPIO_PIN_12) == GPIO_PIN_SET) ? 1U : 0U;

  if (mpu6050_status == MPU6050_RESULT_OK)
  {
    mpu6050_status = MPU6050_ReadData(&mpu6050_data);
    if (mpu6050_status == MPU6050_RESULT_OK)
    {
      mpu6050_read_count++;
    }
    else
    {
      mpu6050_error_count++;
      mpu6050_last_retry_tick = now;
    }
  }

  if (oled_ready != 0U)
  {
    App_ShowMpu6050DebugScreen();
  }

  oled_last_update_tick = now;
}

static void App_ShowMpu6050DebugScreen(void)
{
  char line[22];

  OLED_Clear();
  OLED_SetCursor(0, 0);
  OLED_WriteString("MPU6050 Debug", OLED_COLOR_WHITE);

  if (mpu6050_status == MPU6050_RESULT_OK)
  {
    (void)snprintf(line, sizeof(line), "A:0x%02X W:0x%02X",
                   (unsigned int)mpu6050_data.address_7bit,
                   (unsigned int)mpu6050_data.who_am_i);
    OLED_SetCursor(0, 8);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    App_FormatSignedX10(line, sizeof(line), "GX", mpu6050_data.gyro_dps_x10_x);
    OLED_SetCursor(0, 16);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    App_FormatSignedX10(line, sizeof(line), "GY", mpu6050_data.gyro_dps_x10_y);
    OLED_SetCursor(0, 24);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    App_FormatSignedX10(line, sizeof(line), "GZ", mpu6050_data.gyro_dps_x10_z);
    OLED_SetCursor(0, 32);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "CNT:%lu",
                   (unsigned long)mpu6050_read_count);
    OLED_SetCursor(0, 40);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "INT:%u RDY:%u E:%lu",
                   (unsigned int)mpu6050_int_pin,
                   (unsigned int)(mpu6050_data.int_status & 0x01U),
                   (unsigned long)mpu6050_error_count);
    OLED_SetCursor(0, 48);
    OLED_WriteString(line, OLED_COLOR_WHITE);
  }
  else
  {
    (void)snprintf(line, sizeof(line), "ERR:%s",
                   MPU6050_ResultToString(mpu6050_status));
    OLED_SetCursor(0, 8);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    (void)snprintf(line, sizeof(line), "WHO:0x%02X A:0x%02X",
                   (unsigned int)MPU6050_GetWhoAmI(),
                   (unsigned int)MPU6050_GetAddress7Bit());
    OLED_SetCursor(0, 16);
    OLED_WriteString(line, OLED_COLOR_WHITE);

    OLED_SetCursor(0, 24);
    OLED_WriteString("SCL PC8 SDA PC9", OLED_COLOR_WHITE);

    OLED_SetCursor(0, 32);
    OLED_WriteString("VCC 5V GND", OLED_COLOR_WHITE);

    OLED_SetCursor(0, 40);
    OLED_WriteString("Addr 0x68/0x69", OLED_COLOR_WHITE);
  }

  (void)OLED_UpdateScreen();
}

static void App_FormatSignedX10(char *line,
                                 size_t line_size,
                                 const char *prefix,
                                 int32_t value_x10)
{
  char sign = ' ';
  uint32_t abs_value;

  if (value_x10 < 0)
  {
    sign = '-';
    abs_value = (uint32_t)(-value_x10);
  }
  else
  {
    abs_value = (uint32_t)value_x10;
  }

  (void)snprintf(line, line_size, "%s:%c%4lu.%lu dps",
                 prefix,
                 sign,
                 (unsigned long)(abs_value / 10U),
                 (unsigned long)(abs_value % 10U));
}

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
}
#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
