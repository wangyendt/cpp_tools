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

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt") # Add common paths for dependencies
list(APPEND CMAKE_PREFIX_PATH "/usr/local")
include_directories("/opt/homebrew/opt")
include_directories("/usr/local/include")

find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3 REQUIRED NO_MODULE) # Use NO_MODULE for modern Eigen3 CMake config
find_package(pybind11 REQUIRED)
# Boost is needed for shared_ptr used in the library headers
find_package(Boost REQUIRED)
find_package(Ceres REQUIRED)

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

set_target_properties(camera_models PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${PYTHON_MODULE_OUTPUT_DIR}
)

message(STATUS "OpenCV libraries: ${OpenCV_LIBS}")
message(STATUS "Eigen3 include dir: ${EIGEN3_INCLUDE_DIR}")
message(STATUS "pybind11 include dir: ${pybind11_INCLUDE_DIRS}") # Check if pybind11_INCLUDE_DIRS is set
message(STATUS "Boost include dir: ${Boost_INCLUDE_DIRS}")
message(STATUS "Python module output dir: ${PYTHON_MODULE_OUTPUT_DIR}") 
