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


list(APPEND CMAKE_PREFIX_PATH "/opt/homebrew/opt")
list(APPEND CMAKE_PREFIX_PATH "/usr/local/include")
include_directories("/opt/homebrew/opt")
include_directories("/usr/local/include")

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
set_target_properties(apriltag_detection PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
