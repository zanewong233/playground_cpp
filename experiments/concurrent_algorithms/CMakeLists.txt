# experiments/concurrent/CMakeLists.txt
# ------------------------------------
# 1. header-only 库
include_directories(${CMAKE_SOURCE_DIR}/include)
add_library(concurrent_algorithms INTERFACE)
target_include_directories(concurrent_algorithms INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>)

# 2. playground 可执行
option(BUILD_CONCURRENT_PLAYGROUND "Build experiments/concurrent/benchmark.cpp" OFF)
if (BUILD_CONCURRENT_PLAYGROUND AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/benchmark.cpp")
    add_executable(concurrent_playground benchmark.cpp)
    target_link_libraries(concurrent_playground PRIVATE concurrent_algorithms playground_utils)
    add_test(NAME concurrent_playground COMMAND concurrent_playground)
endif()

# 3. 单元测试
add_subdirectory(tests)
