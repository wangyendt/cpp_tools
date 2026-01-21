cmake_minimum_required(VERSION 3.10)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

project(dsp_butterworth_filter)

set(CMAKE_CXX_STANDARD 17)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()


# list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
# list(APPEND CMAKE_PREFIX_PATH "/usr/local/include")
# include_directories("/opt/homebrew/opt")
# include_directories("/usr/local/include")

# ============================================================================
# 跨平台 Python 环境检测和编译优化
# ============================================================================
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/FindPythonCrossplatform.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/../../cmake/OptimizationFlags.cmake)

find_package(pybind11 CONFIG REQUIRED)

add_library(
	dsp_butterworth_filter
	STATIC
	src/butterworth_filter.cpp
	)
target_include_directories(
	dsp_butterworth_filter
	PUBLIC
	${CMAKE_CURRENT_SOURCE_DIR}/src
)
# 为静态库添加优化选项
target_compile_options(dsp_butterworth_filter PRIVATE ${OPTIMIZATION_FLAGS})

set_target_properties(
    dsp_butterworth_filter 
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib)

pybind11_add_module(butterworth_filter butterworth_filter_pybind.cpp)
target_link_libraries(
	butterworth_filter
	PRIVATE
	dsp_butterworth_filter)

# 为 Python 模块添加优化选项
target_compile_options(butterworth_filter PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(butterworth_filter PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()

set_target_properties(
	butterworth_filter
	PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	${CMAKE_CURRENT_SOURCE_DIR}/lib)