# 2026-05-25 删除 OpenMV 色块追踪，仅保留串口接收

## 修改内容

- 删除主菜单中的 `OpenMV Track` 选项。
- 删除 `main.c` 中 OpenMV 色块追踪状态变量、OLED 显示和电机联动控制逻辑。
- 删除 `main.c` 对 `pd42s1.h` 的引用；PD42S1 驱动文件仍保留在工程中。
- 精简 `openmv_uart.h/.c`：
  - 删除 `OpenMV_BlobFrame_t`
  - 删除 `OpenMV_UART_ReceiveBlobFrame()`
  - 删除色块帧头、校验和、协议类型等专用解析逻辑
  - 删除发送接口，仅保留串口接收相关接口
- USART1 现在仅作为 OpenMV 接收口使用：
  - `PC5 = USART1_RX`
  - `115200，8N1，无校验`

## 保留接口

- `OpenMV_UART_Init()`
- `OpenMV_UART_DeInit()`
- `OpenMV_UART_IsReady()`
- `OpenMV_UART_FlushRx()`
- `OpenMV_UART_GetRxByteCount()`
- `OpenMV_UART_ReceiveByte()`
- `OpenMV_UART_ReceiveBytes()`
- `OpenMV_UART_ReceiveLine()`
- `OpenMV_UART_ResultToString()`

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3368 B / 32 KB
  - FLASH：23472 B / 128 KB
