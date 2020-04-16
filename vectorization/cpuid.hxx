#pragma once

#include <stdint.h>
#if defined(_MSC_VER)
# include <intrin.h>
#endif

namespace {
  
inline void run_cpuid(std::uint32_t eax, std::uint32_t ecx, std::uint32_t* abcd) {
#if defined(_MSC_VER)
  int abcd_[4];
  __cpuidex(abcd_, eax, ecx);
  for (int i = 0; i < 4; ++i)
    abcd[i] = abcd_[i];
#else
  std::uint32_t ebx = 0;
  std::uint32_t edx = 0;
# if defined(__i386__) && defined (__PIC__)
  // in case of PIC under 32-bit EBX cannot be clobbered */
  __asm__ ("movl %%ebx, %%edi \n\t cpuid \n\t xchgl %%ebx, %%edi" : "=D" (ebx),
# else
  __asm__ ("cpuid" : "+b" (ebx),
# endif
           "+a" (eax), "+c" (ecx), "=d" (edx));
  abcd[0] = eax;
  abcd[1] = ebx;
  abcd[2] = ecx;
  abcd[3] = edx;
#endif
}

inline bool has_avx512() {
  std::uint32_t constexpr avx512_f = 1 << 16;
  std::uint32_t constexpr avx512_bw = 1 << 30;

  std::uint32_t abcd[4];

  run_cpuid(7, 0, abcd);

  std::uint32_t need = avx512_f | avx512_bw;
  return (abcd[1] & need) == need;
}

}
