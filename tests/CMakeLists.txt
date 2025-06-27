# ====================================================================
#  为每个测试文件创建可执行程序
# ====================================================================

# 查找所有测试源文件
file(GLOB TEST_FILES "*.cpp")

# 遍历所有找到的测试文件
foreach(TEST_FILE ${TEST_FILES})
  # 从完整的文件路径中，只提取出文件名，并且不包含扩展名
  # NAME_WE 的意思就是 "Name Without Extension"
  get_filename_component(EXECUTABLE_NAME ${TEST_FILE} NAME_WE)

  # 为每个测试文件创建一个单独的可执行程序
  add_executable(${EXECUTABLE_NAME} ${TEST_FILE})

  # ====================================================================
  #  链接依赖
  # ====================================================================

  # 链接我们的工具库。这是被测试的对象。
  target_link_libraries(${EXECUTABLE_NAME} PRIVATE playground_utils)

  # 链接GoogleTest库。gtest_main包含了main函数和测试运行环境
  target_link_libraries(${EXECUTABLE_NAME} PRIVATE gtest_main)

  # ====================================================================
  #  将测试添加到CTest
  # ====================================================================
  # 这允许你通过 "ctest" 命令来批量运行所有测试
  include(GoogleTest)
  gtest_add_tests(TARGET ${EXECUTABLE_NAME})

endforeach()