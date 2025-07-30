#include <benchmark/benchmark.h>
#include <list>
#include <random>

#include "concurrent/sort.hpp"

using playground::experiments::parallel::SorterThreadPool;

// 生成 n 个随机整数的 list
static std::list<int> generate_list(size_t n) {
    std::mt19937 rng(42);
    std::uniform_int_distribution<int> dist(0, 1'000'000);
    std::list<int> v;
    for (size_t i = 0; i < n; ++i) {
        v.push_back(dist(rng));
    }
    return v;
}

static void BM_SorterDoSort(benchmark::State& state) {
    size_t n = state.range(0);
    auto base = generate_list(n);
    SorterThreadPool<int> pool;
    for (auto _ : state) {
        // 每次都拷贝一份未排序数据
        auto data = base;
        auto result = pool.DoSort(std::move(data));
        benchmark::DoNotOptimize(result);
        benchmark::ClobberMemory();
    }
    state.SetComplexityN(n);
}

BENCHMARK(BM_SorterDoSort)
    ->Arg(1'000)
    ->Arg(10'000)
    ->Arg(30'000)
    ->Complexity();

BENCHMARK_MAIN();
