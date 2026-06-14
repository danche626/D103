# 激光云台自动瞄准系统

电赛激光跟踪瞄准题。摄像头识别矩形靶标 → 双轴步进电机追踪 → 对准后激光点亮。

## 核心指标

| 指标 | 要求 |
|------|------|
| 识别距离 | 1～2m |
| 靶标 | A4纸，黑框白底矩形 |
| 跟踪响应 | < 2s |
| 对准精度 | ±8px（约 ±0.3°）|
| 激光触发 | 连续5帧对准后点亮 |

## 硬件清单

| 组件 | 型号 | 说明 |
|------|------|------|
| 主控 | Jetson Nano 4GB | Ubuntu 18.04 + 扩展板 V3.3 |
| 步进电机 | YL42M × 2 | Modbus RTU, X轴1:10 Y轴1:5 |
| 通信 | LES26 USB-RS485 | CH340, 115200baud |
| 摄像头 | USB Camera | 320×240 MJPEG |
| 激光 | 405nm 10mW TTL | 高电平点亮 |
| 电源 | 5V 4A + 12-24V | Jetson与电机独立供电 |

## 快速部署

```bash
# 拷贝到 Jetson
scp -r src/jetson/ yuanji@192.168.0.18:~/gimbal/

# 安装环境
ssh yuanji@192.168.0.18 "sudo bash ~/gimbal/setup.sh"

# 运行（需连接显示器）
ssh yuanji@192.168.0.18 "cd ~/gimbal && DISPLAY=:0 python3 track.py"
```

## 文档导航

- [系统设计](docs/系统设计.md) — 架构、算法原理、PID控制策略
- [硬件布局](docs/硬件布局.md) — 开发板/电池/屏幕物理布局
- [接线指南](docs/接线指南.md) — 完整接线说明
- [原理图说明](hardware/原理图说明.md) — 扩展板 V3.3 GPIO 分析
