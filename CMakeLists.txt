cmake_minimum_required(VERSION 3.20)

project(Playground LANGUAGES CXX)

# ────────────── 通用编译选项 ──────────────
set(CMAKE_CXX_STANDARD 17)
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

add_subdirectory(tests)        # 维持现有测试逻辑   :contentReference[oaicite:2]{index=2}
