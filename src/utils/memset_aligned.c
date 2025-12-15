#include <telly.h>

#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>

#if defined(__AVX2__) || defined(__SSE2__)
#include <immintrin.h>
#endif

#if defined(__aarch64__)
#include <arm_neon.h>
#endif

// SSE2 requires 16-aligned blocks
// AVX2 required 32-aligned blocks
// NEON requires 16-aligned blocks

// This function should not be used for small blocks, it should be used for unknown-sized blocks and large blocks.
void memset_aligned(void *s, int c, size_t n) {
    // ((void * pointer) + value) is undefined behavior
  char *d = (char *) s;
  uint8_t fill_value = (uint8_t) c;

#if defined(__AVX2__)
  if (n < 64) { // Setup cost of AVX2 is much larger than pure memcpy() for (n < 64)
    for (size_t i = 0; i < n; ++i) {
      d[i] = fill_value;
    }
  } else {
    size_t off = 0;
    __m256i vector = _mm256_set1_epi8(fill_value);

    for (; off + 32 <= n; off += 32) {
      _mm256_store_si256((__m256i *) (d + off), vector);
    }

    if (off < n) {
      for (size_t i = off; i < n; ++i) {
        d[i] = fill_value;
      }
    }
  }
#elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  if (n < 32) {  // Setup cost of SSE2 is much larger than pure memset() for (n < 32)
    for (size_t i = 0; i < n; ++i) {
      d[i] = fill_value;
    }
  } else {
    size_t off = 0;
    __m128i vector = _mm_set1_epi8(fill_value);

    for (; off + 16 <= n; off += 16) {
      _mm_store_si128((__m128i *) (d + off), vector);
    }

    if (off < n) {
      for (size_t i = off; i < n; ++i) {
        d[i] = fill_value;
      }
    }
  }
#elif defined(__aarch64__)
  if (n < 32) { // Setup cost of ARM NEON is much larger than pure memset() for (n < 32)
    for (size_t i = 0; i < n; ++i) {
      d[i] = fill_value;
    }
  } else {
    size_t off = 0;

    uint8x16_t vector = vdupq_n_u8(fill_value);

    for (; off + 16 <= n; off += 16) {
      vst1q_u8((uint8_t *) d + off, vector);
    }

      // Kalan kısmı doldurma
    if (off < n) {
      for (size_t i = off; i < n; ++i) {
        d[i] = fill_value;
      }
    }
  }
#else
  for (size_t i = 0; i < n; ++i) {
    d[i] = fill_value;
  }
#endif
}
