# OpenMV 串口配置日志

## 修改时间

2026-05-24

## 背景

需要新增一组串口用于 STM32 与 OpenMV 通信，指定引脚为 PC4 和 PC5。当前工程中 PC4/PC5 未被 GPIO、I2C、按键、电机驱动占用。

## 本次修改

- 新增 `Core/Inc/openmv_uart.h`。
- 新增 `Core/Src/openmv_uart.c`。
- 使用 `USART1`：
  - PC4：USART1_TX。
  - PC5：USART1_RX。
  - 复用功能：`GPIO_AF7_USART1`。
- 默认串口参数：
  - 波特率：115200。
  - 数据位：8 位。
  - 停止位：1 位。
  - 校验：无。
- 新增基础接口：
  - `OpenMV_UART_Init()`
  - `OpenMV_UART_DeInit()`
  - `OpenMV_UART_IsReady()`
  - `OpenMV_UART_FlushRx()`
  - `OpenMV_UART_SendBytes()`
  - `OpenMV_UART_SendString()`
  - `OpenMV_UART_ReceiveByte()`
  - `OpenMV_UART_ReceiveLine()`
  - `OpenMV_UART_ResultToString()`
- 在 `main.c` 初始化阶段调用 `OpenMV_UART_Init()`，上电后自动完成 PC4/PC5 串口配置。
- 将 `openmv_uart.c` 加入 `cmake/stm32cubemx/CMakeLists.txt` 构建列表。

## 验证

- 已执行 `cmake --build build\Debug`，构建通过。
- 构建结果：RAM 使用 3368 B，FLASH 使用 23472 B。

## 注意

本次没有修改 `.ioc` 文件。当前工程已有多个外设采用手写初始化方式，OpenMV 串口配置也集中在 `openmv_uart.c` 中，避免 CubeMX 重新生成时误覆盖业务代码。
