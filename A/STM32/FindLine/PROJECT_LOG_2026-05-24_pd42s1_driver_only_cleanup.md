# PD42S1 驱动化清理日志

## 修改时间

2026-05-24

## 背景

PD42S1 已验证可以稳定转动。后续需要把它作为云台底座等复杂机构的执行电机使用，因此不再需要主菜单里的自动测试入口和驱动库里的测试宏。

## 本次修改

- 从主菜单中移除 `PD42S1 Test` 测试项。
- 从 `main.c` 中移除 PD42S1 自动测试流程、测试屏幕显示函数和相关状态变量。
- 从 `pd42s1.h` 中移除 `PD42S1_MENU_AUTO_TEST_ENABLE`、`PD42S1_TEST_SAFE_*` 等测试宏。
- 从 `pd42s1.c` 中移除 `PD42S1_RunMinimalTest()`，只保留实际驱动接口。
- 保留 `Core/Inc/pd42s1.h` 和 `Core/Src/pd42s1.c` 作为 PD42S1 电机串口驱动库。

## 验证

- 已执行 `cmake --build build\Debug`，构建通过。
- 构建结果：RAM 使用 3360 B，FLASH 使用 23088 B。

## 后续使用建议

业务代码中直接包含 `pd42s1.h`，先初始化并使能电机，再调用 `PD42S1_MoveAbsolute()`、`PD42S1_MoveRelative()` 或 `PD42S1_SetSpeed()` 实现云台底座控制。
