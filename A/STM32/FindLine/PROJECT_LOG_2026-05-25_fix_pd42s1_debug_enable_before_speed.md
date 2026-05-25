# 2026-05-25 修复 PD42S1 Debug 进入菜单后电机不转

## 问题

- `Motor Debug` 菜单进入后，电机没有转动。

## 原因

- `PD42S1_Debug_Init()` 只调用了 `PD42S1_Init()`。
- `PD42S1_Init()` 只负责初始化 USART2 和 PA2/PA3，不会使能电机。
- 后续虽然调用了 `PD42S1_SetSpeed()`，但电机未使能，因此不会转动。

## 修改内容

- 在 `PD42S1_Debug_Init()` 中补充：
  - `PD42S1_EnableMotor()`
  - `PD42S1_ClearState()`
  - 首次 `PD42S1_SetSpeed()`
- `PD42S1_Debug_Update()` 改为每 `500ms` 刷新一次速度命令，避免主循环每 10ms 频繁发送串口指令。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3376 B / 32 KB
  - FLASH：25992 B / 128 KB
