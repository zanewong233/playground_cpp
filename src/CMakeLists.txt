# ====================================================================
#  创建工具库 (playground_utils)
# ====================================================================

# 查找当前目录下所有的 .cpp 文件，并将它们存入 UTILS_SOURCES 变量
# glob是简单方便的方式，但如果新增/删除文件，需要重新运行CMake来发现变更
file(GLOB_RECURSE UTILS_SOURCES CONFIGURE_DEPENDS "*.cpp")

# 创建一个名为 "playground_utils" 的库
add_library(playground_utils STATIC ${UTILS_SOURCES})

# 这行代码会给 playground_utils 目标添加一个编译定义。
# PRIVATE 表示这个定义只在编译本目标时有效，不会传递给链接它的其他目标。
target_compile_definitions(playground_utils PRIVATE PLAYGROUND_UTILS_EXPORTS)

# ====================================================================
#  配置头文件路径
# ====================================================================

# 这是非常关键的一步，它定义了库的公共接口
target_include_directories(playground_utils
  PUBLIC
    # PUBLIC: 意味着任何链接到 playground_utils 的目标，
    # 都会自动继承这个头文件路径。
    # ${CMAKE_SOURCE_DIR} 指的是项目根目录。
    ${CMAKE_SOURCE_DIR}/include
)

# 使用 target_link_libraries 可以传递依赖关系。
# 如果你的工具有其他依赖（比如链接了某个第三方库），可以在这里添加。
# target_link_libraries(playground_utils PUBLIC some_other_lib)