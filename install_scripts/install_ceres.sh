#!/bin/bash

# 脚本功能：编译并可选地安装 glog, gflags, SuiteSparse, 和 Ceres Solver
#
# 参数:
#   1. ceres_src_dir: Ceres Solver 源代码的根目录 (必需)
#   2. glog_src_dir: glog 源代码的根目录 (可选, "skip" 或 "" 跳过)
#   3. gflags_src_dir: gflags 源代码的根目录 (可选, "skip" 或 "" 跳过)
#   4. suitesparse_src_dir: SuiteSparse 源代码的根目录 (可选, "skip" 或 "" 跳过)
#   5. global_install_flag: 是否为所有通过此脚本编译的库执行 sudo make install (true/false) (必需)
#
# 依赖项Eigen应该已经被安装在系统路径 (如 /usr/local)

set -e # 如果任何命令失败，立即退出

# --- 参数解析和检查 ---
if [ "$#" -ne 5 ]; then
    echo "错误: 需要五个参数。"
    echo "用法: $0 <ceres_src_dir> <glog_src_dir|"skip"> <gflags_src_dir|"skip"> <suitesparse_src_dir|"skip"> <global_install_flag>"
    exit 1
fi

CERES_SRC_DIR="$1"
GLOG_SRC_DIR="$2"
GFLAGS_SRC_DIR="$3"
SUITESPARSE_SRC_DIR="$4"
GLOBAL_INSTALL_FLAG_INPUT="$5"

# 将全局安装标志转换为小写
GLOBAL_INSTALL_FLAG=$(echo "$GLOBAL_INSTALL_FLAG_INPUT" | tr '[:upper:]' '[:lower:]')

# 检查 Ceres 路径
if [ ! -d "$CERES_SRC_DIR" ]; then
    echo "错误: Ceres Solver 源代码目录 '$CERES_SRC_DIR' 不存在或不是一个目录。"
    exit 1
fi

# --- 辅助函数 ---

# 函数：编译和可选地安装一个通用 CMake 项目
# $1: 项目名称 (用于日志)
# $2: 源代码目录
# $3: 额外的 CMake 参数 (字符串形式)
# $4: 安装前缀 (通常是 /usr/local)
# $5: 全局安装标志 (true/false) - 决定此项目是否安装
compile_cmake_project() {
    local project_name="$1"
    local src_dir="$2"
    local extra_cmake_args="$3"
    local install_prefix="$4"
    local current_global_install_flag="$5" # 接收全局安装标志
    local build_subdir="build_for_ceres_script" # 使用特定子目录名 for dependencies

    echo "----------------------------------------------------"
    echo "处理项目: $project_name"
    echo "源代码目录: $src_dir"
    echo "安装标志对此项目: $current_global_install_flag"
    echo "----------------------------------------------------"

    if [ -z "$src_dir" ] || [ "$src_dir" = "skip" ]; then
        echo "跳过 $project_name 的处理。"
        return
    fi

    if [ ! -d "$src_dir" ]; then
        echo "警告: $project_name 源代码目录 '$src_dir' 不存在。跳过..."
        return
    fi

    local original_dir=$(pwd)
    cd "$src_dir" || { echo "错误: 无法进入目录 '$src_dir'"; exit 1; }

    if [ -d "$build_subdir" ]; then
        non_interactive_mode_enabled=$(echo "$NON_INTERACTIVE_INSTALL" | tr '[:upper:]' '[:lower:]')
        if [ "$non_interactive_mode_enabled" = "true" ]; then
            echo "NON_INTERACTIVE_INSTALL=true. 目录 '$build_subdir' 已存在于 $project_name 中。自动删除并重建..."
            echo "正在删除 '$build_subdir'..."
            rm -rf "$build_subdir"
            if [ $? -ne 0 ]; then echo "错误: 无法删除目录 '$build_subdir'。"; exit 1; fi
        else
            echo "目录 '$build_subdir' 已存在于 $project_name 中。"
            printf "是否要删除并重建 '$build_subdir' 文件夹? (y/N): "
            read -r response
            response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
            if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
                echo "正在删除 '$build_subdir'..."
                rm -rf "$build_subdir"
                if [ $? -ne 0 ]; then echo "错误: 无法删除目录 '$build_subdir'。"; exit 1; fi
            else
                echo "继续使用现有的 '$build_subdir' 文件夹。"
            fi
        fi
    fi

    mkdir -p "$build_subdir"
    cd "$build_subdir" || { echo "错误: 无法进入目录 '$build_subdir'"; exit 1; }

    echo "配置 $project_name (在 $(pwd))..."
    # shellcheck disable=SC2086 # 允许 extra_cmake_args 被分割
    cmake .. -DCMAKE_INSTALL_PREFIX="$install_prefix" -DCMAKE_BUILD_TYPE=Release $extra_cmake_args
    if [ $? -ne 0 ]; then
        echo "错误: $project_name 的 CMake 配置失败。"
        cd "$original_dir"
        exit 1
    fi

    echo "编译 $project_name (使用 $NUM_CORES 核心)..."
    make -j"$NUM_CORES"
    if [ $? -ne 0 ]; then
        echo "错误: $project_name 的 Make 编译失败。"
        cd "$original_dir"
        exit 1
    fi
    echo "$project_name 编译完成。"

    if [ "$current_global_install_flag" = "true" ]; then
        echo "安装 $project_name 到 $install_prefix..."
        sudo make install
        if [ $? -ne 0 ]; then
            echo "错误: $project_name 的 sudo make install 失败。"
            cd "$original_dir"
            exit 1
        fi
        echo "$project_name 安装完成。"
    else
         echo "跳过 $project_name 的安装步骤 (全局安装标志为 '$current_global_install_flag')。"
    fi

    cd "$original_dir"
    echo "$project_name 处理完毕。"
}


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

# --- 全局安装前缀 ---
GLOBAL_INSTALL_PREFIX="/usr/local"

# --- 编译和安装依赖项 ---

# gflags (必须在 glog 之前安装)
compile_cmake_project "gflags" "$GFLAGS_SRC_DIR" "-DBUILD_SHARED_LIBS=ON -DGFLAGS_NAMESPACE=google" "$GLOBAL_INSTALL_PREFIX" "$GLOBAL_INSTALL_FLAG"

# glog
# 指定 C++14 标准，因为 glog 使用了相关特性 (如 std::underlying_type_t, std::exchange, chrono_literals)
compile_cmake_project "glog" "$GLOG_SRC_DIR" "-DCMAKE_CXX_STANDARD=14" "$GLOBAL_INSTALL_PREFIX" "$GLOBAL_INSTALL_FLAG"

# SuiteSparse
compile_cmake_project "SuiteSparse" "$SUITESPARSE_SRC_DIR" "" "$GLOBAL_INSTALL_PREFIX" "$GLOBAL_INSTALL_FLAG"


# --- 编译和安装 Ceres Solver ---
echo "----------------------------------------------------"
echo "开始处理 Ceres Solver"
echo "源代码目录: $CERES_SRC_DIR"
echo "全局安装标志: $GLOBAL_INSTALL_FLAG"
echo "----------------------------------------------------"

original_dir_ceres=$(pwd)
cd "$CERES_SRC_DIR" || { echo "错误: 无法进入 Ceres 源代码目录 '$CERES_SRC_DIR'"; exit 1; }

echo "DEBUG: Ceres处理阶段，当前 PWD: $(pwd)"

# For Ceres itself, we always attempt to build. Installation is optional.
# We use a specific build directory name to avoid conflict if the user has their own "build"
ceres_build_dir="build_ceres_from_script"

echo "DEBUG: ceres_build_dir 变量值: '${ceres_build_dir}'"
echo "DEBUG: NON_INTERACTIVE_INSTALL 环境变量值: '${NON_INTERACTIVE_INSTALL}'"

if [ -d "$ceres_build_dir" ]; then
    echo "DEBUG: 目录 '$ceres_build_dir' (即 build_ceres_from_script) 已存在。"
    non_interactive_mode_enabled=$(echo "$NON_INTERACTIVE_INSTALL" | tr '[:upper:]' '[:lower:]')
    if [ "$non_interactive_mode_enabled" = "true" ]; then
        echo "NON_INTERACTIVE_INSTALL=true. Ceres 构建目录 '$ceres_build_dir' 已存在。自动删除并重建..."
        echo "正在删除 '$PWD/$ceres_build_dir'..." # PWD 用于确认路径
        rm -rf "$ceres_build_dir"
        if [ $? -ne 0 ]; then echo "错误: 无法删除目录 '$ceres_build_dir'。"; exit 1; fi
        echo "目录 '$ceres_build_dir' 已删除。"
    else
        echo "Ceres 构建目录 '$ceres_build_dir' 已存在。"
        printf "是否要删除并重建它? (y/N): "
        read -r response
        response_lower=$(echo "$response" | tr '[:upper:]' '[:lower:]')
        if [ "$response_lower" = "y" ] || [ "$response_lower" = "yes" ]; then
            echo "正在删除 '$PWD/$ceres_build_dir'..."
            rm -rf "$ceres_build_dir"
            if [ $? -ne 0 ]; then echo "错误: 无法删除目录 '$ceres_build_dir'。"; exit 1; fi
            echo "目录 '$ceres_build_dir' 已删除。"
        else
            echo "用户选择不删除现有目录 '$ceres_build_dir'。脚本将尝试在此目录中构建。"
        fi
    fi
else
    echo "DEBUG: 目录 '$ceres_build_dir' (即 build_ceres_from_script) 不存在。"
fi

echo "DEBUG: 同时检查一个名为 'build' 的目录是否存在:"
ls -ld "build" 2>/dev/null || echo "DEBUG: 'build' 目录不存在或ls失败"

echo "DEBUG: 即将执行: mkdir \"$ceres_build_dir\""
mkdir "$ceres_build_dir"
cd "$ceres_build_dir" || { echo "错误: 无法进入 Ceres 构建目录 '$ceres_build_dir'"; exit 1; }

CERES_CMAKE_ARGS="-DCMAKE_INSTALL_PREFIX=$GLOBAL_INSTALL_PREFIX -DCMAKE_BUILD_TYPE=Release"
CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DBUILD_EXAMPLES=OFF -DBUILD_TESTING=OFF"

    # --- 新增: 明确指定依赖路径给 Ceres (当 GLOBAL_INSTALL_FLAG=false) ---
    if [ "$GLOBAL_INSTALL_FLAG" = "false" ]; then
        echo "(全局安装关闭) 正在为 Ceres 配置本地依赖项路径..."
        LOCAL_DEPS_PREFIX_PATH=""
        if [ -n "$GFLAGS_SRC_DIR" ] && [ "$GFLAGS_SRC_DIR" != "skip" ] && [ -d "$GFLAGS_SRC_DIR/build_for_ceres_script" ]; then
            echo "  提供 gflags 路径: $GFLAGS_SRC_DIR/build_for_ceres_script"
            # Ceres 可能通过 gflags_DIR 或 CMAKE_PREFIX_PATH 找到 gflags
            # CMAKE_PREFIX_PATH 更通用
            LOCAL_DEPS_PREFIX_PATH="$GFLAGS_SRC_DIR/build_for_ceres_script${LOCAL_DEPS_PREFIX_PATH:+;$LOCAL_DEPS_PREFIX_PATH}"
        fi
        if [ -n "$GLOG_SRC_DIR" ] && [ "$GLOG_SRC_DIR" != "skip" ] && [ -d "$GLOG_SRC_DIR/build_for_ceres_script" ]; then
            echo "  提供 glog 路径: $GLOG_SRC_DIR/build_for_ceres_script"
            LOCAL_DEPS_PREFIX_PATH="$GLOG_SRC_DIR/build_for_ceres_script${LOCAL_DEPS_PREFIX_PATH:+;$LOCAL_DEPS_PREFIX_PATH}"
            CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DMINIGLOG=OFF" # 明确告诉 Ceres 不要用 miniglog
        fi
        if [ -n "$SUITESPARSE_SRC_DIR" ] && [ "$SUITESPARSE_SRC_DIR" != "skip" ] && [ -d "$SUITESPARSE_SRC_DIR/build_for_ceres_script" ]; then
            echo "  提供 SuiteSparse 路径: $SUITESPARSE_SRC_DIR/build_for_ceres_script"
            # SuiteSparse 的查找可能比较复杂，CMAKE_PREFIX_PATH 是一个好的开始
            # 可能还需要 -DSUITESPARSE_INCLUDE_DIR_HINTS 和 -DSUITESPARSE_LIBRARY_DIR_HINTS
            LOCAL_DEPS_PREFIX_PATH="$SUITESPARSE_SRC_DIR/build_for_ceres_script${LOCAL_DEPS_PREFIX_PATH:+;$LOCAL_DEPS_PREFIX_PATH}"
            CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DSUITESPARSE=ON" # 尝试启用 SuiteSparse
        fi
        
        if [ -n "$LOCAL_DEPS_PREFIX_PATH" ]; then
            CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DCMAKE_PREFIX_PATH=$LOCAL_DEPS_PREFIX_PATH"
        fi
    else # This is GLOBAL_INSTALL_FLAG = "true"
        # 只有在全局安装开启时，才假设依赖项会被安装到系统路径，从而可以被Ceres找到
        if [ -n "$GLOG_SRC_DIR" ] && [ "$GLOG_SRC_DIR" != "skip" ] && [ -d "$GLOG_SRC_DIR" ]; then # Check if glog was compiled from source by this script
            echo "(全局安装开启) 检测到 glog 从源码编译，Ceres 将使用系统 glog (MINIGLOG=OFF)。"
            CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DMINIGLOG=OFF"
        else
            echo "(全局安装开启) 未从源码编译 glog，Ceres 将尝试使用内置的 MINIGLOG 或系统已有的 glog。"
        fi

        if [ -n "$SUITESPARSE_SRC_DIR" ] && [ "$SUITESPARSE_SRC_DIR" != "skip" ] && [ -d "$SUITESPARSE_SRC_DIR" ]; then # Check if SuiteSparse was compiled from source by this script
            echo "(全局安装开启) 检测到 SuiteSparse 从源码编译，Ceres 将尝试启用 SuiteSparse 支持。"
            CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DSUITESPARSE=ON"
        else
            echo "(全局安装开启) 未从源码编译 SuiteSparse，Ceres 将根据系统情况决定是否启用它。"
        fi
    fi
    # --- 结束新增部分 ---

if [ "$OS_TYPE" == "Darwin" ]; then
    echo "在 macOS 上，尝试为 Ceres 配置 Accelerate 框架 (BLAS/LAPACK)。"
    CERES_CMAKE_ARGS="$CERES_CMAKE_ARGS -DLAPACK=ON -DBLA_VENDOR=Apple"
fi

echo "配置 Ceres Solver (在 $(pwd))..."
echo "Ceres CMake 参数: $CERES_CMAKE_ARGS"
# shellcheck disable=SC2086
cmake .. $CERES_CMAKE_ARGS

if [ $? -ne 0 ]; then
    echo "错误: Ceres Solver 的 CMake 配置失败。"
    cd "$original_dir_ceres"
    exit 1
fi

echo "编译 Ceres Solver (使用 $NUM_CORES 核心)..."
make -j"$NUM_CORES"
if [ $? -ne 0 ]; then
    echo "错误: Ceres Solver 的 Make 编译失败。"
    cd "$original_dir_ceres"
    exit 1
fi
echo "Ceres Solver 编译完成。"

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装 Ceres Solver 到 $GLOBAL_INSTALL_PREFIX..."
    sudo make install
    if [ $? -ne 0 ]; then
        echo "错误: Ceres Solver 的 sudo make install 失败。"
        cd "$original_dir_ceres"
        exit 1
    fi
    echo "Ceres Solver 安装完成。"
else
    echo "跳过 Ceres Solver 的安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG_INPUT')。"
fi

cd "$original_dir_ceres"
echo "----------------------------------------------------"
echo "Ceres Solver 处理脚本执行完毕。"
echo "----------------------------------------------------"
exit 0
