#!/bin/bash

# 脚本功能：编译并可选地安装 Sophus
# Sophus 主要是一个头文件库，但通过 CMake 安装可以方便项目查找。
#
# 参数:
#   1. sophus_src_dir: Sophus 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)
#
# 注意: 此脚本假设 Eigen3 已经安装在系统路径中。

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <sophus_src_dir> <global_install_flag>"
    exit 1
fi

SOPHUS_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

if [ ! -d "$SOPHUS_SRC_DIR" ]; then
    echo "错误: Sophus 源代码目录 '$SOPHUS_SRC_DIR' 不存在或不是一个目录。"
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

# --- 编译和安装 Sophus ---
echo "----------------------------------------------------"
echo "开始处理 Sophus"
echo "Sophus 源代码目录: $SOPHUS_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "确保 Eigen3 已经安装。"
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$SOPHUS_SRC_DIR" || { echo "错误: 无法进入 Sophus 源代码目录 '$SOPHUS_SRC_DIR'"; exit 1; }

build_subdir="build"
if [ -d "$build_subdir" ]; then
    non_interactive_mode_enabled=$(echo "$NON_INTERACTIVE_INSTALL" | tr '[:upper:]' '[:lower:]')
    if [ "$non_interactive_mode_enabled" = "true" ]; then
        echo "NON_INTERACTIVE_INSTALL=true. 目录 '$build_subdir' 已存在于 Sophus 中。自动删除并重建..."
        echo "正在删除 '$build_subdir'..."
        rm -rf "$build_subdir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除 Sophus 的 '$build_subdir' 目录。"; exit 1; fi
    else
        echo "目录 '$build_subdir' 已存在于 Sophus 中。"
        printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
        read -r response
        response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
        if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
            echo "正在删除 '$build_subdir'..."
            rm -rf "$build_subdir"
            if [ $? -ne 0 ]; then echo "错误: 无法删除 Sophus 的 '$build_subdir' 目录。"; exit 1; fi
        else
            echo "继续使用 Sophus 现有的 '$build_subdir' 文件夹。"
        fi
    fi
fi

mkdir -p "$build_subdir"
cd "$build_subdir" || { echo "错误: 无法进入 Sophus 的 '$build_subdir' 目录"; exit 1; }

SOPHUS_CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DBUILD_SOPHUS_TESTS=OFF -DBUILD_SOPHUS_EXAMPLES=OFF"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    SOPHUS_CMAKE_ARGS="$SOPHUS_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

echo "配置 Sophus (在 $(pwd))..."
echo "Sophus CMake 参数: $SOPHUS_CMAKE_ARGS"
# shellcheck disable=SC2086 # Allow word splitting for SOPHUS_CMAKE_ARGS
cmake .. $SOPHUS_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: Sophus 的 CMake 配置失败。请确保 Eigen3 已正确安装并能被 CMake 找到。"
    cd "$original_dir"
    exit 1
fi

echo "编译 Sophus (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: Sophus 的 Make 编译失败。" # 对于头文件库，这通常不应该发生，除非测试/示例开启且有问题
    cd "$original_dir"
    exit 1
fi
echo "Sophus 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 Sophus 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: Sophus 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "Sophus 安装完成。"
else
    echo "跳过 Sophus 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
    echo "Sophus 主要作为头文件库使用。如果不安装，您需要确保您的项目能通过 -I<sophus_src_dir>/sophus (或类似路径) 找到它，"
    echo "或者通过 add_subdirectory 将其直接包含到您的 CMake 项目中 (推荐)。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "Sophus 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
