# AI / YOLO 自测 Demo

基于 TensorFlow 的人脸识别神经网络训练与推理 demo。

## 文件说明

| 文件 | 功能 |
|------|------|
| `get_my_faces.py` | 采集自己的人脸数据 |
| `set_other_faces.py` | 设置其他人脸数据集 |
| `train_faces.py` | 训练人脸识别模型 |
| `is_my_face.py` | 推理 - 判断是否为本人 |

## 使用流程

1. 运行 `get_my_faces.py` 采集自己的人脸样本
2. 运行 `set_other_faces.py` 准备负样本
3. 运行 `train_faces.py` 训练模型
4. 运行 `is_my_face.py` 实时人脸识别

## 参考

原文: https://www.cnblogs.com/mu---mu/p/FaceRecognition-tensorflow.html
