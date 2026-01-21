# ============================================================================
# OptimizationFlags.cmake
# ============================================================================
# 跨平台编译优化配置
# 使用方法：在 CMakeLists.txt 中添加
#   include(${CMAKE_CURRENT_SOURCE_DIR}/../cmake/OptimizationFlags.cmake)
# ============================================================================

# 设置编译类型为 Release（启用优化）
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()
message(STATUS "Build type: ${CMAKE_BUILD_TYPE}")

# 跨平台编译优化选项
if(MSVC)
    # Windows MSVC
    set(OPTIMIZATION_FLAGS /O2 /DNDEBUG)
    set(OPTIMIZATION_LINK_FLAGS /O2)
else()
    # GCC/Clang (Linux/macOS)
    set(OPTIMIZATION_FLAGS -O3 -DNDEBUG -march=native -ffast-math)
    set(OPTIMIZATION_LINK_FLAGS -O3 -flto)
endif()

message(STATUS "Optimization flags: ${OPTIMIZATION_FLAGS}")
