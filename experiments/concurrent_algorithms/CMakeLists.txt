# experiments/concurrent/CMakeLists.txt
# ------------------------------------
# 1. header-only 库
include_directories(${CMAKE_SOURCE_DIR}/include)
add_library(concurrent_algorithms INTERFACE)
target_include_directories(concurrent_algorithms INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)


# 2. 单元测试
add_subdirectory(tests)

# 2. 单元测试
add_subdirectory(benchmark)