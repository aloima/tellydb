#include <telly.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#include <immintrin.h>

// SSE2 requires 16-aligned blocks
// AVX2 required 32-aligned blocks
// This function is not used for small blocks, it used for unknown-sized blocks and large blocks.
void memcpy_aligned(void *restrict dst, const void *restrict src, size_t n) {
#if defined(__AVX2__)
    if (n < 64) { // Setup cost of AVX2 is much larger than pure memcpy() for (n < 64)
      memcpy(dst, src, n);
    } else {
      uint32_t off = 0;

      for (; off + 32 <= n; off += 32) {
        __m256i value = _mm256_loadu_si256((const __m256i *) (src + off));
        _mm256_storeu_si256((__m256i *) (dst + off), value);
      }

      if (off < n) {
        memcpy(dst + off, src + off, n - off);
      }
    }
#elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
    if (n < 32) { // Setup cost of SSE2 is much larger than pure memcpy() for (n < 32)
      memcpy(dst, src, n);
    } else {
      uint32_t off = 0;

      for (; off + 16 <= n; off += 16) {
        __m128i value = _mm_loadu_si128((const __m128i *) (src + off));
        _mm_storeu_si128((__m128i *) (dst + off), value);
      }

      if (off < n) {
        memcpy(dst + off, src + off, n - off);
      }
    }

#else
    memcpy(dst, src, n);
#endif
}
