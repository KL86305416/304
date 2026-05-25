# 2026-05-25 新增 OpenMV 色块数据解析接口

## 修改内容

- 在 `openmv_uart.h/.c` 中新增 OpenMV 色块数据解析结构体：
  - `OpenMV_BlobData_t`
- 新增原始帧解析接口：
  - `OpenMV_UART_ParseBlobFrame()`
- 新增串口接收并解析接口：
  - `OpenMV_UART_ReceiveBlobData()`
- 解析内容包括：
  - `seq`
  - `detected`
  - `err_x`
  - `cx`
  - `cy`
  - `w`
  - `h`
  - `pixels`
- 本次没有添加 OLED 显示。
- 本次没有添加电机控制逻辑。

## 协议格式

- 帧长度：21 字节
- 帧头：`0xAA 0x55`
- 类型：`0x11`
- 色块标识：`0x42`
- 校验：从 Byte2 到 Byte19 累加后取低 8 位
- 数据序：小端

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3368 B / 32 KB
  - FLASH：23472 B / 128 KB
