cmake_minimum_required(VERSION 3.10)

project(dsp_sliding_window)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/include")
include_directories("/opt/homebrew/opt")
include_directories("/usr/local/include")

find_package(pybind11 REQUIRED)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
else()
    set(LIBRARY_TYPE STATIC)
endif()

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
	${CMAKE_CURRENT_SOURCE_DIR}/src
)
set_target_properties(
    dsp_sliding_window 
    PROPERTIES
    ARCHIVE_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib
    LIBRARY_OUTPUT_DIRECTORY
    ${CMAKE_CURRENT_SOURCE_DIR}/lib)

pybind11_add_module(dsp_sw dsp_sw_pybind.cpp)
target_link_libraries(
	dsp_sw
	PRIVATE
	dsp_sliding_window)
set_target_properties(
	dsp_sw
	PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	${CMAKE_CURRENT_SOURCE_DIR}/lib)
