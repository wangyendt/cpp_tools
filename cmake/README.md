# CMake 配置模块说明

## 📁 目录内容

本目录包含两种 CMakeLists.txt 配置方案：

### 1. **CMakeLists_standalone.txt**（推荐✨）
**完全自包含的 CMakeLists.txt 模板**

**优点：**
- ✅ 无需外部依赖，所有配置都内联
- ✅ 下载后即可使用，不需要额外的 `cmake/` 目录
- ✅ 适合通过 `gettool.py` 下载的独立工具
- ✅ 目录结构更清爽

**使用场景：**
- 新建项目
- 通过 `gettool.py` 分发的工具
- 希望工具完全独立的场景

**使用方法：**
```bash
cp cmake/CMakeLists_standalone.txt your_project/CMakeLists.txt
# 修改项目名称和源文件列表
```

**目录结构：**
```
your_project/
├── CMakeLists.txt    # 自包含，无需 cmake/
├── src/
├── example.py
└── lib/              # 编译输出
```

### 2. **FindPythonCrossplatform.cmake + OptimizationFlags.cmake**（模块化）
**可复用的 CMake 模块**

**优点：**
- ✅ 模块化设计，便于维护
- ✅ 多个项目共享同一份配置
- ✅ 修改一处，所有项目生效

**使用场景：**
- cpp_tools 仓库内部的项目（如 `dsp/`, `cv/` 等）
- 多个项目需要保持配置一致

**使用方法：**
```cmake
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FindPythonCrossplatform.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/OptimizationFlags.cmake)
```

**目录结构：**
```
cpp_tools/
├── cmake/                              # 共享模块
│   ├── FindPythonCrossplatform.cmake
│   └── OptimizationFlags.cmake
├── dsp/
│   ├── butterworth_filter/
│   │   └── CMakeLists.txt             # 引用 ../../cmake/
│   └── sliding_window/
│       └── CMakeLists.txt
└── cv/
    └── camera_models/
        └── CMakeLists.txt
```

## 🔄 迁移指南

### 从模块化迁移到自包含

**cpp_tools 仓库的项目现在使用自包含版本：**
```bash
# 已完成迁移的项目
✅ dsp/butterworth_filter/
✅ dsp/sliding_window/
```

**其他项目迁移方法：**
1. 复制 `CMakeLists_standalone.txt` 内容
2. 替换项目的 CMakeLists.txt 开头部分（Python 检测 + 优化配置）
3. 保留项目特定的源文件配置

## 🎯 推荐方案

| 场景 | 推荐方案 |
|------|---------|
| 新项目 | ✨ 自包含版本 |
| 通过 gettool.py 分发 | ✨ 自包含版本 |
| cpp_tools 仓库内部 | ✨ 自包含版本（已迁移） |
| 旧项目维护 | 模块化版本（可继续使用） |

## 🤖 自动化管理工具

为了避免手动维护多个 CMakeLists.txt 的一致性，我们提供了自动化管理工具：

### 使用方法

```bash
cd cmake/

# 检查所有项目的配置状态
python cmake_manager.py check

# 批量迁移所有项目到自包含配置（预览）
python cmake_manager.py migrate-all

# 确认并执行迁移
python cmake_manager.py migrate-all --confirm

# 迁移单个项目
python cmake_manager.py migrate ../dsp/your_project/CMakeLists.txt
```

### 功能说明

1. **`check`** - 检查所有 pybind11 项目的配置状态
   - ✅ 使用自包含配置
   - ⚠️ 使用旧的模块化配置
   - ❌ 缺少 Python 配置

2. **`migrate-all`** - 批量迁移所有项目
   - 自动检测使用模块化配置的项目
   - 替换为自包含配置
   - 支持 dry-run 预览

3. **`migrate <path>`** - 迁移指定项目

### 示例输出

```
🔍 检查所有 CMake 项目...

找到 6 个 pybind11 项目:

  ✅ 使用自包含配置  dsp/butterworth_filter/CMakeLists.txt
  ✅ 使用自包含配置  dsp/sliding_window/CMakeLists.txt
  ✅ 使用自包含配置  adb/CMakeLists.txt
  ✅ 使用自包含配置  cv/apriltag_detection/CMakeLists.txt
  ✅ 使用自包含配置  cv/camera_models/CMakeLists.txt
  ✅ 使用自包含配置  visualization/pangolin_viewer/CMakeLists.txt

📊 统计:
  ✅ 自包含配置: 6
  ⚠️  模块化配置: 0
  ❌ 缺少配置:   0
```

### 新项目创建流程

**推荐方式（零手动维护）：**

```bash
# 1. 复制模板
cp cmake/CMakeLists_standalone.txt your_project/CMakeLists.txt

# 2. 修改项目配置（只需改这几处）
# - project() 名称
# - 源文件列表
# - pybind11 模块名

# 3. 验证配置
python cmake/cmake_manager.py check
```

**优点：**
- ✅ 无需担心配置一致性
- ✅ 工具自动检测和提示
- ✅ 支持批量更新

## 📝 配置说明

### 跨平台 Python 检测

**支持平台：**
- ✅ macOS (`.dylib`)
- ✅ Linux/Ubuntu (`.so`)
- ✅ Windows (`.lib`)

**自动检测：**
1. 优先使用命令行指定：`cmake -DPYTHON_EXECUTABLE=/path/to/python`
2. 自动检测当前激活的虚拟环境
3. 支持 conda、venv、virtualenv

### 编译优化

**GCC/Clang (macOS/Linux):**
```
-O3 -DNDEBUG -march=native -ffast-math -flto
```

**MSVC (Windows):**
```
/O2 /DNDEBUG
```

**性能提升：**
- Butterworth Filter: **1.26x** 加速（相比 SciPy）
- 精度：误差 < 1e-13

## 🔧 故障排除

### Python 版本不匹配

**症状：** `ImportError: symbol not found`

**解决：**
```bash
rm -rf build lib/*.so
python example.py  # 使用正确的 Python 环境
```

### 手动指定 Python

```bash
cmake -DPYTHON_EXECUTABLE=/path/to/python ..
```

## 📚 参考资料

- [pybind11 文档](https://pybind11.readthedocs.io/)
- [CMake FindPython](https://cmake.org/cmake/help/latest/module/FindPython.html)

## 📝 更新日志

### 2026-01-21
- ✅ 创建自包含版本的 CMakeLists.txt 模板
- ✅ 迁移 butterworth_filter 和 sliding_window 到自包含版本
- ✅ 简化 gettool.py，移除 cmake 目录复制逻辑
- ✅ 更优雅的目录结构
