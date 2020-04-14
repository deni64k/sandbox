#include <string>
#include <vector>

#include <benchmark/benchmark.h>

#include "astar.hxx"

static void FindPath_RightDown(benchmark::State& state) {
  std::size_t rows = state.range(0);
  std::size_t cols = state.range(1);
  auto map = Map(rows, cols);
  for (unsigned r = 0; r < map.rows; ++r) {
    for (unsigned c = 0; c < map.cols; ++c) {
      map[r][c].weight = 100;
    }
  }

  for (unsigned i = 0; i < map.cols; ++i) {
    map[0][i].weight = 0;
  }
  for (unsigned i = 0; i < map.rows; ++i) {
    map[i][map.cols - 1].weight = 0;
  }
  
  for (auto _ : state) {
    auto path = find_paths(map, Location{0, 0}, Location{map.rows-1, map.cols-1});
    benchmark::DoNotOptimize(&path);
    benchmark::ClobberMemory();
  }
}
BENCHMARK(FindPath_RightDown)
->Args({1000, 1000})
->Args({100, 100})
->Args({10, 10});

BENCHMARK_MAIN();
