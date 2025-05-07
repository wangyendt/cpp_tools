#!/bin/bash

# 脚本功能：从源码编译并可选地安装 CMake
#
# 参数:
#   1. cmake_src_dir: CMake 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <cmake_src_dir> <global_install_flag>"
    exit 1
fi

CMAKE_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

if [ ! -d "$CMAKE_SRC_DIR" ]; then
    echo "错误: CMake 源代码目录 '$CMAKE_SRC_DIR' 不存在或不是一个目录。"
    exit 1
fi

# --- 获取核心数 ---
OS_TYPE=$(uname -s)
NUM_CORES=1
case "$OS_TYPE" in
    Linux*) NUM_CORES=$(nproc) ;;
    Darwin*) NUM_CORES=$(sysctl -n hw.ncpu) ;;
    *) echo "警告: 未知的操作系统 '$OS_TYPE'。使用 1核心。" ;;
esac
echo "使用 $NUM_CORES 核心进行编译。"

# --- 安装前缀 ---
INSTALL_PREFIX="/usr/local" # CMake 通常安装到这里

# --- 编译和安装 CMake ---
echo "----------------------------------------------------"
echo "开始处理 CMake"
echo "CMake 源代码目录: $CMAKE_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$CMAKE_SRC_DIR" || { echo "错误: 无法进入 CMake 源代码目录 '$CMAKE_SRC_DIR'"; exit 1; }

echo "配置 CMake (运行 bootstrap)..."
bootstrap_args=""
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    bootstrap_args="--prefix=$INSTALL_PREFIX"
fi

./bootstrap $bootstrap_args --parallel="$NUM_CORES" --system-curl

if [ $? -ne 0 ]; then
    echo "错误: CMake 的 bootstrap 过程失败。"
    cd "$original_dir"
    exit 1
fi
echo "CMake bootstrap 完成。"

echo "编译 CMake (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: CMake 的 Make 编译失败。"
    cd "$original_dir"
    exit 1
fi
echo "CMake 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 CMake 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: CMake 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "CMake 安装完成。"
    echo "请确保 $INSTALL_PREFIX/bin 在您的 PATH 环境变量中，或者重新登录。"
else
    echo "跳过 CMake 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
    echo "CMake 已在 $CMAKE_SRC_DIR 编译完成。"
    echo "您可以从 $CMAKE_SRC_DIR/bin/cmake 运行它，或者手动将其安装到您的 PATH。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "CMake 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
