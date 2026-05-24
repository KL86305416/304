# 项目日志：PD42S1 串口控制库与蓝牙功能移除

## 目标

- 删除不再使用的蓝牙调试功能。
- 将 PA2/PA3 对应的 USART2 释放给 PD42S1 闭环步进电机使用。
- 新增 PD42S1 正点原子自定义协议控制库。
- 在主菜单中加入最小测试入口。

## 改动

- 删除 `Core/Src/bluetooth.c` 和 `Core/Inc/bluetooth.h`。
- 从 `cmake/stm32cubemx/CMakeLists.txt` 移除蓝牙源码，并加入 `Core/Src/pd42s1.c`。
- 新增 `Core/Inc/pd42s1.h` 和 `Core/Src/pd42s1.c`。
- PD42S1 使用 USART2：
  - PA2：USART2_TX
  - PA3：USART2_RX
  - 115200，8 数据位，1 停止位，无校验
  - 默认从机地址 `0x01`
- 实现协议帧：
  - 帧头 `0xC5`
  - `addr`
  - `code`
  - `data`
  - checksum
  - 帧尾 `0x5C`
- checksum 为从帧头到数据区所有字节累加后的低 8 位。
- 数据按大端序打包。
- `PD42S1_SendCommand()` 内部保证两条指令至少间隔 2ms。
- 返回帧包含帧头、帧尾、地址、功能码和 checksum 校验。
- 普通操作类指令会判断返回数据首字节是否为 `0x01`。

## 新增接口

- `PD42S1_BuildFrame()`
- `PD42S1_SendCommand()`
- `PD42S1_EnableMotor()`
- `PD42S1_DisableMotor()`
- `PD42S1_ClearState()`
- `PD42S1_StopMotor()`
- `PD42S1_SetSpeed()`
- `PD42S1_MoveRelative()`
- `PD42S1_MoveAbsolute()`
- `PD42S1_ReadPosition()`
- `PD42S1_ReadStatus()`
- `PD42S1_WaitUntilArrived()`
- `PD42S1_RunMinimalTest()`

## 主菜单

- 删除 `Bluetooth Debug`。
- 新增 `PD42S1 Test`。
- 测试流程：
  1. 初始化 USART2。
  2. 使能电机。
  3. 清除状态。
  4. 相对运动 1 圈，速度 600RPM，加减速度 100。
  5. 等待到位。
  6. 停止并失能。

## 注意

- 当前库中 `enable/disable/clear/stop` 功能码集中定义在 `pd42s1.h`，如果手册版本不同，只需要调整这些宏。
- 当前运动数据格式为：`direction` 1 字节、`accel` uint16、`rpm` uint16、`pulses` uint32。

## 验证

- 已执行 `cmake --build build\Debug`。
- CMake 重新生成成功。
- 编译和链接通过，生成 `FindLine.elf`。
