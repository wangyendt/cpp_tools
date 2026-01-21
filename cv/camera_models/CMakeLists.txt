cmake_minimum_required(VERSION 3.14)

project(cv_camera_models)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# --- Configuration ---
set(SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(PYBIND_FILE ${CMAKE_CURRENT_SOURCE_DIR}/camera_models_pybind.cpp)
set(PYTHON_MODULE_OUTPUT_DIR ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# --- Find Packages ---
option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)

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

find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3 REQUIRED NO_MODULE) # Use NO_MODULE for modern Eigen3 CMake config
find_package(pybind11 REQUIRED)
# Boost is needed for shared_ptr used in the library headers
find_package(Boost REQUIRED)
find_package(Ceres REQUIRED)

get_target_property(CERES_INCLUDE_DIRS Ceres::ceres INTERFACE_INCLUDE_DIRECTORIES)

include_directories(
    ${INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
    ${Boost_INCLUDE_DIRS}
    ${CERES_INCLUDE_DIRS}
    # pybind11 include dir is handled by target_link_libraries(pybind11::headers)
)

# --- Build C++ Library ---
file(GLOB SRC_FILES ${SOURCE_DIR}/*.cpp)

add_library(camera_models_lib ${SRC_FILES})
# 添加优化选项到静态库
target_compile_options(camera_models_lib PRIVATE ${OPTIMIZATION_FLAGS})

set_target_properties(camera_models_lib PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin
)
target_link_libraries(camera_models_lib PRIVATE
    ${OpenCV_LIBS}
    Eigen3::Eigen
    Boost::boost # Link Boost headers/components if needed, often headers are enough
    # Ceres::ceres
)

# --- Build Python Module ---
if(NOT EXISTS ${PYBIND_FILE})
    message(FATAL_ERROR "Pybind source file not found: ${PYBIND_FILE}")
endif()

pybind11_add_module(camera_models ${PYBIND_FILE})
target_link_libraries(camera_models PRIVATE
    camera_models_lib
    ${OpenCV_LIBS}
    Eigen3::Eigen
    Boost::boost
    # Ceres::ceres
    pybind11::module # Link pybind11 module library
)
# Include directories already set globally, but could be set per target
# target_include_directories(camera_models PRIVATE ...)

# 添加优化选项到 Python 模块
target_compile_options(camera_models PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(camera_models PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()

set_target_properties(camera_models PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PYTHON_MODULE_OUTPUT_DIR}
)

message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "Eigen3 include dir: ${EIGEN3_INCLUDE_DIR}")
message(STATUS "pybind11 include dir: ${pybind11_INCLUDE_DIRS}") # Check if pybind11_INCLUDE_DIRS is set
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
message(STATUS "Python module output dir: ${PYTHON_MODULE_OUTPUT_DIR}") 
