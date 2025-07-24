# experiments/concurrent/tests/CMakeLists.txt
set(ALL_TEST_SOURCES
	test_for_each.cpp
	test_find.cpp
    test_partical_sum.cpp
)

add_executable(test_experiment ${ALL_TEST_SOURCES})

target_link_libraries(test_experiment
    PRIVATE
        concurrent_algorithms
        GTest::gtest_main)

add_test(NAME test_experiment COMMAND test_experiment)
