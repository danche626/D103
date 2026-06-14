# D103

电子融创社团团队技术仓库，按领域分支管理学习资料与竞赛项目。

## 分支索引

| 分支 | 类别 | 内容 |
|------|------|------|
| `hardware/pcb` | 硬件-PCB | PCB Layout 证书练习、Gerber工程、高性能电路板复刻(H616/RK3588/STM32H750/FPGA) |
| `hardware/mechanical` | 硬件-结构 | 3D打印件（丐版机器人、装甲板灯罩、二维云台） |
| `embedded/stm32` | 嵌入式 | 米醋主板V2.0、中科大电控训练、ESP32低功耗相机 |
| `competition/electronic-design` | 竞赛-电赛 | 历年赛题、激光云台项目V2.0、体感云台、三相AC-DC |
| `competition/math-modeling` | 竞赛-数建 | 国赛C题、统建（LSTM/CNN销量预测）、Matlab |
| `competition/innovation` | 竞赛-双创 | 大创 TransitX 交通规划系统、互联网+ |
| `competition/openharmony` | 竞赛-嵌赛 | 开放原子大赛、OpenHarmony + RK2206 智慧园区 |
| `ai/yolo-demo` | AI开发 | TensorFlow 人脸识别训练/推理 demo |
| `robomaster` | RoboMaster | 蓝三步兵车完整开源（机械+电控+BOM） |

## 使用方式

```bash
# 切换到对应分支查看内容
git checkout hardware/pcb
git checkout competition/electronic-design
# ...
```

## 分支命名规范

- 按 `领域/子方向` 命名（如 `hardware/pcb`、`competition/math-modeling`）
- 项目分支下可建个人分支（如 `激光云台项目` → `申培元项目框架原理`）
- 每次提交写清楚改了什么，保持结构清晰
