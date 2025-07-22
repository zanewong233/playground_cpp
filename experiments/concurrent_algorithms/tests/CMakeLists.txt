# experiments/concurrent/tests/CMakeLists.txt
add_executable(test_experiment for_each_test.cpp)

target_link_libraries(test_experiment
    PRIVATE
        concurrent_algorithms
        GTest::gtest_main)

add_test(NAME test_experiment COMMAND test_experiment)
