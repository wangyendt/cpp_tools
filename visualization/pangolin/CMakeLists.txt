cmake_minimum_required(VERSION 3.12)
project(PangolinViewerPy)

set(CMAKE_POSITION_INDEPENDENT_CODE ON)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/pybind11")
# list(APPEND CMAKE_PREFIX_PATH "/usr/local/include")

find_package(pybind11 REQUIRED)
find_package(OpenCV REQUIRED)
find_package(Eigen3 REQUIRED)
find_package(Pangolin REQUIRED)

set(CMAKE_CXX_STANDARD 14)

add_library(
    pangolin_viewer_lib
    SHARED
    PangolinViewer.cpp PangolinViewer.h)
target_link_libraries(
    pangolin_viewer_lib
    PUBLIC
    ${Pangolin_LIBRARIES} 
    ${OpenCV_LIBS})
target_include_directories(
    pangolin_viewer_lib 
    PUBLIC
    ${EIGEN3_INCLUDE_DIR}
    ${OpenCV_INCLUDE_DIRS}
    ${Pangolin_INCLUDE_DIRS}
    /usr/local/include)

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
