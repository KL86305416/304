# 2026-05-25 添加色块追踪主菜单入口

## 修改内容

- 在 `Core/Src/main.c` 中新增 `Blob Track` 主菜单选项。
- 新增 `APP_SCREEN_BLOB_TRACK` 页面状态。
- 进入该菜单时调用 `OpenMV_TrackerControl_Init()`。
- 在该菜单页面循环调用 `OpenMV_TrackerControl_Update()`。
- 返回主菜单时调用 `OpenMV_TrackerControl_DeInit()`。
- OLED 进入页面后仅显示 `Blob Track`。

## 保持不变

- 未修改 `Core/Src/openmv_tracker_control.c`。
- 未修改 `Core/Inc/openmv_tracker_control.h`。
- 未修改 OpenMV 串口驱动和 PD42S1 电机驱动。

## 验证结果

- 已执行 `cmake --build build\Debug`。
- 编译、链接通过。
