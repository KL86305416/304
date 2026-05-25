# 2026-05-25 修复 OpenMV 追踪时电机动一下就停

## 问题

- OpenMV 已经可以正常发送并解析数据。
- 进入 `OpenMV Track` 后，电机会动一下随后停止。

## 原因分析

- 原控制逻辑只要当前帧 `detected=0`，就会立即下发停止命令。
- 色块追踪时云台刚开始转动，画面会有短暂抖动，OpenMV 可能出现一两帧未识别目标。
- 原控制逻辑只要 `err_x` 进入 12px 死区，也会立即停止，容易被短暂抖动或瞬间居中打断。
- 相同速度命令之前不会重复下发，如果驱动器或控制流程被一次停止打断，就可能出现只动一下的现象。

## 修改内容

- 新增丢目标保持：
  - 短暂丢失目标时保持上一次速度。
  - 超过 `APP_OPENMV_TRACK_LOST_STOP_MS` 后才停止。
- 新增中心停止延迟：
  - `err_x` 进入死区后，需要持续 `APP_OPENMV_TRACK_CENTER_STOP_MS` 才停止。
- 新增速度刷新：
  - 即使方向和速度没有变化，也会每隔 `APP_OPENMV_TRACK_SPEED_REFRESH_MS` 重新下发一次速度命令。

## 关键参数

- `APP_OPENMV_TRACK_LOST_STOP_MS = 300ms`
- `APP_OPENMV_TRACK_CENTER_STOP_MS = 150ms`
- `APP_OPENMV_TRACK_SPEED_REFRESH_MS = 300ms`

## 验证

- 已执行 `cmake --build build\Debug --clean-first`。
- 编译通过。
- 资源占用：
  - RAM：3448 B / 32 KB
  - FLASH：28772 B / 128 KB
