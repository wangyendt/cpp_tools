cmake_minimum_required(VERSION 3.10)
project(PangolinViewerPy)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()

# Include FetchContent module
include(FetchContent)

# 显示FetchContent的下载进度
set(FETCHCONTENT_QUIET OFF)

# ============================================================================
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
string(REPLACE "\n" ";" PYTHON_VERSION_LIST ${PYTHON_VERSION_INFO})
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

# ============================================================================
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

find_package(pybind11 REQUIRED)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(OpenGL REQUIRED)
find_package(GLEW REQUIRED)

# Use FetchContent to download and build Pangolin v0.8
FetchContent_Declare(
    pangolin
    GIT_REPOSITORY https://github.com/stevenlovegrove/Pangolin.git
    GIT_TAG v0.8
    GIT_SHALLOW TRUE
    GIT_SUBMODULES ""
)

# 设置Pangolin编译选项 - 使用和成功版本相同的配置
set(BUILD_SHARED_LIBS ON CACHE BOOL "Build using shared libraries")
set(BUILD_PANGOLIN_PYTHON OFF CACHE BOOL "Build support for Python")  # 禁用Python避免pybind11冲突
set(BUILD_EXAMPLES ON CACHE BOOL "Build Examples")
set(BUILD_TOOLS ON CACHE BOOL "Build Tools")
# 强制使用GLEW以改善OpenGL兼容性
set(HAVE_GLEW ON CACHE BOOL "Force GLEW usage")
# 保持一些可选的依赖关闭，避免复杂性
set(BUILD_PANGOLIN_FFMPEG OFF CACHE BOOL "Build support for ffmpeg")
set(BUILD_PANGOLIN_LIBDC1394 OFF CACHE BOOL "Build support for DC1394")
set(BUILD_PANGOLIN_OPENNI OFF CACHE BOOL "Build support for OpenNI")
set(BUILD_PANGOLIN_OPENNI2 OFF CACHE BOOL "Build support for OpenNI2")
set(BUILD_PANGOLIN_REALSENSE OFF CACHE BOOL "Build support for librealsense")

FetchContent_MakeAvailable(pangolin)

add_library(
    pangolin_viewer_lib
    ${LIBRARY_TYPE}
    ${SOURCE_ROOT}/PangolinViewer.cpp
    ${SOURCE_ROOT}/PangolinViewer.h)

target_link_libraries(
    pangolin_viewer_lib
    PUBLIC
    pango_core pango_display pango_opengl pango_windowing 
    pango_plot pango_vars pango_image pango_geometry 
    pango_glgeometry pango_scene pango_video pango_tools
    ${OpenCV_LIBS}
    ${OPENGL_LIBRARIES}
    GLEW::GLEW)

# 根据平台设置编译定义
if(APPLE)
    target_compile_definitions(
        pangolin_viewer_lib
        PUBLIC
        _OSX_
        GL_SILENCE_DEPRECATION
        HAVE_GLEW)
elseif(UNIX)
    target_compile_definitions(
        pangolin_viewer_lib
        PUBLIC
        _LINUX_
        HAVE_GLEW)
endif()

# 优雅地添加Pangolin组件include路径
set(PANGOLIN_COMPONENTS 
    pango_core pango_display pango_opengl pango_windowing 
    pango_vars pango_image pango_geometry pango_glgeometry 
    pango_plot pango_scene pango_video pango_tools pango_packetstream)

set(PANGOLIN_INCLUDE_DIRS "")
foreach(COMPONENT ${PANGOLIN_COMPONENTS})
    list(APPEND PANGOLIN_INCLUDE_DIRS ${pangolin_SOURCE_DIR}/components/${COMPONENT}/include)
endforeach()

target_include_directories(
    pangolin_viewer_lib 
    PUBLIC
    ${SOURCE_ROOT}
    ${EIGEN3_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${OPENGL_INCLUDE_DIR}
    ${PANGOLIN_INCLUDE_DIRS})

# 添加优化选项到静态库
target_compile_options(pangolin_viewer_lib PRIVATE ${OPTIMIZATION_FLAGS})

set_target_properties(
    pangolin_viewer_lib 
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# Assuming PangolinViewer_pybind.cpp contains your pybind11 bindings
pybind11_add_module(pangolin_viewer pangolin_viewer_pybind.cpp)
target_link_libraries(
    pangolin_viewer
    PRIVATE
    pangolin_viewer_lib)

# 添加优化选项到 Python 模块
target_compile_options(pangolin_viewer PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(pangolin_viewer PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()

set_target_properties(
    pangolin_viewer 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib)
