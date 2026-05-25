# 2026-05-25 创建 OpenMV 色块控制初始化框架

## 修改内容

- 新增控制模块头文件：
  - `Core/Inc/openmv_tracker_control.h`
- 新增控制模块源文件：
  - `Core/Src/openmv_tracker_control.c`
- 模块目前只完成初始化和反初始化，不包含控制算法。

## 当前接口

- `OpenMV_TrackerControl_Init()`
  - 初始化 OpenMV 串口
  - 清空 OpenMV 串口接收缓存
  - 初始化 PD42S1 电机驱动
  - 使能 PD42S1
  - 清除 PD42S1 状态
- `OpenMV_TrackerControl_Update()`
  - 当前为空函数，留给后续编写最终控制逻辑
- `OpenMV_TrackerControl_DeInit()`
  - 失能 PD42S1
- `OpenMV_TrackerControl_GetStatus()`
  - 获取初始化状态和最近一次初始化结果

## 说明

- 本次没有写任何色块追踪控制思路。
- 本次没有把该模块接入主菜单或主循环。
- `openmv_tracker_control.c` 已经在 CMake 中参与编译。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3368 B / 32 KB
  - FLASH：25828 B / 128 KB
