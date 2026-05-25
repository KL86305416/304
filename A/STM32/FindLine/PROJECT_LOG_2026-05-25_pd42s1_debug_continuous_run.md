# 2026-05-25 PD42S1 调试菜单增加连续转动测试

## 修改内容

- 在 `Core/Src/pd42s1_debug.c` 中加入一个简单的连续转动测试。
- 进入 `Motor Debug` 后：
  - 初始化 PD42S1 串口驱动
  - 使能电机
  - 清除状态
  - 下发速度模式命令，让电机持续顺时针转动
- 停留在 `Motor Debug` 界面时，每 500ms 重新下发一次速度命令。
- 退出 `Motor Debug` 时调用 `PD42S1_DisableMotor()`。

## 参数

- 方向：`PD42S1_DIRECTION_CW`
- 加减速度：`50`
- 转速：`60RPM`
- 刷新周期：`500ms`

## 注意

- 本次没有使用 `PD42S1_StopMotor()`，避免触发驱动器“刹车”状态。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3376 B / 32 KB
  - FLASH：26028 B / 128 KB
