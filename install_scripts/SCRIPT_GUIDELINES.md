# 指南：编写可自动化执行的安装脚本

在编写安装脚本，特别是那些可能会被其他程序（如 `gettool.py`）或在 CI/CD 环境中自动调用的脚本时，务必注意处理用户交互环节。

## 问题：交互式提示导致自动化失败

如果脚本在执行过程中包含需要用户手动输入的步骤（例如，询问是否覆盖文件、是否删除已存在的构建目录、选择配置选项等），那么在非交互式环境下执行时，脚本会因为等待用户输入而挂起，最终可能导致超时或执行失败。

## 解决方案：使用环境变量控制交互行为

为了让脚本既能支持用户手动执行时的交互，又能顺利在自动化环境中运行，推荐使用环境变量来控制脚本的行为模式。

1.  **定义标准环境变量**：
    为此类场景统一定义一个环境变量，例如 `NON_INTERACTIVE_INSTALL`。

2.  **脚本内部逻辑判断**：
    在脚本中涉及到交互式提示的地方，首先检查该环境变量的值。
    *   **非交互模式**：如果 `NON_INTERACTIVE_INSTALL` 被设置为某个预定义的值（例如 `true`，通常进行不区分大小写的比较），脚本应跳过用户提示，并执行一个合理的、预设的默认操作。例如，自动删除并重建已存在的构建目录。
    *   **交互模式**：如果环境变量未设置，或者其值不是预定义的"非交互模式"触发值，则脚本可以按原样执行交互式提示，允许用户在手动运行时做出选择。

## Bash 脚本示例

以下是一个在 Bash 脚本中处理已存在构建目录的示例：

```bash
#!/bin/bash

# ... 其他脚本参数和设置 ...

BUILD_DIR_NAME="build_project_script" # 假设的构建目录名称
DEFAULT_INSTALL_PATH="/usr/local"
GLOBAL_INSTALL_FLAG="false" # 示例全局安装标志

# 模拟从参数获取用户是否希望全局安装
if [ ! -z "$2" ]; then
    GLOBAL_INSTALL_FLAG=$(echo "$2" | tr '[:upper:]' '[:lower:]')
fi

# ... 其他参数解析 ...

echo "进入源码目录..."
# cd /path/to/source_code

if [ -d "$BUILD_DIR_NAME" ]; then
    # 将 NON_INTERACTIVE_INSTALL 环境变量的值转换为小写，以便进行不区分大小写的比较
    non_interactive_mode_enabled=$(echo "$NON_INTERACTIVE_INSTALL" | tr '[:upper:]' '[:lower:]')

    if [ "$non_interactive_mode_enabled" = "true" ]; then
        echo "NON_INTERACTIVE_INSTALL=true. 检测到构建目录 '$BUILD_DIR_NAME' 已存在。将自动删除并重建..."
        echo "正在删除 '$BUILD_DIR_NAME' ..."
        rm -rf "$BUILD_DIR_NAME"
        if [ $? -ne 0 ]; then
            echo "错误: 无法删除目录 '$BUILD_DIR_NAME'。"
            exit 1
        fi
    else
        echo "构建目录 '$BUILD_DIR_NAME' 已存在。"
        printf "是否要删除并重建此文件夹? (y/N): "
        read -r user_response
        user_response_lower=$(echo "$user_response" | tr '[:upper:]' '[:lower:]')
        if [ "$user_response_lower" = "y" ] || [ "$user_response_lower" = "yes" ]; then
            echo "用户选择删除。正在删除 '$BUILD_DIR_NAME' ..."
            rm -rf "$BUILD_DIR_NAME"
            if [ $? -ne 0 ]; then
                echo "错误: 无法删除目录 '$BUILD_DIR_NAME'。"
                exit 1
            fi
        else
            echo "用户选择不删除。继续使用现有的 '$BUILD_DIR_NAME' 文件夹。"
            # 注意: 如果选择不删除，后续的 CMake/make 命令需要能够处理潜在的旧构建状态。
            # 有时，即使不删除，清空目录内容也是一个好主意，或者确保构建工具能正确处理。
        fi
    fi
fi

echo "创建构建目录 '$BUILD_DIR_NAME' ..."
mkdir -p "$BUILD_DIR_NAME"
if [ $? -ne 0 ]; then
    echo "错误: 无法创建目录 '$BUILD_DIR_NAME'。"
    exit 1
fi
cd "$BUILD_DIR_NAME"

echo "配置项目 (CMake)..."
# cmake .. -DCMAKE_INSTALL_PREFIX="$DEFAULT_INSTALL_PATH" -DSOME_OPTION=ON

echo "编译项目 (make)..."
# make -j$(nproc)

if [ "$GLOBAL_INSTALL_FLAG" = "true" ]; then
    echo "安装项目..."
    # sudo make install
else
    echo "跳过安装步骤 (全局安装标志为 '$GLOBAL_INSTALL_FLAG')。"
fi

echo "脚本执行完毕。"
```

## 调用脚本时设置环境变量

当通过自动化工具（如 Python 的 `subprocess`模块）调用这些脚本时，可以在执行前设置相应的环境变量：

**Python 示例:**
```python
import subprocess
import os

# 设置环境变量
env = os.environ.copy()
env['NON_INTERACTIVE_INSTALL'] = 'true'

# 执行脚本
try:
    # 假设脚本路径为 ./install_scripts/install_some_library.sh
    # 并且它需要源码路径作为第一个参数，以及一个可选的安装标志作为第二个参数
    script_path = './install_scripts/install_some_library.sh'
    source_directory = '/path/to/some_library_source'
    should_install_globally = 'false' # 或 'true'

    process = subprocess.run(
        [script_path, source_directory, should_install_globally],
        capture_output=True,
        text=True,
        check=False, # 设置为 False 以便手动检查返回码
        env=env
    )

    print("--- Script STDOUT ---")
    print(process.stdout)
    print("--- Script STDERR ---")
    print(process.stderr)

    if process.returncode == 0:
        print(f"脚本 '{script_path}' 执行成功。")
    else:
        print(f"脚本 '{script_path}' 执行失败，返回码: {process.returncode}")

except FileNotFoundError:
    print(f"错误: 脚本 '{script_path}' 未找到。")
except Exception as e:
    print(f"执行脚本时发生错误: {e}")

```

## 总结

采用这种基于环境变量的控制机制，可以显著提高安装脚本的健壮性和适用性，使其能够平滑地在不同执行环境（手动交互式 vs. 自动非交互式）中切换，避免因交互提示而导致的自动化流程中断。 