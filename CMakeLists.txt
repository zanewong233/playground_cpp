cmake_minimum_required(VERSION 3.20)

project(Playground LANGUAGES CXX)

# ────────────── 通用编译选项 ──────────────
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS  OFF)

# 把目标文件全部丢进 build/bin|lib，保持源码树干净
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ────────────── 子目录 ──────────────
add_subdirectory(src)

# 可选：实验/原型代码
option(BUILD_EXPERIMENTS "Build prototype & benchmark code in experiments/" ON)
if (BUILD_EXPERIMENTS)
    add_subdirectory(experiments)
endif()

# 沙盒可执行程序
add_executable(sandbox ${CMAKE_SOURCE_DIR}/sandbox.cpp)
target_link_libraries(sandbox 
    PRIVATE 
        playground_utils
        $<$<BOOL:${BUILD_EXPERIMENTS}>:concurrent_algorithms>
)

# ────────────── 单元测试 ──────────────
enable_testing()

include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
    GIT_TAG        release-1.12.1
)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googletest)

# ────────────── Google Benchmark 集成 ──────────────
FetchContent_Declare(
	googlebenchmark
	GIT_REPOSITORY https://github.com/google/benchmark.git
	GIT_TAG		   v1.9.4
	GIT_SHALLOW	   TRUE
)
# 禁用 benchmark 自带的 GTest 测试，以免与现有单元测试冲突
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
# 不编译 googletest 测试
set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
# 不启用 CTest 测试 (也可以屏蔽 examples，根据版本有时会连带屏蔽)
set(BENCHMARK_ENABLE_TESTING    OFF CACHE BOOL "" FORCE)
FetchContent_MakeAvailable(googlebenchmark)

add_subdirectory(tests)