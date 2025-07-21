# experiments/concurrent/tests/CMakeLists.txt
add_executable(for_each_test for_each_test.cpp)

target_link_libraries(for_each_test
    PRIVATE
        concurrent_algorithms      # 头文件库，来自同目录上层 CMakeLists
        GTest::gtest_main          # 根 CMakeLists 已 FetchContent GTest
)

# 把测试注册给 CTest
add_test(NAME for_each_test COMMAND for_each_test)
