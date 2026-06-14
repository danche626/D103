# Embedded / 嵌入式开发

嵌入式平台开发项目，涵盖 STM32 和 ESP32 生态。

## 目录结构

```
├── micu/                  米醋主板 V2.0 (MainBoard_V2.0_MengXi)
├── 中科大电控训练/         中科大 RoboWalker 电控训练资料
│   ├── 01_electrical/     电路基础 + STM32 + 电机控制 + 底盘
│   ├── readme.md          训练大纲
│   └── robowalker_train_slides/
└── lowpower-camera/       低功耗相机项目（ESP32-IDF）
    ├── main/              应用代码
    ├── components/        组件库
    └── CMakeLists.txt     构建配置
```

## 技术栈

- **micu**: STM32 主控，含原理图与驱动指南
- **中科大电控**: RoboMaster 电控入门训练（GPIO/UART/ADC/电机/PID/底盘）
- **lowpower-camera**: ESP-IDF 框架，低功耗拍照 + WiFi 传图

## 注意

CMSIS 库文件 (.lib) 已移除以减小仓库体积，使用 STM32CubeMX 重新生成即可。
