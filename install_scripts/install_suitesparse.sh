#!/bin/bash

# 脚本功能：编译并可选地安装 SuiteSparse
#
# 参数:
#   1. suitesparse_src_dir: SuiteSparse 源代码的根目录 (必需)
#   2. global_install_flag: 是否执行 sudo make install (true/false) (必需)
#
# 注意: SuiteSparse 的构建可能很复杂。此脚本尝试使用其顶层 Makefile (如果适用)。
#       如果失败，建议通过系统包管理器安装 (e.g., libsuitesparse-dev)。
#       此脚本不处理SuiteSparse的依赖项 (如BLAS, LAPACK, METIS)。这些需要预先安装。

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 2 ]; then
    echo "错误: 需要两个参数。"
    echo "用法: $0 <suitesparse_src_dir> <global_install_flag>"
    exit 1
fi

SUITESPARSE_SRC_DIR="$1"
GLOBAL_INSTALL_FLAG_INPUT="$2"

GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

if [ ! -d "$SUITESPARSE_SRC_DIR" ]; then
    echo "错误: SuiteSparse 源代码目录 '$SUITESPARSE_SRC_DIR' 不存在或不是一个目录。"
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

# --- 编译和安装 SuiteSparse ---
echo "----------------------------------------------------"
echo "开始处理 SuiteSparse"
echo "SuiteSparse 源代码目录: $SUITESPARSE_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装路径 (如果启用): $INSTALL_PREFIX"
fi
echo "重要: 请确保 SuiteSparse 的依赖项 (BLAS, LAPACK, METIS等) 已安装。"
echo "此脚本将尝试使用 SuiteSparse 的顶层 Makefile。"
echo "----------------------------------------------------"

original_dir=$(pwd)
cd "$SUITESPARSE_SRC_DIR" || { echo "错误: 无法进入 SuiteSparse 源代码目录 '$SUITESPARSE_SRC_DIR'"; exit 1; }

if [ ! -f "Makefile" ]; then
    echo "错误: 在 '$SUITESPARSE_SRC_DIR' 中未找到顶层 Makefile。"
    echo "无法继续使用此脚本编译 SuiteSparse。请考虑使用系统包管理器，"
    echo "或者查阅您 SuiteSparse 版本的具体编译说明。"
    cd "$original_dir"
    exit 1
fi

echo "编译 SuiteSparse (使用 $NUM_CORES 核心)..."

make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: SuiteSparse 的 Make 编译失败。"
    echo "请检查依赖项 (BLAS, LAPACK, METIS) 是否已安装并被正确找到。"
    echo "您可能需要手动配置 SuiteSparse_config/SuiteSparse_config.mk。"
    cd "$original_dir"
    exit 1
fi
echo "SuiteSparse 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 SuiteSparse 到 $INSTALL_PREFIX..."
    echo "注意: SuiteSparse 的 'make install' 可能需要手动调整或配置 SuiteSparse_config.mk 以正确设置安装路径 $INSTALL_PREFIX。"
    # 尝试传递 INSTALL 变量，但这取决于Makefile的具体实现
    # 更可靠的方法通常是修改 SuiteSparse_config.mk 文件
    sudo make install INSTALL="$INSTALL_PREFIX"
    if [ $? -ne 0 ]; then
        echo "警告: SuiteSparse 的 sudo make install 使用 INSTALL=$INSTALL_PREFIX 可能未成功或未按预期工作。"
        echo "请检查 $INSTALL_PREFIX 下 SuiteSparse 文件是否已正确安装。"
        echo "如果安装不正确，您可能需要查阅您 SuiteSparse 版本的安装说明，"
        echo "通常涉及修改 SuiteSparse_config/SuiteSparse_config.mk 或使用不同的 make install 变量。"
        # 不因make install的潜在问题而使脚本失败，因为编译可能已成功
    else
        echo "SuiteSparse 安装尝试完成。"
    fi
else
    echo "跳过 SuiteSparse 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
fi

cd "$original_dir"
echo "----------------------------------------------------"
echo "SuiteSparse 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
