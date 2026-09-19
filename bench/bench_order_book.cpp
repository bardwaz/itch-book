#include <benchmark/benchmark.h>

static void BM_Dummy(benchmark::State& state) {
    for (auto _ : state) {
    }
}
BENCHMARK(BM_Dummy);
