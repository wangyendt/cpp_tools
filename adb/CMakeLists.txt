cmake_minimum_required(VERSION 3.10)
project(adb_log)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

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

pybind11_add_module(adb_logcat_reader adb_logcat_reader_pybind.cpp)
target_include_directories(
	adb_logcat_reader
	PRIVATE
	${SOURCE_ROOT}
)

# 添加优化选项
target_compile_options(adb_logcat_reader PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(adb_logcat_reader PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()

set_target_properties(
	adb_logcat_reader
	PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	${CMAKE_CURRENT_SOURCE_DIR}/lib)
