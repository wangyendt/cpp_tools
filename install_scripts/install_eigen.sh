#!/bin/bash

# 脚本功能：安装 Eigen 库
# 参数1: Eigen 源代码的根目录路径
# 参数2: 安装标志 (true/false) - 如果为 true，则执行 sudo make install

# 检查参数数量
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <eigen_source_directory> <install_flag (true/false)>"
    exit 1
fi

EIGEN_PATH="$1"
INSTALL_FLAG_INPUT="$2"

# 将安装标志转换为小写
INSTALL_FLAG=$(echo "$INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

# 检查 Eigen 路径是否存在且为目录
if [ ! -d "$EIGEN_PATH" ]; then
    echo "错误: Eigen 源代码目录 '$EIGEN_PATH' 不存在或不是一个目录。"
    exit 1
fi

# 进入 Eigen 源代码目录
cd "$EIGEN_PATH" || { echo "错误: 无法进入目录 '$EIGEN_PATH'"; exit 1; }

echo "当前工作目录: $(pwd)"

# 创建 build 文件夹，如果不存在的话
BUILD_DIR="build"
if [ ! -d "$BUILD_DIR" ]; then
    echo "创建目录: $BUILD_DIR"
    mkdir "$BUILD_DIR"
    if [ $? -ne 0 ]; then
        echo "错误: 无法创建目录 '$BUILD_DIR'。"
        exit 1
    fi
else
    echo "目录 '$BUILD_DIR' 已存在。建议在干净的 build 目录下进行安装。"
    # Eigen 通常是头文件库，build 目录冲突问题不大，但为了规范，可以提示
    read -r -p "是否要删除并重建 '$BUILD_DIR' 文件夹? (y/N): " response
    response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
    if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
        echo "正在删除 '$BUILD_DIR'..."
        rm -rf "$BUILD_DIR"
        if [ $? -ne 0 ]; then echo "错误: 无法删除目录 '$BUILD_DIR'。"; exit 1; fi
        echo "创建目录: $BUILD_DIR"
        mkdir "$BUILD_DIR"
        if [ $? -ne 0 ]; then echo "错误: 无法创建目录 '$BUILD_DIR'。"; exit 1; fi
    else
        echo "继续使用现有的 '$BUILD_DIR' 文件夹。"
    fi
fi

# 进入 build 文件夹
cd "$BUILD_DIR" || { echo "错误: 无法进入目录 '$BUILD_DIR'"; exit 1; }

echo "切换到编译目录: $(pwd)"

# 判断操作系统并获取核心数
OS_TYPE=$(uname -s)
NUM_CORES=1 # 默认核心数

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

INSTALL_PREFIX="/usr/local"
echo "Eigen 将被安装到: $INSTALL_PREFIX (如果启用安装)"

# 执行 CMake
echo "正在执行 CMake..."
cmake .. -DCMAKE_INSTALL_PREFIX="$INSTALL_PREFIX"
if [ $? -ne 0 ]; then
    echo "错误: CMake 执行失败。"
    exit 1
fi
echo "CMake 完成。"

# 执行 Make (对于 Eigen，这一步可能主要是构建测试或文档，如果它们被启用的话)
echo "正在执行 Make (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: Make 执行失败。"
    # 对于纯头文件库，make失败可能不影响安装，但仍需注意
fi
echo "Make 完成。"

# 如果安装标志为 true，则执行 sudo make install
if [ "$INSTALL_FLAG" = "true" ]; then
    echo "正在执行 sudo make install..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: sudo make install 执行失败。"
        exit 1 # 对于安装步骤，失败则退出
    fi
    echo "sudo make install 完成。Eigen 已安装到 $INSTALL_PREFIX"
else
    echo "跳过安装步骤 (install_flag 为 '$INSTALL_FLAG_INPUT')。"
    echo "如果需要安装，请在第二个参数传入 true。"
fi

echo "Eigen 安装脚本执行完毕。"
cd ../.. # 返回到脚本执行前的原始目录的父目录
exit 0
