# 项目日志：删除舵机调试功能

## 目标

- 完全删除舵机调试功能。
- 删除舵机驱动文件。
- 删除 OLED 界面中有关按键功能的提示文字。

## 改动

- 从 `Core/Src/main.c` 删除 `Servo Debug` 主菜单项。
- 删除舵机调试页面的进入、更新、显示和按键控制逻辑。
- 删除 `Servo_Init()` 初始化调用和 `servo.h` 引用。
- 将主菜单项数量调整为 2，仅保留 `Line Follow` 和 `Bluetooth Debug`。
- 删除主菜单、蓝牙调试页和 MPU6050 调试页中的按键功能提示文字。
- 从 `Core/Src/gpio.c` 删除 PA6/PA7 舵机 PWM 复用输出配置。
- 从 `cmake/stm32cubemx/CMakeLists.txt` 删除 `servo.c` 构建引用。
- 删除 `Core/Src/servo.c` 和 `Core/Inc/servo.h`。

## 验证

- 已执行 `cmake --build build\Debug`。
- CMake 重新生成构建文件成功。
- 编译和链接通过，生成 `FindLine.elf`。
