#include <stdint.h>
#if defined(_MSC_VER)
# include <intrin.h>
#endif

namespace {
  
inline void run_cpuid(std::uint32_t eax, std::uint32_t ecx, std::uint32_t* abcd) {
#if defined(_MSC_VER)
  __cpuidex(abcd, eax, ecx);
#else
  [[maybe_unused]] std::uint32_t ebx = 0;
  [[maybe_unused]] std::uint32_t edx = 0;
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
  std::uint32_t abcd[4];
  // std::uint32_t osxsave_mask = (1 << 27); // OSX.
  std::uint32_t avx2_bmi12_mask = (1 << 16) | // AVX-512F
                                  (1 << 26) | // AVX-512PF
                                  (1 << 27) | // AVX-512ER
                                  (1 << 28);  // AVX-512CD

  run_cpuid(7, 0, abcd);

  return (abcd[1] & avx2_bmi12_mask) == avx2_bmi12_mask;
}

}
