#!/bin/bash

# https://blog.csdn.net/RadiantJeral/article/details/112757233

# 脚本功能：使用 Boost.Build (b2) 编译并可选地安装 Boost 库
#
# 参数:
#   1. boost_src_dir: Boost 源代码的根目录路径 (必需)
#   2. global_install_flag: 是否执行 sudo ./b2 install (true/false) (必需)

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <boost_source_directory> <global_install_flag (true/false)>"
    exit 1
fi

BOOST_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

if [ ! -d "$BOOST_SRC_DIR" ]; then
    echo "错误: Boost 源代码目录 '$BOOST_SRC_DIR' 不存在或不是一个目录。"
    exit 1
fi

cd "$BOOST_SRC_DIR" || { echo "错误: 无法进入 Boost 源代码目录 '$BOOST_SRC_DIR'"; exit 1; }

echo "----------------------------------------------------"
echo "Checking for and initializing submodules within Boost source directory..."
# Check if the current directory (which is BOOST_SRC_DIR) is a git repo.
# The -d .git check is a common quick check; git rev-parse is more robust.
if [ -d ".git" ] || git rev-parse --is-inside-work-tree > /dev/null 2>&1; then
    echo "Boost source directory appears to be a Git repository."
    echo "Attempting: git submodule update --init --recursive"
    if git submodule update --init --recursive; then
        echo "Boost submodules updated successfully."
    else
        # This is a warning, not a fatal error, as Boost might not have submodules,
        # they might be optional, or already up to date and the command returns non-zero for other reasons.
        echo "警告: 'git submodule update --init --recursive' 在 Boost 源码目录中执行时失败或未执行任何操作。"
        echo "如果 Boost 没有子模块或它们已是最新状态，这可能是正常的。"
    fi
else
    echo "Boost source directory does not appear to be a Git repository. Skipping submodule update check."
fi
echo "----------------------------------------------------"

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

# --- 编译和安装 Boost 使用 b2 ---
echo "----------------------------------------------------"
echo "开始处理 Boost (使用 b2)"
echo "Boost 源代码目录: $BOOST_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$BOOST_SRC_DIR" || { echo "错误: 无法进入 Boost 源代码目录 '$BOOST_SRC_DIR'"; exit 1; }

# 1. 运行 bootstrap.sh
bootstrap_cmd="./bootstrap.sh"
bootstrap_args=""

# 检查 bootstrap.sh 是否存在
if [ ! -f "$bootstrap_cmd" ]; then
    # 有些老的 Boost 版本可能还在使用 bjam 直接作为 bootstrap
    if [ -f "./bjam" ]; then
        echo "未找到 bootstrap.sh，但找到了 bjam。尝试使用 bjam 进行引导..."
        bootstrap_cmd="./bjam --setup" # 这只是一个猜测，具体命令可能不同
    else
        echo "错误: 未找到 bootstrap.sh (或 bjam) 脚本。请确保 Boost 源码完整。"
        cd "$original_dir"
        exit 1
    fi
fi


if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    bootstrap_args="--prefix=$INSTALL_PREFIX"
fi

# 自动检测工具集，或允许用户通过环境变量 BOOST_TOOLSET 指定
toolset_arg=""
if [ -n "$BOOST_TOOLSET" ]; then
    toolset_arg="--with-toolset=$BOOST_TOOLSET"
    echo "使用用户指定的工具集: $BOOST_TOOLSET"
elif [ "$OS_TYPE" == "Darwin" ]; then
    toolset_arg="--with-toolset=clang" # macOS 通常使用 clang
    echo "在 macOS 上，默认使用 clang 工具集。"
else
    # Linux 上，让 bootstrap 自动检测 (通常是 gcc)
    echo "在 Linux 上，允许 bootstrap 自动检测编译器工具集。"
fi

echo "运行引导脚本: $bootstrap_cmd $bootstrap_args $toolset_arg ..."
# shellcheck disable=SC2086 # Allow word splitting for args
$bootstrap_cmd $bootstrap_args $toolset_arg
if [ $? -ne 0 ]; then
    echo "错误: Boost 的引导过程 ($bootstrap_cmd) 失败。"
    cd "$original_dir"
    exit 1
fi
echo "Boost 引导完成。"

# 2. 运行 b2 (或 bjam)
# 确定 b2 命令的名称 (新版本是 b2, 老版本可能是 bjam)
b2_cmd="./b2"
if [ ! -f "$b2_cmd" ]; then
    if [ -f "./bjam" ]; then
        b2_cmd="./bjam"
    else
        echo "错误: 未找到 b2 或 bjam 命令。引导过程可能不完整。"
        cd "$original_dir"
        exit 1
    fi
fi
echo "将使用 $b2_cmd 进行编译和安装。"


# b2 编译选项
# link=shared: 构建共享库
# runtime-link=shared: 动态链接C++运行时 (通常推荐)
# variant=release: 构建release版本
# --layout=system: 使用标准的系统目录布局 (如 lib, include)
# address-model=64: 64位 (在现代系统上通常是默认)
# threading=multi: 多线程支持
# --build-dir=./build_b2 : 指定一个构建目录，避免污染源码树顶层
# 考虑默认编译哪些库，或者让用户通过环境变量 WITH_LIBRARIES="filesystem system thread" 指定
# 如果不指定 --with-<library> 或 --without-<library>, b2会尝试编译大部分常用库
b2_build_args="-j${NUM_CORES} --build-dir=./build_b2_temp link=shared runtime-link=shared variant=release threading=multi --layout=system"

if [ -n "$WITH_LIBRARIES" ]; then
    b2_build_args="$b2_build_args --with-libraries=$WITH_LIBRARIES"
    echo "将只编译指定的库: $WITH_LIBRARIES"
else
    echo "将编译 Boost 默认的库集合 (或所有可构建的库)。这可能需要较长时间。"
    # 可以考虑添加一组常用库作为默认，例如：
    # b2_build_args="$b2_build_args --with-filesystem --with-system --with-thread --with-program_options --with-date_time --with-chrono --with-regex --with-serialization --with-atomic"
fi


echo "编译 Boost (使用 $b2_cmd $b2_build_args) ..."
# shellcheck disable=SC2086
$b2_cmd $b2_build_args
if [ $? -ne 0 ]; then
    echo "错误: Boost 的编译 ($b2_cmd) 失败。"
    cd "$original_dir"
    exit 1
fi
echo "Boost 编译完成。"

# 3. 安装 (如果标志为 true)
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    # 对于 install，也需要传递所有编译选项，尤其是 prefix
    b2_install_args="-j${NUM_CORES} --prefix=$INSTALL_PREFIX --build-dir=./build_b2_temp link=shared runtime-link=shared variant=release threading=multi --layout=system"
    if [ -n "$WITH_LIBRARIES" ]; then
       b2_install_args="$b2_install_args --with-libraries=$WITH_LIBRARIES"
    fi
    # 注意：`install` 目标本身可能不需要所有构建参数，但传递它们通常无害。
    # 核心的是 --prefix 必须在这里生效。
    echo "安装 Boost (使用 $b2_cmd install $b2_install_args) ..."
    # shellcheck disable=SC2086
    sudo $b2_cmd install $b2_install_args
    if [ $? -ne 0 ]; then
        echo "错误: Boost 的安装 (sudo $b2_cmd install) 失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "Boost 安装完成到 $INSTALL_PREFIX。"
else
    echo "跳过 Boost 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
    echo "Boost 已在 $BOOST_SRC_DIR/build_b2_temp (临时构建目录) 和 stage (链接库) 编译完成。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "Boost 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
