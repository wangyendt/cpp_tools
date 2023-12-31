cmake_minimum_required(VERSION 3.12)

project(dsp_sliding_window)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_ROOT ${CMAKE_SOURCE_DIR}/src)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt/pybind11")

find_package(pybind11 REQUIRED)

add_library(
	dsp_sliding_window
	STATIC
    src/MKAverage.cpp
	src/MonoQueue.cpp
	src/WelfordStd.cpp
	)
target_include_directories(
	dsp_sliding_window
	PUBLIC
	${CMAKE_SOURCE_DIR}/src
)

pybind11_add_module(dsp_sw dsp_sw_pybind.cpp)
target_link_libraries(
	dsp_sw
	PRIVATE
	dsp_sliding_window)
set_target_properties(
	dsp_sw
	PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	${CMAKE_SOURCE_DIR}/lib)
	