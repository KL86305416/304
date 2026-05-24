# 项目日志：禁用 PD42S1 菜单自动运动

## 背景

- 进入 `PD42S1 Test` 菜单后，电机出现约 3000RPM 的高速旋转。
- 这说明当前运动控制帧的数据字段格式仍未与 PD42S1 实际协议完全匹配。
- 在协议字段确认前，菜单入口不能自动发送运动指令。

## 改动

- 在 `Core/Inc/pd42s1.h` 新增 `PD42S1_MENU_AUTO_TEST_ENABLE`。
- 默认设置 `PD42S1_MENU_AUTO_TEST_ENABLE = 0U`，禁止主菜单自动执行最小运动测试。
- 进入 `PD42S1 Test` 菜单后只显示 `Auto Disabled`，不发送串口运动命令。
- `PD42S1_RunMinimalTest()` 也受同一开关保护，默认不会发送运动命令。
- 将测试默认参数进一步降低：
  - `PD42S1_TEST_SAFE_RPM = 10U`
  - `PD42S1_TEST_SAFE_REVOLUTIONS = 0.02f`

## 注意

- 在没有核对 PD42S1 手册或抓取正确上位机帧之前，不建议重新开启自动运动测试。
- 需要确认 `0xF1`、`0xF2`、`0xF3` 的数据字段顺序和字段宽度后，再恢复菜单测试。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译和链接通过，生成 `FindLine.elf`。
- 为 `PD42S1_RunMinimalTest()` 增加保护后再次构建通过。
