# 2026-05-25 新增空的 PD42S1 电机调试菜单

## 修改内容

- 新增空调试文件：
  - `Core/Inc/pd42s1_debug.h`
  - `Core/Src/pd42s1_debug.c`
- `pd42s1_debug.c` 当前只包含空函数：
  - `PD42S1_Debug_Init()`
  - `PD42S1_Debug_Update()`
  - `PD42S1_Debug_DeInit()`
- 主菜单新增 `Motor Debug` 选项。
- 进入 `Motor Debug` 后会调用 `PD42S1_Debug_Init()`。
- 停留在 `Motor Debug` 界面时会循环调用 `PD42S1_Debug_Update()`。
- 返回主菜单时会调用 `PD42S1_Debug_DeInit()`。
- 已将 `pd42s1_debug.c` 加入 CMake 编译列表。

## 说明

- 本次没有在 debug 文件中写任何电机控制动作。
- 后续可以直接在 `pd42s1_debug.c` 中添加自己的测试逻辑。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译通过。
- 资源占用：
  - RAM：3368 B / 32 KB
  - FLASH：23804 B / 128 KB
