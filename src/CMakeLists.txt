# src/CMakeLists.txt

# 创建一个 INTERFACE 库，专门用于管理头文件
add_library(playground_headers INTERFACE)

# 告诉这个库，它的公共头文件目录在哪里
target_include_directories(playground_headers
  INTERFACE
    ${CMAKE_SOURCE_DIR}/include
)

# --- 明确列出 playground_utils 库的所有源文件 ---
# 路径是相对于当前 CMakeLists.txt 文件（即 src/ 目录）的
set(UTILS_SOURCES
	cpp17/cpp17_test.cpp
	threading/async_log.cpp
	threading/thread_pool.cpp
	threading/thread_scope.cpp
	print_class/print_class.cpp
)

# 使用这个明确的列表来创建库
add_library(playground_utils STATIC ${UTILS_SOURCES})

# 让 playground_utils “链接”到 playground_headers
# 这样它就能自动找到所需的头文件了
target_link_libraries(playground_utils PUBLIC playground_headers)