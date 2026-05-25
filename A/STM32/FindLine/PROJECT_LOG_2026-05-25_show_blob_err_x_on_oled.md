# 2026-05-25 OLED 实时显示 OpenMV ERR-X

## 修改内容

- 在 `OpenMV_TrackerControl_Status_t` 中新增 `data_valid`、`detected`、`err_x` 字段。
- `OpenMV_TrackerControl_Update()` 成功解析 OpenMV 数据后，保存最新 `err_x`。
- `Blob Track` 页面每 100ms 刷新 OLED，显示最新 `ERR-X` 数值。
- 页面同时显示 `DET` 和串口接收状态，方便判断 OpenMV 是否正在正常发送数据。

## 验证结果

- 已执行 `cmake --build build\Debug`。
- 编译、链接通过。
