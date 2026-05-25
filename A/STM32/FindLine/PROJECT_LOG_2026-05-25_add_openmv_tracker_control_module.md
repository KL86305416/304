# 2026-05-25 新增 OpenMV 色块追踪控制模块

## 修改内容

- 新增控制模块：
  - `Core/Inc/openmv_tracker_control.h`
  - `Core/Src/openmv_tracker_control.c`
- 将新源文件加入 `cmake/stm32cubemx/CMakeLists.txt`。
- 控制模块负责：
  - 接收并解析 OpenMV 色块数据
  - 根据 `err_x` 计算电机方向
  - 根据误差大小计算 RPM
  - 使用 PD42S1 速度模式控制电机
  - 目标短暂丢失时保持
  - 目标居中后延迟停止
  - 周期性刷新速度命令

## 主要接口

- `OpenMV_TrackerControl_Init()`
- `OpenMV_TrackerControl_Update()`
- `OpenMV_TrackerControl_Stop()`
- `OpenMV_TrackerControl_DeInit()`
- `OpenMV_TrackerControl_CalcRpm()`
- `OpenMV_TrackerControl_GetStatus()`

## 说明

- 本次没有修改主菜单。
- 本次没有把控制模块接入 `main.c` 主循环。
- 后续只需要在需要追踪的模式中调用 `Init()` 和周期性 `Update()` 即可。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3368 B / 32 KB
  - FLASH：23472 B / 128 KB
