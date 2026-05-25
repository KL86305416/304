# 2026-05-24 OpenMV 色块追踪控制 PD42S1

## 修改内容

- 在 `openmv_uart.h/.c` 中新增 OpenMV 色块二进制帧解析接口：
  - `OpenMV_BlobFrame_t`
  - `OpenMV_UART_ReceiveBlobFrame()`
- 按 OpenMV 脚本的数据格式解析 21 字节固定帧：
  - 帧头：`0xAA 0x55`
  - 类型：`0x11`
  - 色块标识：`0x42`
  - 小端解析 `err_x`、`cx`、`cy`、`w`、`h`、`pixels`
  - 校验 `Byte2` 到 `Byte19` 的累加和低 8 位
- 在主菜单新增 `OpenMV Track` 选项。
- 进入 `OpenMV Track` 后：
  - 初始化并清空 OpenMV 串口接收缓存
  - 初始化并使能 PD42S1
  - 根据 `err_x` 控制电机顺时针或逆时针连续转动
  - 目标接近画面中心或丢失目标时停止电机
  - 退出菜单时停止并失能电机
- 追踪速度采用保守参数：
  - 死区：`12px`
  - 最小速度：`40RPM`
  - 最大速度：`300RPM`
  - 加减速度：`80`

## 使用说明

- OpenMV TX 接 STM32 PC5。
- OpenMV RX 接 STM32 PC4。
- 两边必须共地。
- 如果发现目标在右边时电机反而往左追，需要交换：
  - `APP_OPENMV_TRACK_RIGHT_DIRECTION`
  - `APP_OPENMV_TRACK_LEFT_DIRECTION`

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3408 B / 32 KB
  - FLASH：28452 B / 128 KB
