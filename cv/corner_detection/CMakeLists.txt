cmake_minimum_required(VERSION 3.10)

project(cv_corner_detection)

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

# add_library(
# 	cv_corner_detection
# 	STATIC
#     src/GridCalibrationTargetBase.cpp
# 	src/GridCalibrationTargetAprilgrid.cpp
# 	)
# target_link_libraries(
# 	cv_corner_detection
# 	ethz_apriltag2
# )
# target_include_directories(
# 	cv_corner_detection
# 	PUBLIC
# 	${CMAKE_CURRENT_SOURCE_DIR}/src
# )
# set_target_properties(
#     cv_corner_detection 
#     PROPERTIES
#     ARCHIVE_OUTPUT_DIRECTORY
#     ${CMAKE_CURRENT_SOURCE_DIR}/lib
#     LIBRARY_OUTPUT_DIRECTORY
#     ${CMAKE_CURRENT_SOURCE_DIR}/lib)

# add_executable(
# 	apriltag_board_corner_detection
# 	src/apriltag_board_corner_detection.cpp
# )
# target_link_libraries(
# 	apriltag_board_corner_detection
# 	cv_corner_detection
# 	ethz_apriltag2
# )

pybind11_add_module(corner_detection_py corner_detection_pybind.cpp)
target_link_libraries(corner_detection_py PRIVATE
	ethz_apriltag2
    ${OpenCV_LIBS}
    Eigen3::Eigen
)
target_include_directories(corner_detection_py PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/apriltags
    ${OpenCV_INCLUDE_DIRS}
    ${EIGEN3_INCLUDE_DIR}
)
set_target_properties(corner_detection_py PROPERTIES
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/lib
)
