cmake_minimum_required(VERSION 3.10)

project(cw_tools)

add_definitions(-std=c++17 -w)
set(CMAKE_CXX_FLAGS "-std=c++17")
set(CMAKE_CONFIGURATION_TYPES ${CMAKE_BUILD_TYPE} CACHE STRING "" FORCE)
set(SOURCE_ROOT ${CMAKE_CURRENT_SOURCE_DIR}/src)

option(BUILD_SHARED_LIBS "Build using shared libraries" OFF)
if(BUILD_SHARED_LIBS)
    set(LIBRARY_TYPE SHARED)
    set(DYAML_BUILD_SHARED_LIBS ON)
else()
    set(LIBRARY_TYPE STATIC)
    set(DYAML_BUILD_SHARED_LIBS OFF)
endif()


list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")

find_package(OpenCV 4 REQUIRED)
find_package(Eigen3 3 REQUIRED)


include(FetchContent)

FetchContent_Declare(
  yaml-cpp
  GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
  GIT_TAG 0.8.0 # Can be a tag (yaml-cpp-x.x.x), a commit hash, or a branch name (master)
)
FetchContent_GetProperties(yaml-cpp)

if(NOT yaml-cpp_POPULATED)
  message(STATUS "Fetching yaml-cpp...")
  FetchContent_Populate(yaml-cpp)
  add_subdirectory(${yaml-cpp_SOURCE_DIR} ${yaml-cpp_BINARY_DIR})
endif()

file(GLOB_RECURSE SRC_FILES "${SOURCE_ROOT}/*.cpp")
file(GLOB_RECURSE HEADER_FILES "${SOURCE_ROOT}/*.h")

add_library(cw_tools INTERFACE)
target_include_directories(
	cw_tools
	INTERFACE
	${SOURCE_ROOT}
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)
target_link_libraries(cw_tools INTERFACE yaml-cpp)
