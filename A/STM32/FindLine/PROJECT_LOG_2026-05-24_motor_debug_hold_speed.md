# PD42S1 按住匀速调试日志

## 修改时间

2026-05-24

## 背景

原 `Motor Debug` 菜单采用相对位置模式，按一次按键运动 5 度，实际手感有明显段落感。现在调试云台底座需要按住按键时电机以固定速度连续、平滑转动，松开按键后停止。

## 本次修改

- 新增按键保持状态读取函数 `App_ButtonGetPressedMask()`，用于识别 PC3/PC2 是否持续按住。
- 将 `Motor Debug` 从相对位置单步控制改为速度模式控制：
  - 按住 PC3：调用 `PD42S1_SetSpeed()` 顺时针匀速转动。
  - 按住 PC2：调用 `PD42S1_SetSpeed()` 逆时针匀速转动。
  - 松开按键或 PC3/PC2 同时按下：发送 0RPM 停止，必要时回退到停止指令。
- 移除调试菜单中的 5 度单步位置逻辑。
- OLED 显示当前速度、加速度和运行方向状态。

## 当前参数

- `APP_MOTOR_DEBUG_RPM = 60U`
- `APP_MOTOR_DEBUG_ACCEL = 20U`

## 验证

- 已执行 `cmake --build build\Debug`，构建通过。
- 构建结果：RAM 使用 3368 B，FLASH 使用 27420 B。

## 后续调整

如需改变转动速度，可在 `Core/Src/main.c` 修改 `APP_MOTOR_DEBUG_RPM`；如需改变启停柔和程度，可调整 `APP_MOTOR_DEBUG_ACCEL`。
