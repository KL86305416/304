# 2026-05-25 巡线模式并行启用色块追踪

## 修改内容

- 在 `Core/Src/main.c` 中将色块追踪模块接入巡线模式。
- 进入巡线功能时调用 `OpenMV_TrackerControl_Init()`。
- 巡线循环中每 `50ms` 调用一次 `OpenMV_TrackerControl_Update()`。
- 退出巡线功能回到主菜单时调用 `OpenMV_TrackerControl_DeInit()`。

## 设计说明

- 本次没有修改 `openmv_tracker_control.c` 中的用户控制逻辑。
- 追踪更新没有放到每一次主循环都执行，而是按 `APP_LINE_TRACKER_UPDATE_MS` 周期执行，减少 OpenMV 串口接收等待对巡线控制节奏的影响。

## 验证结果

- 已执行 `cmake --build build\Debug`。
- 编译、链接通过。
