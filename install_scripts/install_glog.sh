#!/bin/bash

# 脚本功能：编译并可选地安装 glog
#
# 参数:
#   1. glog_src_dir: glog 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)
#
# 注意: 此脚本假设 gflags 已经安装在系统路径中 (或通过本系列脚本安装)。

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <glog_src_dir> <global_install_flag>"
    exit 1
fi

GLOG_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

if [ ! -d "$GLOG_SRC_DIR" ]; then
    echo "错误: glog 源代码目录 '$GLOG_SRC_DIR' 不存在或不是一个目录。"
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
INSTALL_PREFIX="/usr/local"

# --- 编译和安装 glog ---
echo "----------------------------------------------------"
echo "开始处理 glog"
echo "glog 源代码目录: $GLOG_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "确保 gflags 已经安装。"
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$GLOG_SRC_DIR" || { echo "错误: 无法进入 glog 源代码目录 '$GLOG_SRC_DIR'"; exit 1; }

build_subdir="build_glog_script" # 使用特定名称
if [ -d "$build_subdir" ]; then
    non_interactive_mode_enabled=$(echo "$NON_INTERACTIVE_INSTALL" | tr '[:upper:]' '[:lower:]')
    if [ "$non_interactive_mode_enabled" = "true" ]; then
        echo "NON_INTERACTIVE_INSTALL=true. 目录 '$build_subdir' 已存在于 glog 中。自动删除并重建..."
        echo "正在删除 '$build_subdir'..."
        rm -rf "$build_subdir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除 glog 的 '$build_subdir' 目录。"; exit 1; fi
    else
        echo "目录 '$build_subdir' 已存在于 glog 中。"
        printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
        read -r response
        response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
        if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
            echo "正在删除 '$build_subdir'..."
            rm -rf "$build_subdir"
            if [ $? -ne 0 ]; then echo "错误: 无法删除 glog 的 '$build_subdir' 目录。"; exit 1; fi
        else
            echo "继续使用 glog 现有的 '$build_subdir' 文件夹。"
        fi
    fi
fi

mkdir -p "$build_subdir"
cd "$build_subdir" || { echo "错误: 无法进入 glog 的 '$build_subdir' 目录"; exit 1; }

# glog 可能需要 C++14 (根据之前的错误日志)
GLOG_CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_STANDARD=14"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    GLOG_CMAKE_ARGS="$GLOG_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

echo "配置 glog (在 $(pwd))..."
echo "glog CMake 参数: $GLOG_CMAKE_ARGS"
# shellcheck disable=SC2086
cmake .. $GLOG_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: glog 的 CMake 配置失败。请确保 gflags 已正确安装，并且C++编译器支持C++14。"
    cd "$original_dir"
    exit 1
fi

echo "编译 glog (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: glog 的 Make 编译失败。"
    cd "$original_dir"
    exit 1
fi
echo "glog 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 glog 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: glog 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "glog 安装完成。"
else
    echo "跳过 glog 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "glog 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
