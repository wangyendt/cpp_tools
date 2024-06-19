cmake_minimum_required(VERSION 3.10)
project(adb_log)

set(CMAKE_CXX_STANDARD 14)
set(SOURCE_ROOT ${CMAKE_SOURCE_DIR}/src)

list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/include")
include_directories("/opt/homebrew/opt")
include_directories("/usr/local/include")

find_package(pybind11 REQUIRED)

pybind11_add_module(adb_logcat adb_logcat_pybind.cpp)
target_include_directories(
	adb_logcat
	PRIVATE
	${SOURCE_ROOT}
)
set_target_properties(
	adb_logcat
	PROPERTIES LIBRARY_OUTPUT_DIRECTORY
	${CMAKE_SOURCE_DIR}/lib)
