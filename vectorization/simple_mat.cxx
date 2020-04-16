#include <memory>

#include <benchmark/benchmark.h>
#include <emmintrin.h>
#include <immintrin.h>
#include <mmintrin.h>

#include "cpuid.hxx"

// Good clear numbers:
// http://quick-bench.com/fve5Wt5DvuB8PQRZ7JT-xAG2_H0

auto  rows = 1080;
auto  cols = 1280;

struct aligned_deleter {
  std::align_val_t align;
  void operator()(std::uint8_t* p) const { operator delete[](p, align); }
};

auto aligned_unique_ptr(std::size_t size, std::align_val_t align) {
  auto* p = operator new[](sizeof(std::uint8_t) * size, align);
  return std::unique_ptr<std::uint8_t, aligned_deleter>(
      reinterpret_cast<std::uint8_t*>(p), aligned_deleter{align});
}

#if defined(__clang__)
# undef __GNUC__
#endif

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target ("tune=native")
#endif
static void AutoVec(benchmark::State& state) {
  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  auto const ncells = rows * cols;

  {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) = 0;
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }
    
    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVec)->MeasureProcessCPUTime()->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target ("tune=native")
#endif
static void AutoVecAligned32(benchmark::State& state) {
  auto const nrows = rows;
  auto const ncols = cols;

  auto  pmat = aligned_unique_ptr(rows * cols, std::align_val_t{32});
  auto* mat  = pmat.get();
#if defined (_MSC_VER)
  __assume(((char*)pmat.get() - (char*)0) % 32 == 0);
#else
  mat = reinterpret_cast<decltype(pmat.get())>(__builtin_assume_aligned(pmat.get(), 32));
#endif
  
  {
    auto* ptr = mat;
    for (int r = 0; r < nrows; ++r) {
      for (int c = 0; c < ncols; ++c) {
        *(ptr++) = 0;
      }
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int r = 0; r < nrows; ++r) {
      for (int c = 0; c < ncols; ++c) {
        *ptr += 42;
        ++ptr;
      }
    }
    
    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVecAligned32)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target("tune=native")
#endif
static void AutoVecAligned64(benchmark::State& state) {
  auto const nrows = rows;
  auto const ncols = cols;

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{64});
  auto* mat = pmat.get();
#if defined(_MSC_VER)
  __assume(((char*)pmat.get() - (char*)0) % 64 == 0);
#else
  mat = reinterpret_cast<decltype(pmat.get())>(__builtin_assume_aligned(pmat.get(), 64));
#endif
  
  {
    auto* ptr = mat;
    for (int r = 0; r < nrows; ++r) {
      for (int c = 0; c < ncols; ++c) {
        *(ptr++) = 0;
      }
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int r = 0; r < nrows; ++r) {
      for (int c = 0; c < ncols; ++c) {
        *ptr += 42;
        ++ptr;
      }
    }
    
    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVecAligned64)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if !defined(__clang__)
#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,avx512f,avx512bw,tune=skylake-avx512")
#endif
static void ManualVecAVX512(benchmark::State& state) {
  if (!has_avx512()) {
    state.SkipWithError("No AVX512 support");
    for (auto _ : state) {}
    return;
  }

  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  __m512i zeros = _mm512_setzero_si512();
  __m512i const42 = _mm512_set1_epi8(42);

  constexpr int nlanes = 64;
  auto const ncells = cols * rows;

  {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      _mm512_storeu_si512((__m512i*)ptr, zeros);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr = 0;
      ++ptr;
    }
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _mm512_loadu_si512((const __m512i*)ptr);
      vec = _mm512_add_epi8(vec, const42);
      _mm512_storeu_si512((__m512i*)ptr, vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX512)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,avx512f,avx512bw,tune=skylake-avx512")
#endif
static void ManualVecAVX512Aligned(benchmark::State& state) {
  if (!has_avx512()) {
    state.SkipWithError("No AVX512 support");
    for (auto _ : state) {}
    return;
  }

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{64});
  auto* mat = pmat.get();

  __m512i zeros = _mm512_setzero_si512();
  __m512i const42 = _mm512_set1_epi8(42);

  constexpr int nlanes = 64;
  auto const ncells = cols * rows;

  {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      _mm512_store_si512((__m512i*)ptr, zeros);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr = 0;
      ++ptr;
    }
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _mm512_load_si512((const __m512i*)ptr);
      vec = _mm512_add_epi8(vec, const42);
      _mm512_store_si512((__m512i*)ptr, vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX512Aligned)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);
#endif  // !defined(__clang__)

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,avx2,tune=skylake")
#endif
static void ManualVecAVX2(benchmark::State& state) {
  __m256i zeros = _mm256_setzero_si256();
  __m256i const42 = _mm256_set1_epi8(42);

  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  constexpr int nlanes = 32;
  auto const ncells = cols * rows;

  {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      _mm256_storeu_si256((__m256i*)ptr, zeros);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr = 0;
      ++ptr;
    }
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _mm256_loadu_si256((const __m256i*)ptr);
      vec = _mm256_add_epi8(vec, const42);
      _mm256_storeu_si256((__m256i*)ptr, vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX2)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,avx2,tune=skylake")
#endif
static void ManualVecAVX2Aligned(benchmark::State& state) {
  __m256i zeros = _mm256_setzero_si256();
  __m256i const42 = _mm256_set1_epi8(42);

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{32});
  auto* mat = pmat.get();

  constexpr int nlanes = 32;
  auto const ncells = cols * rows;

  {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      _mm256_store_si256((__m256i*)ptr, zeros);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr = 0;
      ++ptr;
    }
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _mm256_load_si256((const __m256i*)ptr);
      vec = _mm256_add_epi8(vec, const42);
      _mm256_store_si256((__m256i*)ptr, vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX2Aligned)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,tune=skylake")
#endif
static void ManualVecSSE(benchmark::State& state) {
  __m128i zeros = _mm_setzero_si128();
  __m128i const42 = _mm_set1_epi8(42);

  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  constexpr int nlanes = 16;
  auto const ncells = cols * rows;

  {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      _mm_storeu_si128((__m128i*)ptr, zeros);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr = 0;
      ++ptr;
    }
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _mm_loadu_si128((const __m128i*)ptr);
      vec = _mm_add_epi8(vec, const42);
      _mm_storeu_si128((__m128i*)ptr, vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecSSE)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,tune=skylake")
#endif
static void ManualVecSSEAligned(benchmark::State& state) {
  __m128i zeros = _mm_setzero_si128();
  __m128i const42 = _mm_set1_epi8(42);

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{16});
  auto* mat = pmat.get();

  constexpr int nlanes = 16;
  auto const ncells = cols * rows;

  {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      _mm_store_si128((__m128i*)ptr, zeros);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr = 0;
      ++ptr;
    }
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _mm_load_si128((const __m128i*)ptr);
      vec = _mm_add_epi8(vec, const42);
      _mm_store_si128((__m128i*)ptr, vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecSSEAligned)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if !defined(_MSC_VER)
#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("mmx,tune=skylake")
#endif
static void ManualVecMMX(benchmark::State& state) {
  __m64 zeros = _mm_setzero_si64();
  __m64 const42 = _mm_set1_pi8(42);

  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  constexpr int nlanes = 8;
  auto const ncells = cols * rows;

  int i = 0;
  auto* ptr = mat;
  for (; i <= ncells - nlanes; i += nlanes) {
    *reinterpret_cast<std::uint64_t*>(ptr) = _m_to_int64(zeros);
    ptr += nlanes;
  }
  for (; i < ncells; ++i) {
    *ptr = 0;
    ++ptr;
  }

  for (auto _ : state) {
    int i = 0;
    auto* ptr = mat;
    for (; i <= ncells - nlanes; i += nlanes) {
      auto vec = _m_from_int64(*(const std::uint64_t*)ptr);
      vec = _mm_add_pi8(vec, const42);
      *reinterpret_cast<std::uint64_t*>(ptr) = _m_to_int64(vec);
      ptr += nlanes;
    }
    for (; i < ncells; ++i) {
      *ptr += 42;
      ++ptr;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }

  _mm_empty();
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecMMX)->MeasureProcessCPUTime()->Unit(benchmark::kNanosecond);
#endif

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
#endif
static void NoVec(benchmark::State& state) {
  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  {
    auto* ptr = mat;
    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        *(ptr++) = 0;
      }
    }
  }

  auto const nrows = rows;
  auto const ncols = cols;
  for (auto _ : state) {
    auto* ptr = mat;
    for (int r = 0; r < nrows; ++r) {
      for (int c = 0; c < ncols; ++c) {
        *ptr += 42;
        ++ptr;
      }
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(NoVec)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

#if defined(__GNUC__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
#endif
static void NoVecAligned32(benchmark::State& state) {
  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{32});
  auto* mat = pmat.get();

  {
    auto* ptr = mat;
    for (int r = 0; r < rows; ++r) {
      for (int c = 0; c < cols; ++c) {
        *(ptr++) = 0;
      }
    }
  }

  auto const nrows = rows;
  auto const ncols = cols;
  for (auto _ : state) {
    auto* ptr = mat;
    for (int r = 0; r < nrows; ++r) {
      for (int c = 0; c < ncols; ++c) {
        *ptr += 42;
        ++ptr;
      }
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__)
# pragma GCC pop_options
#endif
BENCHMARK(NoVecAligned32)
    ->MeasureProcessCPUTime()
    ->Unit(benchmark::kNanosecond);

BENCHMARK_MAIN();
