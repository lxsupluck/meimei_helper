#!/bin/bash
set -e

TARGET="lx@192.168.1.20:/home/lx/helper"
PASSWD="lx123456"

cd "$(dirname "$0")/../build_rpi"
cmake .. -DCMAKE_TOOLCHAIN_FILE=../cmake/toolchain_rpi.cmake
make -j$(nproc)
echo ""
echo "=== 交叉编译完成 ==="
file meimei_helper

# 部署到树莓派
echo ""
echo "=== 部署到树莓派 ==="
if command -v sshpass &> /dev/null; then
    sshpass -p "$PASSWD" scp meimei_helper "$TARGET"
    echo "部署完成: $TARGET"
else
    echo "提示: 安装 sshpass 可自动部署"
    echo "  sudo apt install sshpass -y"
    echo "手动部署: scp meimei_helper $TARGET"
fi
