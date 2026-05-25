# 2026-05-25 修复 OpenMV 接收一直 RX_TIMEOUT

## 问题

- 进入 `OpenMV Track` 后 OLED 一直显示 `RX_TIMEOUT`。
- 原因是接收窗口只有 `8ms`，并且色块帧解析状态没有跨主循环保存。
- OpenMV 做图像处理后帧周期通常大于 8ms，如果数据刚好在 STM32 主循环延时、OLED 刷新或电机命令期间到达，STM32 容易丢掉部分字节，下一轮又重新找帧头，导致一直无法拼出完整 21 字节帧。

## 修改内容

- 将 `APP_OPENMV_TRACK_RX_TIMEOUT_MS` 从 `8ms` 调整为 `50ms`。
- `OpenMV_UART_ReceiveBlobFrame()` 改为跨调用保留帧解析状态。
- 校验失败或帧类型错误时不立即退出，而是在超时时间内继续寻找下一帧。
- 增加 OpenMV 串口诊断计数：
  - `OpenMV_UART_GetRxByteCount()`
  - `OpenMV_UART_GetRxFrameCount()`
- OLED 的 `OpenMV Track` 页面新增：
  - `B`：收到的字节数
  - `F`：成功解析的帧数

## 判断方法

- `B` 一直是 `0`：STM32 没收到任何串口字节，优先检查接线、OpenMV 是否运行脚本、串口号和共地。
- `B` 增加但 `F` 一直是 `0`：说明有串口数据，但帧格式、波特率、接线噪声或 OpenMV 脚本协议可能不匹配。
- `F` 增加：说明通信和解析正常。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3440 B / 32 KB
  - FLASH：28640 B / 128 KB
