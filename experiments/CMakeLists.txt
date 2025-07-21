# 这个文件只做一件事：递归地把 experiments/ 下的每个一级子目录
# 当成一个“实验模块”加进来。每个子目录再各自写自己的 CMakeLists.txt，
# 以便独立管理依赖 & 编译选项。

file(GLOB CHILD_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(dir ${CHILD_DIRS})
    if(IS_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${dir})
        add_subdirectory(${dir})
    endif()
endforeach()
