# 2026-05-25 恢复 OpenMV 控制文件为空 Update 框架

## 修改内容

- 将 `Core/Src/openmv_tracker_control.c` 中的 `OpenMV_TrackerControl_Update()` 恢复为空实现。
- 移除该函数内的 OpenMV 数据接收、`err_x` 计算和 PD42S1 速度控制逻辑。
- 保留 `OpenMV_TrackerControl_Init()`、`OpenMV_TrackerControl_DeInit()` 和 `OpenMV_TrackerControl_GetStatus()`。
- 保留 `openmv_tracker_control.c/.h` 文件本身，方便后续重新编写控制策略。

## 验证结果

- 已执行 `cmake --build build\Debug`。
- 编译、链接通过。
