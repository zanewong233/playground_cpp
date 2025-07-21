#include <gtest/gtest.h>
#include <numeric>
#include <vector>
#include "playground/concurrent/parallel/for_each.hpp"

TEST(ParallelForEach, Increment) {
    std::vector<int> v(1000);
    std::iota(v.begin(), v.end(), 0);
    playground::concurrent::parallel::ParallelForEach(
        v.begin(), v.end(), [](int& x) { ++x; });
    for (std::size_t i = 0; i < v.size(); ++i) {
        EXPECT_EQ(v[i], static_cast<int>(i) + 1);
    }
}

TEST(ParallelForEach, AtomicSum) {
    std::vector<int> v(1'000'000, 1);
    std::atomic<long long> sum{0};
    playground::concurrent::parallel::ParallelForEach(
        v.begin(), v.end(),
        [&sum](int x) { sum.fetch_add(x, std::memory_order_relaxed); });
    EXPECT_EQ(sum.load(), 1'000'000);
}
