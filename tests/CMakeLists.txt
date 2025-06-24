# ====================================================================
#  为每个测试文件创建可执行程序
# ====================================================================

# 查找所有测试源文件
file(GLOB TEST_FILES "*.cpp")

# 遍历所有找到的测试文件
foreach(TEST_FILE ${TEST_FILES})
  # 从完整路径中获取不带扩展名的文件名 (例如：test_string_utils)
  string(REPLACE ".cpp" "" EXECUTABLE_NAME ${TEST_FILE})

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