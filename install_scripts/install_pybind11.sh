#!/bin/bash

# 脚本功能：编译并可选地安装 pybind11
# pybind11 主要是一个头文件库，但通过 CMake 安装可以方便项目查找。
#
# 参数:
#   1. pybind11_src_dir: pybind11 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <pybind11_src_dir> <global_install_flag>"
    exit 1
fi

PYBIND11_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

# 将全局安装标志转换为小写
GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

# 检查 pybind11 路径
if [ ! -d "$PYBIND11_SRC_DIR" ]; then
    echo "错误: pybind11 源代码目录 '$PYBIND11_SRC_DIR' 不存在或不是一个目录。"
    exit 1
fi

# --- 获取核心数 (主要用于 make，pybind11 的 make 步骤可能很轻量) ---
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

# --- 编译和安装 pybind11 ---
echo "----------------------------------------------------"
echo "开始处理 pybind11"
echo "pybind11 源代码目录: $PYBIND11_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$PYBIND11_SRC_DIR" || { echo "错误: 无法进入 pybind11 源代码目录 '$PYBIND11_SRC_DIR'"; exit 1; }

build_subdir="build" # 通用构建子目录名
if [ -d "$build_subdir" ]; then
    echo "目录 '$build_subdir' 已存在于 pybind11 中。"
    printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
    read -r response
    response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
    if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
        echo "正在删除 '$build_subdir'..."
        rm -rf "$build_subdir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除 pybind11 的 '$build_subdir' 目录。"; exit 1; fi
    else
        echo "继续使用 pybind11 现有的 '$build_subdir' 文件夹。"
    fi
fi

mkdir -p "$build_subdir"
cd "$build_subdir" || { echo "错误: 无法进入 pybind11 的 '$build_subdir' 目录"; exit 1; }

PYBIND11_CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    PYBIND11_CMAKE_ARGS="$PYBIND11_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

# pybind11 的 CMakeLists.txt 通常用于构建测试和安装自身，使其能被 find_package 找到
# 对于 pybind11 < 2.6, PYBIND11_TEST=OFF 可能是必要的如果不想构建测试
# 对于 pybind11 >= 2.6, use PYBIND11_NOPYTHON=ON (or OFF) and PYBIND11_NO_CMAKE_CONFIG=ON (or OFF)
# 默认情况下，我们会尝试构建和安装能让 find_package(pybind11) 工作的组件
# 如果您的pybind11版本较旧且遇到测试相关的编译问题，可以尝试添加 -DPYBIND11_TEST=OFF
# PYBIND11_CMAKE_ARGS="$PYBIND11_CMAKE_ARGS -DPYBIND11_TEST=OFF" # 例如

echo "配置 pybind11 (在 $(pwd))..."
echo "pybind11 CMake 参数: $PYBIND11_CMAKE_ARGS"
# shellcheck disable=SC2086
cmake .. $PYBIND11_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: pybind11 的 CMake 配置失败。"
    cd "$original_dir"
    exit 1
fi

# pybind11 的 make 步骤可能非常快，因为它主要是头文件，但也可能编译一些测试或辅助工具
echo "编译 pybind11 (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "警告: pybind11 的 Make 编译步骤出现问题 (可能只是测试部分，如果已启用)。"
    # 不直接退出，因为核心的头文件可能仍然可用，安装步骤更重要
fi
echo "pybind11 编译完成 (或尝试编译完成)。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 pybind11 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: pybind11 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "pybind11 安装完成。"
else
    echo "跳过 pybind11 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
    echo "pybind11 主要作为头文件库使用。如果不安装，您需要确保您的项目能通过 -I<pybind11_src_dir>/include 找到它，"
    echo "或者通过 add_subdirectory 将其直接包含到您的 CMake项目中。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "pybind11 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
