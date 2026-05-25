# 2026-05-25 添加色块追踪主菜单入口

## 修改内容

- 在 `Core/Src/main.c` 中新增 `Blob Track` 主菜单选项。
- 新增 `APP_SCREEN_BLOB_TRACK` 页面状态。
- 进入 `Blob Track` 后调用 `OpenMV_TrackerControl_Init()`，主循环中持续调用 `OpenMV_TrackerControl_Update()`。
- 从 `Blob Track` 返回主菜单时调用 `OpenMV_TrackerControl_DeInit()`，释放追踪控制状态。
- OLED 进入页面后显示 `Blob Track`，不额外显示调试数据，避免干扰控制逻辑。

## 验证结果

- 已执行 `cmake --build build\Debug`。
- 编译、链接通过。

## 备注

- 本次只接入主菜单入口，没有改动 `openmv_tracker_control.c` 中的控制算法。
