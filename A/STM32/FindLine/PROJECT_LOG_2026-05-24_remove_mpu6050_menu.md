# 项目日志：移除主菜单 MPU6050 调试入口

## 目标

- 从 OLED 主菜单中去掉陀螺仪调试相关选项。
- 保留 MPU6050 驱动文件，不删除 `Core/Inc/mpu6050.h` 和 `Core/Src/mpu6050.c`。

## 改动

- 将 `APP_MENU_ITEM_COUNT` 从 4 调整为 3。
- 删除主菜单中的 `MPU6050 Debug` 显示行。
- 删除 OK 键选择第 4 项时进入 `App_EnterMpu6050Debug()` 的分支。
- 移除已经没有主菜单调用点的 `App_EnterMpu6050Debug()` 应用层入口函数。

## 影响

- 主菜单现在只保留：
  - `Line Follow`
  - `Bluetooth Debug`
  - `Servo Debug`
- MPU6050 驱动源码与构建引用保持不变，后续需要恢复调试入口时可复用现有代码。

## 验证

- 已执行 `cmake --build build\Debug`。
- 编译和链接通过，生成 `FindLine.elf`。
