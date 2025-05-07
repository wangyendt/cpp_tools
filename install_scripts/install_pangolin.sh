#!/bin/bash

# 脚本功能：编译并可选地安装 Pangolin
#
# 参数:
#   1. pangolin_src_dir: Pangolin 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)
#
# 注意: 此脚本不主动安装Pangolin的所有可选依赖，依赖于系统已有的库。
# 请确保在运行此脚本前，已安装Pangolin所需的OpenGL, Eigen3以及其他可选依赖 (如X11, FFmpeg等)。

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <pangolin_src_dir> <global_install_flag>"
    exit 1
fi

PANGOLIN_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

# 将全局安装标志转换为小写
GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

# 检查 Pangolin 路径
if [ ! -d "$PANGOLIN_SRC_DIR" ]; then
    echo "错误: Pangolin 源代码目录 '$PANGOLIN_SRC_DIR' 不存在或不是一个目录。"
    exit 1
fi

# --- 获取核心数 ---
OS_TYPE=$(uname -s)
NUM_CORES=1
case "$OS_TYPE" in
    Linux*)
        NUM_CORES=$(nproc)
        echo "检测到 Linux 系统，使用 $NUM_CORES 核心进行编译。"
        ;;
    Darwin*)
        NUM_CORES=$(sysctl -n hw.ncpu)
        echo "检测到 macOS 系统，使用 $NUM_CORES 核心进行编译。"
        ;;
    *)
        echo "警告: 未知的操作系统 '$OS_TYPE'。将使用默认的 1 个核心进行编译。"
        ;;
esac

# --- 安装前缀 ---
INSTALL_PREFIX="/usr/local"

# --- 编译和安装 Pangolin ---
echo "----------------------------------------------------"
echo "开始处理 Pangolin"
echo "Pangolin 源代码目录: $PANGOLIN_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "重要提示: 请确保已安装 Pangolin 的依赖项 (OpenGL, Eigen3, X11/Cocoa, FFmpeg等)。"
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$PANGOLIN_SRC_DIR" || { echo "错误: 无法进入 Pangolin 源代码目录 '$PANGOLIN_SRC_DIR'"; exit 1; }

build_subdir="build"
if [ -d "$build_subdir" ]; then
    echo "目录 '$build_subdir' 已存在于 Pangolin 中。"
    printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
    read -r response
    response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
    if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
        echo "正在删除 '$build_subdir'..."
        rm -rf "$build_subdir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除 Pangolin 的 '$build_subdir' 目录。"; exit 1; fi
    else
        echo "继续使用 Pangolin 现有的 '$build_subdir' 文件夹。"
    fi
fi

mkdir -p "$build_subdir"
cd "$build_subdir" || { echo "错误: 无法进入 Pangolin 的 '$build_subdir' 目录"; exit 1; }

PANGOLIN_CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

# Pangolin 的一些常用 CMake 选项 (可以根据需要调整)
# Pangolin的CMake脚本会自动检测很多依赖。这里可以覆盖或强制某些选项。
# PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DBUILD_EXAMPLES=ON" # 默认通常是ON
# PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DBUILD_TOOLS=ON"    # 默认通常是ON
# PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DBUILD_PANGOLIN_PYTHON=ON" # 尝试构建Python绑定
# PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DOpenGL_GL_PREFERENCE=GLVND" # Linux上可以尝试GLVND

if [ "$OS_TYPE" == "Darwin" ]; then
    # 在 macOS 上，Pangolin 通常能很好地找到 Cocoa 和 OpenGL
    # 如果遇到 Eigen3 找不到的问题，且 Eigen3 是通过 brew 安装在非标准路径，
    # 可能需要类似 -DEIGEN3_INCLUDE_DIR=/opt/homebrew/include/eigen3 的参数
    echo "macOS 系统，Pangolin 将尝试使用系统框架。"
fi

if [ "$OS_TYPE" == "Linux" ]; then
    echo "Linux 系统，确保已安装 X11, OpenGL, GLEW, Eigen3 等依赖。"
    # 如果需要特定版本的Python绑定:
    # PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DPYTHON_EXECUTABLE=/usr/bin/python3.x"
    # PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DPYTHON_INCLUDE_DIR=/usr/include/python3.x"
    # PANGOLIN_CMAKE_ARGS="$PANGOLIN_CMAKE_ARGS -DPYTHON_LIBRARY=/usr/lib/x86_64-linux-gnu/libpython3.x.so"
fi

echo "配置 Pangolin (在 $(pwd))..."
echo "Pangolin CMake 参数: $PANGOLIN_CMAKE_ARGS"
# shellcheck disable=SC2086
cmake .. $PANGOLIN_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: Pangolin 的 CMake 配置失败。请检查依赖项是否都已安装。"
    cd "$original_dir"
    exit 1
fi

echo "编译 Pangolin (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: Pangolin 的 Make 编译失败。"
    cd "$original_dir"
    exit 1
fi
echo "Pangolin 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 Pangolin 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: Pangolin 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "Pangolin 安装完成。"
else
    echo "跳过 Pangolin 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "Pangolin 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
