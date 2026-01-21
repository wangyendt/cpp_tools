# cpp_tools

高性能 C++ 工具库，通过 pybind11 提供 Python 接口，支持通过 `gettool.py` 自动下载和编译。

---

## 📚 目录

- [简介](#-简介)
- [与 gettool.py 的关系](#-与-gettoolpy-的关系)
- [现有工具](#-现有工具)
- [快速开始](#-快速开始)
- [新增工具完整指南](#-新增工具完整指南)
- [AI 提示词模板](#-ai-提示词模板)
- [目录结构规范](#-目录结构规范)
- [故障排除](#-故障排除)

---

## 🎯 简介

`cpp_tools` 是一个高性能 C++ 工具库集合，特点：

- ✅ **独立版本控制** - 每个工具通过 Git tags 独立管理版本
- ✅ **Python 绑定** - 通过 pybind11 提供 Python 接口
- ✅ **自动化分发** - 通过 `gettool.py` 自动下载、编译、安装
- ✅ **高性能** - C++ 实现，性能优于纯 Python（例如 Butterworth Filter 比 SciPy 快 1.25x）
- ✅ **跨平台** - 支持 macOS、Linux、Windows
- ✅ **自包含配置** - 每个工具都是完全独立的，无需外部依赖

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
         │ 2. 运行 cmake && make
         │ 3. 复制到目标目录
         │
         ▼
┌─────────────────┐
│  bin/dsp/       │  最终安装位置
│  butterworth_   │  import butterworth_filter
│  filter/        │
└─────────────────┘
```

### 配置文件：name_to_path_map.yaml

`gettool.py` 通过 `name_to_path_map.yaml` 找到工具的路径：

```yaml
# cpp_tools/name_to_path_map.yaml
butterworth_filter: dsp/butterworth_filter
sliding_window_dsp: dsp/sliding_window
apriltag_detection: cv/apriltag_detection
camera_models: cv/camera_models
pangolin_viewer: visualization/pangolin_viewer
adb_logcat_reader: adb
```

### 使用方法

```bash
# 在 wayne_algorithm_lib/bin/ 目录下
python gettool.py -l                        # 列出所有可用工具
python gettool.py butterworth_filter        # 下载源码
python gettool.py butterworth_filter -b     # 下载并编译
python gettool.py butterworth_filter -c     # 仅编译（已下载的）
```

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
# macOS
brew install cmake pybind11 eigen opencv

# Ubuntu
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

### 在 Python 中使用

```python
import sys
sys.path.append('lib')
import butterworth_filter as bf

# 创建滤波器
filter = bf.ButterworthFilter(order=4, cutoff=0.1)

# 滤波
filtered = filter.filtfilt(signal)
```

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
    ret = os.system(f"cd {build_dir} && cmake .. && make -j$(nproc)")
    
    if ret != 0:
        print("❌ 编译失败")
        sys.exit(1)
    
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
your_tool_name: category/your_tool_name
```

然后运行：

```bash
cd cpp_tools/
python generate_name_to_path_map.py  # 如果有自动生成脚本
```

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

   - 自动编译逻辑：def build() + os.system()
   - 导入模块：sys.path.append('lib') + import
   - 功能测试
   - 性能对比（与 Python/NumPy/SciPy 等价实现对比）
   - 输出结果

6. **注册工具**：
   - 在 name_to_path_map.yaml 中添加：<your_tool_name>: <category>/<your_tool_name>

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
│   ├── QUICKSTART.md                  # 快速开始指南
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
├── generate_name_to_path_map.py     # 自动生成工具
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
       os.system("mkdir -p build && cd build && cmake .. && make")
   
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
   ✅ your_tool_name: category/your_tool_name
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

# 如果没有，添加：
echo "your_tool_name: category/your_tool_name" >> name_to_path_map.yaml

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
- [cmake/QUICKSTART.md](cmake/QUICKSTART.md) - 5 分钟快速开始
- [install_scripts/SCRIPT_GUIDELINES.md](install_scripts/SCRIPT_GUIDELINES.md) - 依赖安装指南

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
