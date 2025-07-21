# ----------------------------------------------------------------------------------
# Header-only 库：只告诉 CMake “有这么一套头文件”，无需编译源文件。
# ----------------------------------------------------------------------------------
add_library(concurrent_algorithms INTERFACE)

# 公开头文件搜索路径
target_include_directories(concurrent_algorithms INTERFACE
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>        # 若以后要 install
)

# C++17
target_compile_features(concurrent_algorithms INTERFACE cxx_std_17)

target_link_libraries(concurrent_algorithms INTERFACE playground_utils)

add_executable(concurrent_playground main.cpp)

target_link_libraries(concurrent_playground
    PRIVATE
        concurrent_algorithms   # 头文件库
        playground_utils        # 若 main 里要用你的核心工具
)

# 可选：让 `ctest -R concurrent` 直接运行这个可执行文件
# enable_testing() 已在根 CMakeLists 打开
add_test(NAME concurrent_playground COMMAND concurrent_playground)
