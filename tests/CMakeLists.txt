# tests/CMakeLists.txt (合并后的版本)

# 1. 查找所有以 "test_" 开头的 cpp 文件，将它们全部收集到一个变量里
set(ALL_TEST_SOURCES
	test_print_class.cpp
	test_thread_scope.cpp
    test_joining_thread.cpp
	test_threadsafe_queue.cpp
    test_threadsafe_lookup_table.cpp
    test_threadsafe_list.cpp
	test_lockfree_stack.cpp
)

# 2. 只创建一个可执行程序目标，名字叫 run_all_tests
#    这个目标由所有找到的测试源文件共同构成
add_executable(run_all_tests ${ALL_TEST_SOURCES})

# 3. 为这个总的测试目标链接所需要的库
#    所有测试都需要 GTest 和我们自己的库
target_link_libraries(run_all_tests 
    PRIVATE 
    gtest_main
    playground_utils
)

# 4. 将这个目标中的所有测试添加到 CTest
include(GoogleTest)
gtest_add_tests(TARGET run_all_tests)