cmake_minimum_required(VERSION 3.10)

project(cv_corner_detection)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()


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
find_package(Eigen3 REQUIRED)
find_package(pybind11 REQUIRED)
include_directories(
	${EIGEN3_INCLUDE_DIR}
	src/ethz_apriltag2/include
)


add_subdirectory(src/ethz_apriltag2)

# add_executable(example src/examples/detect_apriltag_board_corners.cpp)
# target_link_libraries(example ${OpenCV_LIBS} ethz_apriltag2)


pybind11_add_module(apriltag_detection apriltag_detection_pybind.cpp)
target_link_libraries(apriltag_detection PRIVATE
	ethz_apriltag2
    ${OpenCV_LIBS}
    Eigen3::Eigen
)
target_include_directories(apriltag_detection PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apriltags
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)

# 添加优化选项
target_compile_options(apriltag_detection PRIVATE ${OPTIMIZATION_FLAGS})
if(NOT MSVC)
    target_link_options(apriltag_detection PRIVATE ${OPTIMIZATION_LINK_FLAGS})
endif()

set_target_properties(apriltag_detection PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
