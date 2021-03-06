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

#if defined (_MSC_VER)
# define assume_aligned(ptr, x) __assume(((char*)ptr - (char*)0) % x == 0);
#else
# define assume_aligned(ptr, x) ptr = reinterpret_cast<decltype(ptr)>(__builtin_assume_aligned(ptr, x));
#endif

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target ("tune=native")
#endif
static void AutoVec(benchmark::State& state) {
  auto const ncells = rows * cols;

  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVec);

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target ("tune=native")
#endif
static void AutoVecAligned16(benchmark::State& state) {
  auto const ncells = rows * cols;

  auto  pmat = aligned_unique_ptr(rows * cols, std::align_val_t{16});
  auto* mat  = pmat.get();
  assume_aligned(mat, 16);
  
  {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) = 0;
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) += 42;
    }
    
    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVecAligned16);

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target ("tune=native")
#endif
static void AutoVecAligned32(benchmark::State& state) {
  auto const ncells = rows * cols;

  auto  pmat = aligned_unique_ptr(rows * cols, std::align_val_t{32});
  auto* mat  = pmat.get();
  assume_aligned(mat, 32);
  
  {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) = 0;
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) += 42;
    }
    
    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVecAligned32);

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2,tree-vectorize")
# pragma GCC target("tune=native")
#endif
static void AutoVecAligned64(benchmark::State& state) {
  auto const ncells = rows * cols;

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{64});
  auto* mat = pmat.get();
  assume_aligned(mat, 64);
  
  {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) = 0;
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) += 42;
    }
    
    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(AutoVecAligned64);

#if !defined(__clang__)
#if defined(__GNUC__) && !defined(__clang__)
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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX512);

#if defined(__GNUC__) && !defined(__clang__)
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
  assume_aligned(mat, 64);

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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX512Aligned);
#endif  // !defined(__clang__)

#if defined(__GNUC__) && !defined(__clang__)
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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX2);

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,avx,avx2,tune=skylake")
#endif
static void ManualVecAVX2Aligned(benchmark::State& state) {
  __m256i zeros = _mm256_setzero_si256();
  __m256i const42 = _mm256_set1_epi8(42);

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{32});
  auto* mat = pmat.get();
  assume_aligned(mat, 32);

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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecAVX2Aligned);

#if defined(__GNUC__) && !defined(__clang__)
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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecSSE);

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
# pragma GCC target ("sse,sse2,sse3,ssse3,sse4,popcnt,abm,mmx,tune=skylake")
#endif
static void ManualVecSSEAligned(benchmark::State& state) {
  __m128i zeros = _mm_setzero_si128();
  __m128i const42 = _mm_set1_epi8(42);

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{16});
  auto* mat = pmat.get();
  assume_aligned(mat, 16);

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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecSSEAligned);

#if !defined(_MSC_VER)
#if defined(__GNUC__) && !defined(__clang__)
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
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(ManualVecMMX);
#endif

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
#endif
static void NoVec(benchmark::State& state) {
  auto const ncells = rows * cols;

  auto  pmat = std::make_unique<std::uint8_t[]>(rows * cols);
  auto* mat = pmat.get();

  {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) = 0;
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) += 42;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(NoVec);

#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC push_options
# pragma GCC optimize ("O2")
#endif
static void NoVecAligned32(benchmark::State& state) {
  auto const ncells = rows * cols;

  auto pmat = aligned_unique_ptr(rows * cols, std::align_val_t{32});
  auto* mat = pmat.get();
  assume_aligned(mat, 32);

  {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) = 0;
    }
  }

  for (auto _ : state) {
    auto* ptr = mat;
    for (int i = 0; i < ncells; ++i) {
      *(ptr++) += 42;
    }

    benchmark::DoNotOptimize(mat);
    benchmark::ClobberMemory();
  }
}
#if defined(__GNUC__) && !defined(__clang__)
# pragma GCC pop_options
#endif
BENCHMARK(NoVecAligned32);

BENCHMARK_MAIN();
