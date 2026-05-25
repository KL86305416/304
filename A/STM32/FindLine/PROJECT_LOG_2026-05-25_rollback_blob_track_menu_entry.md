# 2026-05-25 回退色块追踪主菜单入口

## 修改内容

- 从 `Core/Src/main.c` 移除 `Blob Track` 主菜单选项。
- 移除主循环中对 `OpenMV_TrackerControl_Update()` 的菜单入口调用。
- 移除进入和退出 `Blob Track` 页面时的初始化、反初始化逻辑。
- 移除 OLED 页面中实时显示 `ERR-X` 的菜单显示代码。
- 将 `APP_MENU_ITEM_COUNT` 恢复为 `2U`。
- 将 `OpenMV_TrackerControl_Status_t` 恢复为只保留初始化状态、OpenMV 串口状态和电机状态。

## 保留内容

- 保留 `openmv_tracker_control.c/.h` 控制文件。
- 保留 OpenMV UART 接收驱动文件。
- 保留 PD42S1 电机驱动文件。

## 验证结果

- 已执行 `cmake --build build\Debug`。
- 编译、链接通过。
