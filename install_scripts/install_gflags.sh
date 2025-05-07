#!/bin/bash

# 脚本功能：编译并可选地安装 gflags
#
# 参数:
#   1. gflags_src_dir: gflags 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <gflags_src_dir> <global_install_flag>"
    exit 1
fi

GFLAGS_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

if [ ! -d "$GFLAGS_SRC_DIR" ]; then
    echo "错误: gflags 源代码目录 '$GFLAGS_SRC_DIR' 不存在或不是一个目录。"
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

# --- 编译和安装 gflags ---
echo "----------------------------------------------------"
echo "开始处理 gflags"
echo "gflags 源代码目录: $GFLAGS_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$GFLAGS_SRC_DIR" || { echo "错误: 无法进入 gflags 源代码目录 '$GFLAGS_SRC_DIR'"; exit 1; }

build_subdir="build_gflags_script" # 使用特定名称避免与项目自身build冲突
if [ -d "$build_subdir" ]; then
    echo "目录 '$build_subdir' 已存在于 gflags 中。"
    printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
    read -r response
    response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
    if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
        echo "正在删除 '$build_subdir'..."
        rm -rf "$build_subdir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除 gflags 的 '$build_subdir' 目录。"; exit 1; fi
    else
        echo "继续使用 gflags 现有的 '$build_subdir' 文件夹。"
    fi
fi

mkdir -p "$build_subdir"
cd "$build_subdir" || { echo "错误: 无法进入 gflags 的 '$build_subdir' 目录"; exit 1; }

# Ceres 官方文档建议 gflags 使用共享库并指定命名空间
GFLAGS_CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release -DBUILD_SHARED_LIBS=ON -DGFLAGS_NAMESPACE=google"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    GFLAGS_CMAKE_ARGS="$GFLAGS_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

echo "配置 gflags (在 $(pwd))..."
echo "gflags CMake 参数: $GFLAGS_CMAKE_ARGS"
# shellcheck disable=SC2086
cmake .. $GFLAGS_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: gflags 的 CMake 配置失败。"
    cd "$original_dir"
    exit 1
fi

echo "编译 gflags (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: gflags 的 Make 编译失败。"
    cd "$original_dir"
    exit 1
fi
echo "gflags 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 gflags 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: gflags 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "gflags 安装完成。"
else
    echo "跳过 gflags 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "gflags 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
