# 删除 PD42S1 调试菜单并保留驱动日志

## 修改时间

2026-05-24

## 背景

PD42S1 调试菜单已经完成验证，电机可以按预期平滑转动。后续工程不再需要主界面中的临时调试入口，只需要保留电机驱动文件，供正式业务逻辑调用。

## 本次修改

- 从 `Core/Src/main.c` 移除 `Motor Debug` 主菜单项。
- 移除主程序中 PD42S1 调试相关的状态枚举、宏定义、变量、函数声明和函数实现。
- 移除主程序对 `pd42s1.h` 的包含，避免主界面残留未使用的电机调试依赖。
- 保留 `Core/Inc/pd42s1.h` 和 `Core/Src/pd42s1.c`，继续作为独立电机驱动库。
- 保留 `cmake/stm32cubemx/CMakeLists.txt` 中的 `pd42s1.c` 构建引用，确保驱动文件仍在工程中。

## 验证

- 已确认 `Core/Src/main.c` 中无 `PD42S1`、`pd42s1`、`Motor Debug`、`MOTOR_DEBUG` 等残留引用。
- 已执行 `cmake --build build\Debug`，构建通过。
- 构建结果：RAM 使用 3360 B，FLASH 使用 23088 B。
