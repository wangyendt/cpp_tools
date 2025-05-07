#!/bin/bash

# 脚本功能：编译并可选地安装 OpenCV 及其 contrib 模块
#
# 参数:
#   1. opencv_src_dir: OpenCV 源代码的根目录 (必需)
#   2. opencv_contrib_src_dir: opencv_contrib 源代码的根目录 (可选, "skip" 或 "" 跳过)
#   3. global_install_flag: 是否执行 sudo make install (true/false) (必需)

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 3 ]; then
    echo "错误: 需要三个参数。"
    echo "用法: $0 <opencv_src_dir> <opencv_contrib_src_dir|"skip"> <global_install_flag>"
    exit 1
fi

OPENCV_SRC_DIR="$1"
OPENCV_CONTRIB_SRC_DIR="$2"
GLOBAL_INSTALL_FLAG_INPUT="$3"

# 将全局安装标志转换为小写
GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

# 检查 OpenCV 路径
if [ ! -d "$OPENCV_SRC_DIR" ]; then
    echo "错误: OpenCV 源代码目录 '$OPENCV_SRC_DIR' 不存在或不是一个目录。"
    exit 1
fi

# 检查 opencv_contrib 路径 (如果不是 skip)
if [ -n "$OPENCV_CONTRIB_SRC_DIR" ] && [ "$OPENCV_CONTRIB_SRC_DIR" != "skip" ] && [ ! -d "$OPENCV_CONTRIB_SRC_DIR" ]; then
    echo "错误: opencv_contrib 源代码目录 '$OPENCV_CONTRIB_SRC_DIR' 不存在或不是一个目录。"
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

# --- 编译和安装 OpenCV ---
echo "----------------------------------------------------"
echo "开始处理 OpenCV"
echo "OpenCV 源代码目录: $OPENCV_SRC_DIR"
if [ -n "$OPENCV_CONTRIB_SRC_DIR" ] && [ "$OPENCV_CONTRIB_SRC_DIR" != "skip" ]; then
    echo "OpenCV Contrib 源代码目录: $OPENCV_CONTRIB_SRC_DIR"
fi
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$OPENCV_SRC_DIR" || { echo "错误: 无法进入 OpenCV 源代码目录 '$OPENCV_SRC_DIR'"; exit 1; }

build_subdir="build" # OpenCV 通常使用 'build'
if [ -d "$build_subdir" ]; then
    non_interactive_mode_enabled=$(echo "$NON_INTERACTIVE_INSTALL" | tr '[:upper:]' '[:lower:]')
    if [ "$non_interactive_mode_enabled" = "true" ]; then
        echo "NON_INTERACTIVE_INSTALL=true. 目录 '$build_subdir' 已存在于 OpenCV 中。自动删除并重建..."
        echo "正在删除 '$build_subdir'..."
        rm -rf "$build_subdir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除 OpenCV 的 '$build_subdir' 目录。"; exit 1; fi
    else
        echo "目录 '$build_subdir' 已存在于 OpenCV 中。"
        printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
        read -r response
        response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
        if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
            echo "正在删除 '$build_subdir'..."
            rm -rf "$build_subdir"
            if [ $? -ne 0 ]; then echo "错误: 无法删除 OpenCV 的 '$build_subdir' 目录。"; exit 1; fi
        else
            echo "继续使用 OpenCV 现有的 '$build_subdir' 文件夹。"
        fi
    fi
fi

mkdir -p "$build_subdir"
cd "$build_subdir" || { echo "错误: 无法进入 OpenCV 的 '$build_subdir' 目录"; exit 1; }

OPENCV_CMAKE_ARGS="-DCMAKE_BUILD_TYPE=Release"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DCMAKE_INSTALL_PREFIX=$INSTALL_PREFIX"
fi

# Contrib 模块
if [ -n "$OPENCV_CONTRIB_SRC_DIR" ] && [ "$OPENCV_CONTRIB_SRC_DIR" != "skip" ]; then
    # 需要确保路径是绝对的，或者相对于OpenCV源码目录的相对路径是正确的
    # CMake期望的是 opencv_contrib/modules 目录
    contrib_modules_path="$OPENCV_CONTRIB_SRC_DIR/modules"
    if [[ "$OPENCV_CONTRIB_SRC_DIR" != /* ]]; then # 如果不是绝对路径，则假定相对于当前目录(即opencv源码目录的父目录)
         # This might need adjustment if script is not run from workspace root
         # Assuming CONTRIB_SRC_DIR is relative to where the main opencv src dir is, or absolute
         # For safety, let's try to make it absolute if it's not already.
         # This part can be tricky if paths are not well-defined.
         # A simpler approach: require OPENCV_CONTRIB_SRC_DIR to be an absolute path or relative to $original_dir
        if [ -d "$original_dir/$OPENCV_CONTRIB_SRC_DIR/modules" ]; then
            contrib_modules_path="$original_dir/$OPENCV_CONTRIB_SRC_DIR/modules"
        elif [ -d "$OPENCV_CONTRIB_SRC_DIR" ] && [ -d "$OPENCV_CONTRIB_SRC_DIR/modules" ]; then
             # It might be a path directly to the contrib repo
             : # contrib_modules_path is already set correctly as relative
        else
            echo "警告: 无法确定 opencv_contrib/modules 的有效路径: $contrib_modules_path. 请确保路径正确。"
        fi
    fi
    echo "使用 Contrib 模块路径: $contrib_modules_path"
    OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DOPENCV_EXTRA_MODULES_PATH=$contrib_modules_path"
fi

# 常用编译选项
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DBUILD_EXAMPLES=OFF"
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DBUILD_TESTS=OFF"
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DBUILD_PERF_TESTS=OFF"
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DBUILD_DOCS=OFF"
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DWITH_CUDA=OFF" # 根据需要开启
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DBUILD_opencv_python3=ON" # 或 python2
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DOPENCV_ENABLE_NONFREE=OFF" # 根据需要开启
OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DOPENCV_GENERATE_PKGCONFIG=ON"

# 特定于操作系统的设置
if [ "$OS_TYPE" == "Darwin" ]; then
    echo "在 macOS 上，添加特定框架链接。"
    OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DWITH_OPENCL=OFF" # OpenCL在macOS上可能复杂
    OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DWITH_FFMPEG=OFF" # FFMPEG需要单独安装
    # macOS 上的 Video I/O 通常通过 AVFoundation
    OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DWITH_AVFOUNDATION=ON"
    # 如果需要JPEG, PNG, TIFF等，确保libjpeg, libpng, libtiff已安装
    # brew install jpeg libpng libtiff
    OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DBUILD_JPEG=ON -DBUILD_PNG=ON -DBUILD_TIFF=ON"
fi

if [ "$OS_TYPE" == "Linux" ]; then
    # Linux 上通常需要 GTK 支持 GUI, 以及其他媒体库如 FFMPEG, GStreamer
    # 示例: sudo apt-get install libgtk2.0-dev pkg-config libavcodec-dev libavformat-dev libswscale-dev
    # OPENCV_CMAKE_ARGS="$OPENCV_CMAKE_ARGS -DWITH_GTK=ON"
    echo "Linux系统，确保已安装必要的开发库 (GTK, FFMPEG, anaconda等)。"
fi

echo "配置 OpenCV (在 $(pwd))..."
echo "OpenCV CMake 参数: $OPENCV_CMAKE_ARGS"
# shellcheck disable=SC2086
cmake .. $OPENCV_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: OpenCV 的 CMake 配置失败。"
    cd "$original_dir"
    exit 1
fi

echo "编译 OpenCV (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: OpenCV 的 Make 编译失败。"
    cd "$original_dir"
    exit 1
fi
echo "OpenCV 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 OpenCV 到 $INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: OpenCV 的 sudo make install 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "OpenCV 安装完成。"
else
    echo "跳过 OpenCV 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "OpenCV 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
