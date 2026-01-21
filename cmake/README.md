# CMake 跨平台配置模块

本目录包含可复用的 CMake 模块，用于统一管理所有 pybind11 项目的配置。

## 📁 文件说明

### 核心模块（已应用到所有项目）

#### `FindPythonCrossplatform.cmake`
**功能：** 跨平台 Python 环境自动检测

- ✅ 支持 macOS、Linux (Ubuntu)、Windows
- ✅ 自动检测当前激活的 Python 环境（虚拟环境优先）
- ✅ 避免 Python 版本不匹配导致的运行时错误

**使用方法：**
```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FindPythonCrossplatform.cmake)
```

#### `OptimizationFlags.cmake`
**功能：** 跨平台编译优化配置

- ✅ Release 模式：`-O3 -DNDEBUG -march=native -ffast-math` (GCC/Clang)
- ✅ Release 模式：`/O2 /DNDEBUG` (MSVC)
- ✅ 链接时优化 (LTO)

**使用方法：**
```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/OptimizationFlags.cmake)

# 应用到目标
target_compile_options(your_target PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(your_target PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()
```

### 模板文件

#### `CMakeLists_template.txt`
**功能：** 新项目的 CMakeLists.txt 起始模板

包含完整的跨平台配置、Python 检测和优化设置。创建新项目时复制此文件并修改项目名称和源文件列表。

## 🚀 性能提升

使用这些模块后的实际性能：

| 项目              | 操作     | 加速比  | 精度     |
|-------------------|----------|---------|----------|
| butterworth_filter| filtfilt | 1.26x   | 2.4e-14  |
| butterworth_filter| lfilter  | 1.01x   | 0.0      |

## 📦 已应用的项目

- ✅ `dsp/butterworth_filter/`
- ✅ `dsp/sliding_window/`
- ✅ `adb/`
- ✅ `cv/apriltag_detection/`
- ✅ `cv/camera_models/`
- ✅ `visualization/pangolin_viewer/`

## 💡 使用示例

### 完整的 CMakeLists.txt 模板

```cmake
cmake_minimum_required(VERSION 3.10)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)
project(your_project_name)

set(CMAKE_CXX_STANDARD 17)

# 引入跨平台模块
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/FindPythonCrossplatform.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/OptimizationFlags.cmake)

find_package(pybind11 REQUIRED)

# 创建静态库
add_library(your_lib STATIC src/your_code.cpp)
target_compile_options(your_lib PRIVATE ${OPTIMIZATION_FLAGS})
target_include_directories(your_lib PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/src)

# 创建 Python 模块
pybind11_add_module(your_module your_pybind.cpp)
target_link_libraries(your_module PRIVATE your_lib)
target_compile_options(your_module PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(your_module PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()

set_target_properties(your_module PROPERTIES 
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib)
```

## 🔧 故障排除

### Python 版本不匹配

**症状：** `ImportError: symbol not found`

**解决：**
```bash
rm -rf build lib/*.so
python example.py  # 使用正确的 Python 环境
```

### 手动指定 Python 路径

```bash
cmake -DPYTHON_EXECUTABLE=/path/to/python ..
```

## 📝 更新日志

### 2026-01-21
- ✅ 创建跨平台 Python 检测模块
- ✅ 创建统一的优化配置模块
- ✅ 应用到所有 pybind11 项目
- ✅ 清理旧模板文件
