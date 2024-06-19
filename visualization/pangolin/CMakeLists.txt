cmake_minimum_required(VERSION 3.10)
project(PangolinViewerPy)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)
set(CMAKE_CXX_STANDARD 14)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()


list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/include")
include_directories("/opt/homebrew/opt")
include_directories("/usr/local/include")


find_package(pybind11 REQUIRED)
find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Pangolin REQUIRED)


add_library(
    pangolin_viewer_lib
    ${LIBRARY_TYPE}
    ${SOURCE_ROOT}/PangolinViewer.cpp
    ${SOURCE_ROOT}/PangolinViewer.h)
target_link_libraries(
    pangolin_viewer_lib
    PUBLIC
    ${Pangolin_LIBRARIES} 
    ${OpenCV_LIBS})
target_include_directories(
    pangolin_viewer_lib 
    PUBLIC
    ${SOURCE_ROOT}
    ${EIGEN3_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${Pangolin_INCLUDE_DIRS})
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
set_target_properties(
    pangolin_viewer 
    PROPERTIES LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib)
