# 激光云台自动瞄准系统

## 项目简介

电子设计竞赛（电赛）激光跟踪瞄准题目。系统通过摄像头识别矩形靶标，双轴步进电机云台自动追踪，对准后激光自动点亮。

## 目录结构

```
激光云台/
├── README.md                 ← 本文件
├── docs/
│   ├── 系统设计.md           ← 整体架构、算法原理、控制策略
│   ├── 硬件布局.md           ← 开发板/电池/屏幕物理布局方案
│   └── 接线指南.md           ← 完整接线说明
├── src/
│   ├── jetson/
│   │   ├── track.py          ← Jetson 部署主程序
│   │   ├── setup.sh          ← 环境配置脚本
│   │   └── gimbal.service    ← systemd 服务文件
│   └── tools/
│       ├── dir_test.py       ← 电机方向测试
│       └── limit_test.py     ← 限位角度测试
└── hardware/
    └── 原理图说明.md         ← 扩展板原理图关键信息
```

## 核心指标

| 指标 | 要求 |
|------|------|
| 识别距离 | 1～2m |
| 靶标 | A4纸，黑框白底 |
| 跟踪响应 | < 2s |
| 对准精度 | ±8px（约 ±0.3°）|
| 激光触发 | 连续5帧对准后点亮 |

## 硬件清单

| 组件 | 型号 | 说明 |
|------|------|------|
| 主控 | Jetson Nano 4GB | Ubuntu 18.04 + 扩展板 V3.3 |
| 步进电机 | YL42M × 2 | Modbus RTU，X轴1:10 Y轴1:5 |
| 通信 | LES26 USB-RS485 | CH340，115200baud |
| 摄像头 | USB Camera | 320×240 MJPEG |
| 激光 | 405nm 10mW TTL | 高电平点亮 |
| 电池 | 5V 4A (Jetson) + 12-24V (电机) | 双电源独立供电 |

## 快速部署

```bash
# 1. 拷贝到 Jetson
scp -r src/jetson/ yuanji@192.168.0.18:~/gimbal/

# 2. 安装环境
ssh yuanji@192.168.0.18 "sudo bash ~/gimbal/setup.sh"

# 3. 运行
ssh yuanji@192.168.0.18 "cd ~/gimbal && DISPLAY=:0 python3 track.py"
```
