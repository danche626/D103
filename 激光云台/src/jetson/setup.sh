#!/bin/bash
# Jetson Nano 环境配置一键脚本
# 用法: sudo bash setup.sh

set -e

echo "=== Jetson Nano 激光云台环境配置 ==="

# 1. 换源（清华源，加速下载）
echo "[1/6] 配置apt源..."
cp /etc/apt/sources.list /etc/apt/sources.list.bak
cat > /etc/apt/sources.list << 'EOF'
deb http://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports/ bionic main restricted universe multiverse
deb http://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports/ bionic-updates main restricted universe multiverse
deb http://mirrors.tuna.tsinghua.edu.cn/ubuntu-ports/ bionic-security main restricted universe multiverse
EOF
apt update

# 2. 安装系统依赖
echo "[2/6] 安装系统依赖..."
apt install -y python3-pip python3-opencv python3-numpy libglib2.0-0 v4l-utils

# 3. pip换源 + 安装Python包
echo "[3/6] 安装Python包..."
pip3 install -i https://pypi.tuna.tsinghua.edu.cn/simple pyserial Jetson.GPIO

# 4. 串口权限
echo "[4/6] 配置串口权限..."
usermod -aG dialout $SUDO_USER

# 5. GPIO权限
echo "[5/6] 配置GPIO权限..."
usermod -aG gpio $SUDO_USER

# 6. 摄像头权限
echo "[6/6] 配置摄像头权限..."
usermod -aG video $SUDO_USER

echo ""
echo "=== 配置完成 ==="
echo "请重新登录或重启使权限生效: sudo reboot"
echo "然后运行: cd ~/gimbal && python3 track.py"
