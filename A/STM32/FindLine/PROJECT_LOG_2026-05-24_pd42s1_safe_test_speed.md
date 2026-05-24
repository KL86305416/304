# 项目日志：降低 PD42S1 测试默认速度

## 背景

- 之前主菜单 `PD42S1 Test` 使用 600RPM、加速度 100、相对运动 1 圈。
- 600RPM 等于每秒 10 圈，作为首次联调测试过于激进。

## 改动

- 在 `Core/Inc/pd42s1.h` 新增测试安全默认参数：
  - `PD42S1_TEST_SAFE_ACCEL = 20U`
  - `PD42S1_TEST_SAFE_RPM = 60U`
  - `PD42S1_TEST_SAFE_REVOLUTIONS = 0.10f`
  - `PD42S1_TEST_SAFE_TIMEOUT_MS = 5000U`
- `Core/Src/main.c` 的主菜单测试流程改用上述安全参数。
- `Core/Src/pd42s1.c` 的 `PD42S1_RunMinimalTest()` 同步改用上述安全参数。
- OLED 测试页新增显示当前测试 `RPM` 和 `ACC`。
- 修正 PD42S1 运动数据打包顺序：
  - 原来为 `direction + accel + rpm + pulses`
  - 现在为 `direction + rpm + accel + pulses`
  - `PD42S1_SetSpeed()` 同步改为 `direction + rpm + accel`

## 调速方法

- 相对运动速度由 `PD42S1_MoveRelative(direction, accel, rpm, revolutions)` 的第三个参数 `rpm` 决定。
- 速度模式由 `PD42S1_SetSpeed(direction, accel, rpm)` 的第三个参数 `rpm` 决定。
- 主菜单测试默认转速请改 `Core/Inc/pd42s1.h` 中的 `PD42S1_TEST_SAFE_RPM`。
- 修改宏后需要重新编译并重新烧录固件。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译和链接通过，生成 `FindLine.elf`。
- 修正字段顺序后再次执行 `cmake --build build\Debug`，编译和链接通过。
