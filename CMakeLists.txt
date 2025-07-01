cmake_minimum_required(VERSION 3.14)

# 定义项目名称和C++标准
project(Playground LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# 设置构建输出目录，避免在源码树中产生编译文件
# 这样所有 .exe, .lib, .dll, .a, .so 文件都会在 build/bin, build/lib 等目录下
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# ====================================================================
#  子目录处理
# ====================================================================

# 将源码目录添加进来，这会执行 src/CMakeLists.txt 文件
# 它会定义一个名为 "playground_utils" 的库
add_subdirectory(src)

# -----------------------------------------------------------------
# Playground 随机实验可执行文件
# -----------------------------------------------------------------
add_executable(sandbox
  ${CMAKE_SOURCE_DIR}/sandbox.cpp
)
# 如果想用 playground_utils 库，就 link 上：
target_link_libraries(sandbox PRIVATE playground_utils)

# ====================================================================
#  测试配置
# ====================================================================

# 开启测试功能，这样CTest才能工作
enable_testing()

# 包含FetchContent模块，用于在构建时自动下载依赖（如GoogleTest）
include(FetchContent)

# 声明GoogleTest依赖
FetchContent_Declare(
  googletest
  GIT_REPOSITORY https://github.com/google/googletest.git
  GIT_TAG        release-1.12.1 # 建议锁定到一个具体的稳定版本
)

# 新增下面这行代码来强制gtest使用动态运行时库(MD/MDd)
set(gtest_force_shared_crt ON CACHE BOOL "" FORCE)

# 让FetchContent真正地去获取GoogleTest源码
# QUIET 参数可以减少不必要的输出信息
FetchContent_MakeAvailable(googletest)

# 将测试目录添加进来，这会执行 tests/CMakeLists.txt
# 它会使用上面下载的GoogleTest来构建测试用例
add_subdirectory(tests)