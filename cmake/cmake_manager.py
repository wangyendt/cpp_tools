#!/usr/bin/env python3
"""
CMakeLists.txt 管理工具

功能：
1. 检查所有 pybind11 项目的 CMakeLists.txt 是否使用自包含配置
2. 自动更新或生成符合标准的 CMakeLists.txt
3. 验证配置的正确性

使用方法：
    python cmake_manager.py check          # 检查所有项目
    python cmake_manager.py update <path>  # 更新指定项目
    python cmake_manager.py validate       # 验证配置一致性
"""

import os
import sys
import re
from pathlib import Path
from typing import List, Dict, Tuple

# 标准的 Python 检测和优化配置代码块（用于检测和插入）
PYTHON_DETECTION_BLOCK = '''# ============================================================================
# 跨平台 Python 环境检测（自包含版本）
# ============================================================================
if(DEFINED PYTHON_EXECUTABLE)
    set(Python_EXECUTABLE "${PYTHON_EXECUTABLE}")
    message(STATUS "Using Python executable from command line: ${Python_EXECUTABLE}")
else()
    if(WIN32)
        execute_process(COMMAND where python OUTPUT_VARIABLE DETECTED_PYTHON OUTPUT_STRIP_TRAILING_WHITESPACE)
    else()
        execute_process(COMMAND which python OUTPUT_VARIABLE DETECTED_PYTHON OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    set(Python_EXECUTABLE "${DETECTED_PYTHON}")
    message(STATUS "Detected Python executable: ${Python_EXECUTABLE}")
endif()

execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sys; print(sys.version_info[0]); print(sys.version_info[1])"
    OUTPUT_VARIABLE PYTHON_VERSION_INFO OUTPUT_STRIP_TRAILING_WHITESPACE
)
string(REPLACE "\\n" ";" PYTHON_VERSION_LIST ${PYTHON_VERSION_INFO})
list(GET PYTHON_VERSION_LIST 0 PYTHON_VERSION_MAJOR)
list(GET PYTHON_VERSION_LIST 1 PYTHON_VERSION_MINOR)

execute_process(
    COMMAND ${Python_EXECUTABLE} -c "import sys; print(sys.prefix)"
    OUTPUT_VARIABLE Python_ROOT_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
)

if(WIN32)
    set(Python_INCLUDE_DIRS "${Python_ROOT_DIR}/include")
    set(Python_LIBRARIES "${Python_ROOT_DIR}/libs/python${PYTHON_VERSION_MAJOR}${PYTHON_VERSION_MINOR}.lib")
elseif(APPLE)
    set(Python_INCLUDE_DIRS "${Python_ROOT_DIR}/include/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
    set(Python_LIBRARIES "${Python_ROOT_DIR}/lib/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.dylib")
else()
    set(Python_INCLUDE_DIRS "${Python_ROOT_DIR}/include/python${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
    if(EXISTS "${Python_ROOT_DIR}/lib/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.so")
        set(Python_LIBRARIES "${Python_ROOT_DIR}/lib/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.so")
    elseif(EXISTS "${Python_ROOT_DIR}/lib/x86_64-linux-gnu/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.so")
        set(Python_LIBRARIES "${Python_ROOT_DIR}/lib/x86_64-linux-gnu/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.so")
    elseif(EXISTS "${Python_ROOT_DIR}/lib/aarch64-linux-gnu/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.so")
        set(Python_LIBRARIES "${Python_ROOT_DIR}/lib/aarch64-linux-gnu/libpython${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}.so")
    else()
        set(Python_LIBRARIES "")
    endif()
endif()

set(CMAKE_PREFIX_PATH ${Python_ROOT_DIR} ${CMAKE_PREFIX_PATH})
find_package(Python COMPONENTS Interpreter Development REQUIRED)

message(STATUS "========================================")
message(STATUS "Python Configuration:")
message(STATUS "  Platform: ${CMAKE_SYSTEM_NAME}")
message(STATUS "  Python_EXECUTABLE: ${Python_EXECUTABLE}")
message(STATUS "  Python version: ${PYTHON_VERSION_MAJOR}.${PYTHON_VERSION_MINOR}")
message(STATUS "  Python_ROOT_DIR: ${Python_ROOT_DIR}")
message(STATUS "========================================")
'''

OPTIMIZATION_BLOCK = '''# ============================================================================
# 编译优化配置（自包含版本）
# ============================================================================
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

if(MSVC)
    set(OPTIMIZATION_FLAGS /O2 /DNDEBUG)
    set(OPTIMIZATION_LINK_FLAGS /O2)
else()
    set(OPTIMIZATION_FLAGS -O3 -DNDEBUG -march=native -ffast-math)
    set(OPTIMIZATION_LINK_FLAGS -O3 -flto)
endif()
message(STATUS "Optimization flags: ${OPTIMIZATION_FLAGS}")
'''


class CMakeProject:
    """表示一个 CMake 项目"""
    
    def __init__(self, cmake_file: Path):
        self.path = cmake_file
        self.content = cmake_file.read_text()
        self.is_pybind_project = 'pybind11' in self.content
        
    def uses_standalone_config(self) -> bool:
        """检查是否使用自包含配置"""
        return '跨平台 Python 环境检测（自包含版本）' in self.content
    
    def uses_modular_config(self) -> bool:
        """检查是否使用模块化配置"""
        return 'include(' in self.content and ('FindPythonCrossplatform.cmake' in self.content or 'OptimizationFlags.cmake' in self.content)
    
    def get_status(self) -> str:
        """获取项目状态"""
        if not self.is_pybind_project:
            return "⚪ 非 pybind11 项目"
        elif self.uses_standalone_config():
            return "✅ 使用自包含配置"
        elif self.uses_modular_config():
            return "⚠️  使用旧的模块化配置"
        else:
            return "❌ 缺少 Python 配置"


def find_cmake_projects(root_dir: Path) -> List[CMakeProject]:
    """查找所有 CMake 项目"""
    projects = []
    
    # 排除 third_party 目录
    for cmake_file in root_dir.rglob('CMakeLists.txt'):
        if 'third_party' in str(cmake_file):
            continue
        if 'build' in str(cmake_file):
            continue
            
        projects.append(CMakeProject(cmake_file))
    
    return projects


def check_all_projects(root_dir: Path):
    """检查所有项目的配置状态"""
    print("🔍 检查所有 CMake 项目...\n")
    
    projects = find_cmake_projects(root_dir)
    pybind_projects = [p for p in projects if p.is_pybind_project]
    
    print(f"找到 {len(pybind_projects)} 个 pybind11 项目:\n")
    
    standalone_count = 0
    modular_count = 0
    missing_count = 0
    
    for project in pybind_projects:
        rel_path = project.path.relative_to(root_dir)
        status = project.get_status()
        print(f"  {status}  {rel_path}")
        
        if "✅" in status:
            standalone_count += 1
        elif "⚠️" in status:
            modular_count += 1
        elif "❌" in status:
            missing_count += 1
    
    print(f"\n📊 统计:")
    print(f"  ✅ 自包含配置: {standalone_count}")
    print(f"  ⚠️  模块化配置: {modular_count}")
    print(f"  ❌ 缺少配置:   {missing_count}")
    
    if modular_count > 0:
        print(f"\n💡 提示: 有 {modular_count} 个项目仍在使用旧的模块化配置")
        print(f"   运行 'python {sys.argv[0]} migrate-all' 可批量迁移")


def migrate_project(cmake_file: Path, dry_run: bool = False) -> bool:
    """迁移单个项目到自包含配置"""
    project = CMakeProject(cmake_file)
    
    if not project.is_pybind_project:
        print(f"⚪ 跳过非 pybind11 项目: {cmake_file}")
        return False
    
    if project.uses_standalone_config():
        print(f"✅ 已是自包含配置: {cmake_file}")
        return False
    
    if not project.uses_modular_config():
        print(f"❓ 未检测到标准配置: {cmake_file}")
        return False
    
    # 替换模块化配置为自包含配置
    content = project.content
    
    # 移除 include 语句和相关注释
    include_pattern = r'# ============================================================================\s*\n# 跨平台.*?\n# ============================================================================\s*\ninclude\([^\)]+FindPythonCrossplatform\.cmake\)\s*\ninclude\([^\)]+OptimizationFlags\.cmake\)\s*\n'
    
    if re.search(include_pattern, content):
        replacement = PYTHON_DETECTION_BLOCK + '\n' + OPTIMIZATION_BLOCK + '\n'
        new_content = re.sub(include_pattern, replacement, content)
        
        if not dry_run:
            cmake_file.write_text(new_content)
            print(f"✅ 已迁移: {cmake_file}")
        else:
            print(f"🔄 将迁移: {cmake_file}")
        
        return True
    else:
        print(f"❌ 未找到标准模式: {cmake_file}")
        return False


def migrate_all_projects(root_dir: Path, dry_run: bool = False):
    """批量迁移所有项目"""
    print(f"🚀 {'[DRY RUN] ' if dry_run else ''}批量迁移到自包含配置...\n")
    
    projects = find_cmake_projects(root_dir)
    pybind_projects = [p for p in projects if p.is_pybind_project and p.uses_modular_config()]
    
    if not pybind_projects:
        print("✅ 所有项目已使用自包含配置")
        return
    
    print(f"找到 {len(pybind_projects)} 个需要迁移的项目:\n")
    
    migrated = 0
    for project in pybind_projects:
        if migrate_project(project.path, dry_run):
            migrated += 1
    
    print(f"\n📊 {'将' if dry_run else '已'}迁移 {migrated} 个项目")
    
    if dry_run:
        print(f"\n💡 运行 'python {sys.argv[0]} migrate-all --confirm' 执行实际迁移")


def main():
    """主函数"""
    if len(sys.argv) < 2:
        print(__doc__)
        sys.exit(1)
    
    command = sys.argv[1]
    
    # 获取 cpp_tools 根目录（此脚本在 cmake/ 目录下）
    root_dir = Path(__file__).parent.parent
    
    if command == 'check':
        check_all_projects(root_dir)
    
    elif command == 'migrate-all':
        dry_run = '--confirm' not in sys.argv
        migrate_all_projects(root_dir, dry_run)
    
    elif command == 'migrate':
        if len(sys.argv) < 3:
            print("用法: python cmake_manager.py migrate <CMakeLists.txt路径>")
            sys.exit(1)
        
        cmake_file = Path(sys.argv[2])
        if not cmake_file.exists():
            print(f"❌ 文件不存在: {cmake_file}")
            sys.exit(1)
        
        migrate_project(cmake_file)
    
    else:
        print(f"❌ 未知命令: {command}")
        print(__doc__)
        sys.exit(1)


if __name__ == '__main__':
    main()
