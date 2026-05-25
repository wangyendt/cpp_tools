# cpp_tools

面向 Python 与 C++ 复用的 C++ 工具集合。工具既可以被 sparse checkout 后作为 C++ 源码引用，也可以通过 pybind11 编译成 Python 扩展模块，并由 Python 包装类在首次导入失败时自动调用 `gettool` 拉取、编译和安装到本地 `lib/`。

---

## 📚 目录

- [简介](#-简介)
- [与 gettool.py 的关系](#-与-gettoolpy-的关系)
- [现有工具](#-现有工具)
- [快速开始](#-快速开始)
- [当前工程边界](#-当前工程边界)
- [新增工具完整指南](#-新增工具完整指南)
- [AI 提示词模板](#-ai-提示词模板)
- [目录结构规范](#-目录结构规范)
- [故障排除](#-故障排除)

---

## 🎯 简介

`cpp_tools` 的核心目标是把重计算或强依赖系统能力的模块沉淀成一套可复用工具：

- **C++ 源码复用**：通过 `gettool <tool>` 或 sparse checkout 获取指定工具源码，需要时直接 include。
- **Python 扩展复用**：每个可构建工具提供 pybind11 模块，编译产物输出到工具目录的 `lib/`。
- **Python 包装类按需安装**：`pywayne` 侧包装类优先 import 本地 `lib/`，失败后自动执行 `gettool <tool> -b -t <lib_path>`。
- **macOS/Linux 优先**：现有 CI 覆盖 macOS 和 Ubuntu。Windows 不是当前一等支持目标，不能按“开箱即用”承诺。
- **自包含 CMake 配置**：每个工具的 `CMakeLists.txt` 内联 Python 检测和优化配置，但 OpenCV、Eigen、Boost、Ceres、Pangolin 等系统依赖仍需被安装或由安装脚本处理。

当前真正的用户路径是：Python 用户 import 包装类，包装类自动调用 `gettool` 获取 `.so/.dylib`；C++ 用户调用 `gettool <tool> -c` 获取精简源码，或直接 sparse checkout 工具目录。

---

## 🔗 与 gettool.py 的关系

### 工作流程

```
┌─────────────────┐
│   cpp_tools     │  GitHub 仓库
│   (本仓库)      │  https://github.com/wangyendt/cpp_tools
└────────┬────────┘
         │
         │ git clone --sparse
         │
         ▼
┌─────────────────┐
│   gettool.py    │  下载和编译工具
│                 │  位于 wayne_algorithm_lib/bin/
└────────┬────────┘
         │
         │ 1. sparse checkout 指定工具
         │ 2. 可选运行 cmake && make
         │ 3. 复制源码或 lib/ 到目标目录
         │
         ▼
┌─────────────────┐
│  pywayne/*/lib  │  Python 包装类的本地扩展目录
│  或用户指定路径 │  import butterworth_filter
└─────────────────┘
```

### 配置文件：name_to_path_map.yaml

`gettool.py` 通过 `name_to_path_map.yaml` 找到工具路径、判断是否可构建、以及定位依赖安装脚本。当前配置是对象格式，不是简单的 `name: path`：

```yaml
# cpp_tools/name_to_path_map.yaml
butterworth_filter:
  path: dsp/butterworth_filter
  buildable: true
  installable: false

opencv:
  path: third_party/opencv
  buildable: false
  installable: true
  installation:
    install_script: install_scripts/install_opencv.sh
    extra_dependencies:
      - opencv_contrib
```

### 使用方法

```bash
# 在 wayne_algorithm_lib/bin/ 目录下
python gettool.py -l                        # 列出所有可用工具
python gettool.py butterworth_filter        # 下载源码
python gettool.py butterworth_filter -b     # 下载并编译
python gettool.py butterworth_filter -c     # 仅复制 src/include，供 C++ include 使用
python gettool.py apriltag_detection -b -t /path/to/lib
```

### Python 包装类中的自动安装模式

`wayne_algorithm_lib` 中的包装类已经使用了统一思路：

```python
from pywayne.cpp_loader import import_cpp_module

lib_path = os.path.join(os.path.dirname(__file__), "lib")
module = import_cpp_module("apriltag_detection", "apriltag_detection", lib_path)
```

这条链路的关键前提是：`gettool` 在 `PATH` 中可执行，CMake 能找到当前 Python 环境、pybind11 和工具依赖，编译后的模块名与 `PYBIND11_MODULE(...)` 完全一致。公共 helper 会保留 import 原始错误，避免把“动态库加载失败”误判成“模块不存在”。

---

## 📦 现有工具

| 工具名 | 路径 | 功能 | 性能提升 |
|--------|------|------|----------|
| `butterworth_filter` | `dsp/butterworth_filter/` | Butterworth 滤波器 (filtfilt/lfilter) | 1.25x vs SciPy |
| `sliding_window_dsp` | `dsp/sliding_window/` | 滑动窗口统计 (MKAverage, WelfordStd) | 高性能 |
| `apriltag_detection` | `cv/apriltag_detection/` | AprilTag 检测 | - |
| `camera_models` | `cv/camera_models/` | 相机模型 (Pinhole, Fisheye) | - |
| `pangolin_viewer` | `visualization/pangolin_viewer/` | Pangolin 可视化 | - |
| `adb_logcat_reader` | `adb/` | Android Logcat 解析 | - |

---

## 🚀 快速开始

### 安装依赖

```bash
# macOS，按工具需要追加 boost/ceres/glew 等依赖
brew install cmake pybind11 eigen opencv

# Ubuntu，按工具需要追加 libboost-all-dev/libceres-dev/libglew-dev 等依赖
sudo apt install cmake pybind11-dev libeigen3-dev libopencv-dev

# 或使用提供的安装脚本
cd install_scripts/
./install_pybind11.sh
./install_eigen.sh
```

### 使用现有工具

```bash
# 方式 1: 通过 gettool.py（推荐）
cd wayne_algorithm_lib/bin/
python gettool.py butterworth_filter -b

# 方式 2: 直接在 cpp_tools 仓库编译
cd cpp_tools/dsp/butterworth_filter/
python example.py  # 自动编译并运行
```

### 在 Python 中直接使用 pybind 模块

```python
import sys
import numpy as np
sys.path.append('lib')
import butterworth_filter as bf

# 创建滤波器
filter = bf.ButterworthFilter.from_params(order=4, fs=1.0, btype="lowpass", cutoff=[0.1])

# 滤波
x = np.ascontiguousarray(signal, dtype=np.float64)
filtered = filter.filtfilt(x)
```

### 在 pywayne 中使用包装类

包装类负责自动检测并补齐本地扩展库。例如：

```python
from pywayne.cv.apriltag_detector import ApriltagCornerDetector

detector = ApriltagCornerDetector()
detections = detector.detect("test.png")
```

---

## 🚧 当前工程边界

这套流程已经具备雏形，但要做到任意 macOS/Linux 设备上真正开箱即用，还需要补齐下面几块：

- **统一工具元数据**：`name_to_path_map.yaml` 已开始记录 `python_module`、`bundle_runtime_dependencies` 和部分 OpenCV 组件，但还需要补齐产物文件模式、系统依赖、Python 依赖、C++ clean copy、headless 能力等 schema。
- **统一 Python 安装助手**：`pywayne` 侧已有公共 `import_cpp_module()`，`apriltag_detector`、`camera_model`、`pangolin_utils`、`logcat_reader` 已迁移；后续新包装类应复用这一入口。
- **gettool 可恢复构建**：当前已能使用当前 Python、`cmake --build`、运行时依赖打包和 import 验证；还需要增加依赖预检、构建缓存和更明确的系统包建议。
- **依赖自动安装策略**：简单工具只需 pybind11；CV/可视化工具依赖 OpenCV、Eigen、Boost、Ceres、Pangolin、OpenGL/GLEW。应优先使用系统包管理器或已有环境，源码安装脚本作为明确选择，而不是默认长时间编译。
- **CI 覆盖与发布产物**：CI 已覆盖 macOS/Ubuntu，但工具列表缺少 `dsp/butterworth_filter`，也没有把构建矩阵直接回写为可供 `gettool` 使用的二进制索引。
- **C++ include 模式**：`gettool -c` 能复制源码，但还没有稳定的 include manifest，调用方仍需要知道头文件路径、源文件和依赖链接方式。

---

## 🛠️ 新增工具完整指南

### 标准目录结构（必须遵循）

```
your_tool_name/
├── CMakeLists.txt              # CMake 配置（自包含版本）
├── your_tool_pybind.cpp        # pybind11 绑定代码
├── example.py                  # 示例和测试代码（必须包含自动编译）
└── src/                        # C++ 源代码（必须在此目录）
    ├── YourClass.h             # 头文件
    └── YourClass.cpp           # 实现文件
```

### 新增工具的 6 个步骤

#### 步骤 1: 创建目录结构

```bash
cd cpp_tools/
mkdir -p category/your_tool_name/src
cd category/your_tool_name/
```

**分类建议：**
- `dsp/` - 信号处理
- `cv/` - 计算机视觉
- `optimization/` - 优化算法
- `ml/` - 机器学习
- `utils/` - 通用工具

#### 步骤 2: 编写 C++ 代码

在 `src/` 目录下创建头文件和实现：

```cpp
// src/YourClass.h
#pragma once
#include <vector>

class YourClass {
public:
    YourClass(int param);
    std::vector<double> process(const std::vector<double>& input);
private:
    int param_;
};
```

```cpp
// src/YourClass.cpp
#include "YourClass.h"

YourClass::YourClass(int param) : param_(param) {}

std::vector<double> YourClass::process(const std::vector<double>& input) {
    // 实现你的算法
    return input;
}
```

#### 步骤 3: 复制 CMakeLists.txt 模板

```bash
cp ../../cmake/CMakeLists_standalone.txt CMakeLists.txt
```

**修改以下 3 处：**

```cmake
# 1. 项目名称（第 13 行）
project(category_your_tool_name)  # 例如：dsp_butterworth_filter

# 2. 静态库配置（第 100-120 行）
add_library(
    category_your_tool_name    # ← 改这里
    STATIC
    src/YourClass.cpp          # ← 改这里：你的源文件
)

target_include_directories(
    category_your_tool_name    # ← 改这里
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_options(category_your_tool_name PRIVATE ${OPTIMIZATION_FLAGS})  # ← 改这里

set_target_properties(
    category_your_tool_name    # ← 改这里
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)

# 3. Python 模块配置（第 122-140 行）
pybind11_add_module(your_tool_name your_tool_pybind.cpp)  # ← 改这里：模块名和pybind文件名

target_link_libraries(
    your_tool_name             # ← 改这里：模块名
    PRIVATE category_your_tool_name  # ← 改这里：静态库名
)

target_compile_options(your_tool_name PRIVATE ${OPTIMIZATION_FLAGS})  # ← 改这里：模块名
if(NOT MSVC)
    target_link_options(your_tool_name PRIVATE ${OPTIMIZATION_LINK_FLAGS})  # ← 改这里：模块名
endif()

set_target_properties(
    your_tool_name             # ← 改这里：模块名
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
```

**验证配置：**

```bash
cd ../../
python cmake/cmake_manager.py check
```

#### 步骤 4: 编写 pybind11 绑定

创建 `your_tool_pybind.cpp`：

```cpp
#include <pybind11/pybind11.h>
#include <pybind11/stl.h>
#include "YourClass.h"

namespace py = pybind11;

PYBIND11_MODULE(your_tool_name, m) {
    m.doc() = "Your tool description";
    
    py::class_<YourClass>(m, "YourClass")
        .def(py::init<int>(), py::arg("param"))
        .def("process", &YourClass::process, 
             py::arg("input"),
             "Process input data");
}
```

**常用 pybind11 模式：**

```cpp
// 基本类型
.def("method", &Class::method, py::arg("x"), py::arg("y"))

// std::vector
#include <pybind11/stl.h>
std::vector<double> → list[float]

// Eigen
#include <pybind11/eigen.h>
Eigen::MatrixXd → np.ndarray

// NumPy buffer
#include <pybind11/numpy.h>
py::array_t<double> input
```

#### 步骤 5: 编写 example.py

创建 `example.py`（必须包含自动编译逻辑）：

```python
#!/usr/bin/env python3
"""
your_tool_name 使用示例

功能：
1. 自动编译 C++ 代码
2. 基本功能测试
3. 性能对比（如果有 Python 等价实现）
"""

import os
import sys
import subprocess
import numpy as np
import time

# ============================================================================
# 自动编译（必须包含）
# ============================================================================
def build():
    """自动编译 C++ 模块"""
    build_dir = os.path.join(os.path.dirname(__file__), 'build')
    os.makedirs(build_dir, exist_ok=True)
    
    print("🔨 编译 C++ 模块...")
    subprocess.run(["cmake", ".."], cwd=build_dir, check=True)
    subprocess.run(["cmake", "--build", ".", "-j", str(os.cpu_count() or 1)], cwd=build_dir, check=True)
    
    print("✅ 编译成功\n")

# 确保编译
build()

# 添加库路径
sys.path.append(os.path.join(os.path.dirname(__file__), 'lib'))
import your_tool_name

# ============================================================================
# 测试代码
# ============================================================================
print("="*80)
print("测试 your_tool_name")
print("="*80)

# 创建对象
obj = your_tool_name.YourClass(param=10)

# 准备测试数据
data = np.random.randn(1000)

# 测试功能
result = obj.process(data.tolist())
print(f"输入长度: {len(data)}")
print(f"输出长度: {len(result)}")

# 性能测试
n_iter = 100
start = time.perf_counter()
for _ in range(n_iter):
    result = obj.process(data.tolist())
end = time.perf_counter()

avg_time = (end - start) / n_iter * 1000
print(f"\n平均耗时: {avg_time:.3f} ms")

print("\n✅ 测试通过")
```

#### 步骤 6: 注册到 name_to_path_map.yaml

编辑 `cpp_tools/name_to_path_map.yaml`：

```yaml
# 添加你的工具
your_tool_name:
  path: category/your_tool_name
  buildable: true
  installable: false
```

不要直接运行当前版本的 `generate_name_to_path_map.py` 来更新正式配置；它会按旧的简单 `name: path` 结构重写文件，不保留 `buildable/installable/installation` 元数据。新增工具时应手动补齐元数据，或先升级生成脚本。

### 测试流程

```bash
# 1. 本地测试
cd category/your_tool_name/
python example.py

# 2. 验证配置
cd ../../
python cmake/cmake_manager.py check

# 3. 提交到 GitHub
git add .
git commit -m "Add your_tool_name"
git push

# 4. 通过 gettool 测试
cd ~/wayne_algorithm_lib/bin/
python gettool.py your_tool_name -b
```

---

## 🤖 AI 提示词模板

### 模板 1: 从零创建新工具

```
我想在 cpp_tools 仓库中创建一个新的 C++ 工具，并通过 pybind11 提供 Python 接口。

**工具信息：**
- 工具名：<your_tool_name>
- 分类：<dsp/cv/optimization/utils>
- 功能：<简要描述功能>
- 核心算法：<关键算法或公式>

**必须满足的要求：**

1. **目录结构**（必须遵循）：

<category>/<your_tool_name>/
├── CMakeLists.txt
├── <your_tool_name>_pybind.cpp
├── example.py
└── src/
    ├── YourClass.h
    └── YourClass.cpp

2. **CMakeLists.txt 配置**：
   - 从 `cmake/CMakeLists_standalone.txt` 复制模板
   - 修改项目名为 `<category>_<your_tool_name>`
   - 修改静态库名为 `<category>_<your_tool_name>`
   - 修改 Python 模块名为 `<your_tool_name>`
   - 源文件必须在 `src/` 目录下
   - 完成后运行 `python cmake/cmake_manager.py check` 验证

3. **C++ 代码**（必须在 src/ 目录）：
   - 实现高性能算法
   - 提供清晰的接口
   - 使用 std::vector 或 Eigen 处理数据

4. **pybind11 绑定**：
   - 文件名：`<your_tool_name>_pybind.cpp`
   - 模块名：`PYBIND11_MODULE(<your_tool_name>, m)`
   - 包含必要的 pybind11 头文件（stl.h, eigen.h 等）
   - 绑定所有公共接口

5. **example.py**（必须包含以下部分）：

   - 自动编译逻辑：`def build()` + `subprocess.run(["cmake", ...])`
   - 导入模块：sys.path.append('lib') + import
   - 功能测试
   - 性能对比（与 Python/NumPy/SciPy 等价实现对比）
   - 输出结果

6. **注册工具**：
   - 在 `name_to_path_map.yaml` 中添加 `path/buildable/installable` 对象配置

**参考示例：**
请参考 `dsp/butterworth_filter/` 的完整实现，它是一个标准的工具示例。

开始实现吧！
```

### 模板 2: 移植现有 C++ 代码

```
我有一个现有的 C++ 类/库，想集成到 cpp_tools 仓库并提供 Python 接口。

**现有代码：**
<粘贴你的 C++ 代码或描述>

**目标：**
1. 将代码整合到 cpp_tools 的标准结构中
2. 创建 pybind11 绑定
3. 编写 example.py 展示功能
4. 与 Python 等价实现进行性能对比

**必须遵循的规范：**
- 源代码放在 `src/` 目录
- 使用 `cmake/CMakeLists_standalone.txt` 模板
- example.py 包含自动编译逻辑
- 完成后用 `python cmake/cmake_manager.py check` 验证

**参考示例：**
- 完整实现：dsp/butterworth_filter/
- CMakeLists.txt：cmake/CMakeLists_standalone.txt
- pybind11 绑定：butterworth_filter_pybind.cpp
- example.py：包含自动编译、测试、性能对比

开始移植吧！
```

### 模板 3: 添加新功能到现有工具

```
我想为 cpp_tools 中的现有工具添加新功能。

**工具路径：** `<category>/<tool_name>/`

**新功能：**
<描述要添加的功能>

**任务：**
1. 修改 `src/` 下的 C++ 代码
2. 更新 pybind11 绑定
3. 在 example.py 中添加新功能的测试
4. 确保向后兼容

**验证流程：**
1. cd <category>/<tool_name>/
2. python example.py
3. 确保所有功能测试通过

开始修改吧！
```

### 模板 4: 性能优化

```
我想优化 cpp_tools 中某个工具的性能。

**工具：** `<category>/<tool_name>/`

**当前性能：** <描述当前性能指标>

**优化目标：**
- [ ] 减少内存分配
- [ ] 使用 SIMD 指令
- [ ] 并行化计算
- [ ] 算法优化

**要求：**
1. 修改 C++ 实现
2. 在 example.py 中添加性能对比
3. 确保正确性（精度测试）
4. 文档化性能提升

开始优化吧！
```

---

## 📋 目录结构规范

### 完整目录结构示例

```
cpp_tools/
├── cmake/                              # CMake 工具和模板
│   ├── CMakeLists_standalone.txt      # 新项目模板（必须使用）
│   ├── cmake_manager.py               # 自动化管理工具
│   └── README.md                      # CMake 详细文档
│
├── dsp/                               # 信号处理工具
│   ├── butterworth_filter/            # 标准示例 ✨
│   │   ├── CMakeLists.txt            # 自包含配置
│   │   ├── butterworth_filter_pybind.cpp
│   │   ├── example.py                # 自动编译 + 测试
│   │   ├── src/
│   │   │   ├── butterworth_filter.h
│   │   │   └── butterworth_filter.cpp
│   │   └── lib/                      # 编译输出（自动生成）
│   │       └── butterworth_filter.cpython-*.so
│   │
│   └── sliding_window/               # 另一个示例
│       ├── CMakeLists.txt
│       ├── sliding_window_dsp_pybind.cpp
│       ├── example.py
│       └── src/
│           ├── MKAverage.h
│           ├── MKAverage.cpp
│           ├── WelfordStd.h
│           └── WelfordStd.cpp
│
├── cv/                               # 计算机视觉工具
│   ├── apriltag_detection/
│   └── camera_models/
│
├── visualization/                    # 可视化工具
│   └── pangolin_viewer/
│
├── install_scripts/                  # 依赖安装脚本
│   ├── install_pybind11.sh
│   ├── install_eigen.sh
│   └── ...
│
├── name_to_path_map.yaml            # gettool.py 配置（必须更新）
├── generate_name_to_path_map.py     # 旧版生成脚本，升级前不要覆盖正式 YAML
├── README.md                        # 本文件
└── LICENSE
```

### 必须遵循的规则

1. **源代码位置** ✅
   ```
   ✅ your_tool/src/YourClass.cpp
   ❌ your_tool/YourClass.cpp
   ```

2. **CMakeLists.txt** ✅
   ```
   ✅ 使用 cmake/CMakeLists_standalone.txt 模板
   ❌ 自己手写 CMakeLists.txt
   ```

3. **example.py** ✅
   ```python
   ✅ 包含自动编译逻辑
   def build():
       subprocess.run(["cmake", ".."], cwd=build_dir, check=True)
       subprocess.run(["cmake", "--build", "."], cwd=build_dir, check=True)
   
   ❌ 手动运行 cmake
   ```

4. **pybind11 文件名** ✅
   ```
   ✅ butterworth_filter_pybind.cpp
   ✅ your_tool_name_pybind.cpp
   ❌ pybind.cpp
   ❌ bindings.cpp
   ```

5. **name_to_path_map.yaml** ✅
   ```yaml
   ✅ your_tool_name:
        path: category/your_tool_name
        buildable: true
        installable: false
   ❌ 忘记添加
   ```

---

## 🔧 故障排除

### 问题 1: CMake 找不到 Python

**症状：**
```
CMake Error: Could not find Python
```

**解决：**
```bash
# 激活正确的 Python 环境
conda activate your_env

# 验证
which python
python --version

# 清理并重新编译
rm -rf build lib/*.so
python example.py
```

### 问题 2: ImportError: symbol not found

**症状：**
```
ImportError: dlopen(...): symbol not found
```

**原因：** Python 版本不匹配

**解决：**
```bash
# 确保使用同一个 Python
which python  # 应该指向你的虚拟环境

# 重新编译
rm -rf build lib
python example.py
```

### 问题 3: gettool.py 找不到工具

**症状：**
```
Tool 'your_tool' not found
```

**解决：**
```bash
# 检查 name_to_path_map.yaml
cat name_to_path_map.yaml | grep your_tool

# 如果没有，按对象格式添加：
# your_tool_name:
#   path: category/your_tool_name
#   buildable: true
#   installable: false

# 提交到 GitHub
git add name_to_path_map.yaml
git commit -m "Add your_tool to name_to_path_map"
git push
```

### 问题 4: 编译时找不到头文件

**症状：**
```
fatal error: 'YourClass.h' file not found
```

**解决：**
```cmake
# 检查 CMakeLists.txt 中的 target_include_directories
target_include_directories(
    your_tool_name
    PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src  # ← 确保包含 src/
)
```

### 问题 5: 配置不一致

**症状：** 不确定配置是否正确

**解决：**
```bash
cd cpp_tools/
python cmake/cmake_manager.py check

# 输出示例：
# ✅ 使用自包含配置  dsp/butterworth_filter/CMakeLists.txt
# ⚠️  使用旧的模块化配置  your_tool/CMakeLists.txt  ← 需要更新
```

---

## 📚 相关文档

- [cmake/README.md](cmake/README.md) - CMake 详细配置文档
- [install_scripts/SCRIPT_GUIDELINES.md](install_scripts/SCRIPT_GUIDELINES.md) - 依赖安装指南
- [docs/roadmap/](docs/roadmap/) - 开箱即用构建与分发路线图
- [docs/plan/](docs/plan/) - 近期可执行改造计划

---

## 🤝 贡献指南

1. Fork 本仓库
2. 创建你的工具（遵循上述规范）
3. 运行 `python cmake/cmake_manager.py check` 验证
4. 提交 Pull Request

---

## 📄 License

MIT License - 详见 [LICENSE](LICENSE)

---

## 🎯 总结：新增工具核心检查清单

创建新工具时，确保满足以下所有条件：

- [ ] 目录结构正确：`category/tool_name/src/`
- [ ] 从 `cmake/CMakeLists_standalone.txt` 复制并修改 3 处
- [ ] C++ 代码在 `src/` 目录下
- [ ] 创建了 `tool_name_pybind.cpp`
- [ ] `example.py` 包含自动编译逻辑
- [ ] `example.py` 包含功能测试和性能对比
- [ ] 添加到 `name_to_path_map.yaml`
- [ ] 运行 `python cmake/cmake_manager.py check` ✅
- [ ] 本地测试通过：`python example.py` ✅
- [ ] 通过 gettool 测试：`python gettool.py tool_name -b` ✅

**全部完成后，你的工具就可以被 `gettool.py` 自动下载和编译了！** 🎉
