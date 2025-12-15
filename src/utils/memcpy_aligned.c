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
void memcpy_aligned(void *restrict dst, const void *restrict src, size_t n) {
  // ((void * pointer) + value) is undefined behavior
  char *d = (char *) dst;
  const char *s = (const char *) src;

#if defined(__AVX2__)
  if (n < 64) { // Setup cost of AVX2 is much larger than pure memcpy() for (n < 64)
    memcpy(d, s, n);
  } else {
    size_t off = 0;

    for (; off + 32 <= n; off += 32) {
      __m256i value = _mm256_loadu_si256((const __m256i *) (s + off));
      _mm256_storeu_si256((__m256i *) (d + off), value);
    }

    if (off < n) {
      memcpy(d + off, s + off, n - off);
    }
  }
#elif defined(__SSE2__) || (defined(_M_IX86_FP) && _M_IX86_FP >= 2)
  if (n < 32) { // Setup cost of SSE2 is much larger than pure memcpy() for (n < 32)
    memcpy(d, s, n);
  } else {
    size_t off = 0;

    for (; off + 16 <= n; off += 16) {
      __m128i value = _mm_loadu_si128((const __m128i *) (s + off));
      _mm_storeu_si128((__m128i *) (d + off), value);
    }

    if (off < n) {
      memcpy(d + off, s + off, n - off);
    }
  }
#elif defined(__aarch64__)
  if (n < 32) { // Setup cost of ARM NEON is much larger than pure memcpy() for (n < 32)
    memcpy(d, s, n);
  } else {
    size_t off = 0;

    for (; off + 16 <= n; off += 16) {
      uint8x16_t value = vld1q_u8((const uint8_t *) s + off);
      vst1q_u8((uint8_t *) d + off, value);
    }

    if (off < n) {
      memcpy(d + off, s + off, n - off);
    }
  }
#else
  memcpy(d, s, n);
#endif
}
