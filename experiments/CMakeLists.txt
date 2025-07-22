# experiments/CMakeLists.txt
# 只负责把 experiments 里的每个一级子目录递归加进来
file(GLOB CHILD_DIRS RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} *)
foreach(dir ${CHILD_DIRS})
    if(IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
        add_subdirectory(${dir})
    endif()
endforeach()
