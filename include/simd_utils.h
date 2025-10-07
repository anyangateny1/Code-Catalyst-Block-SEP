#ifndef SIMD_UTILS_H
#define SIMD_UTILS_H

#include <cstddef>
#include <cstdint>

#if defined(__SSE2__)
  #include <emmintrin.h>
  #define SEP_HAVE_SSE2 1
#else
  #define SEP_HAVE_SSE2 0
#endif

// Compare a byte row [ptr, ptr+len) to a constant byte 'val'. Returns true if all equal
inline bool row_all_equal(const char* ptr, std::size_t len, char val) {
#if SEP_HAVE_SSE2
    const std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
    std::size_t i = 0;
    // Align to 16 bytes
    std::size_t pre = (16 - (addr & 15)) & 15;
    if (pre > len) pre = len;
    for (; i < pre; ++i) if (ptr[i] != val) return false;
    __m128i v = _mm_set1_epi8(val);
    std::size_t simdLen = (len - i) / 16;
    const __m128i* p = reinterpret_cast<const __m128i*>(ptr + i);
    for (std::size_t k = 0; k < simdLen; ++k) {
        __m128i x = _mm_loadu_si128(p + k);
        __m128i cmp = _mm_cmpeq_epi8(x, v);
        if (_mm_movemask_epi8(cmp) != 0xFFFF) return false;
    }
    i += simdLen * 16;
    for (; i < len; ++i) if (ptr[i] != val) return false;
    return true;
#else
    for (std::size_t i = 0; i < len; ++i) if (ptr[i] != val) return false;
    return true;
#endif
}

// Check a byte row [ptr, ptr+len) is all zero. Returns true if all zero
inline bool row_all_zero(const char* ptr, std::size_t len) {
#if SEP_HAVE_SSE2
    const std::uintptr_t addr = reinterpret_cast<std::uintptr_t>(ptr);
    std::size_t i = 0;
    std::size_t pre = (16 - (addr & 15)) & 15;
    if (pre > len) pre = len;
    for (; i < pre; ++i) if (ptr[i] != 0) return false;
    __m128i zero = _mm_setzero_si128();
    std::size_t simdLen = (len - i) / 16;
    const __m128i* p = reinterpret_cast<const __m128i*>(ptr + i);
    for (std::size_t k = 0; k < simdLen; ++k) {
        __m128i x = _mm_loadu_si128(p + k);
        __m128i cmp = _mm_cmpeq_epi8(x, zero);
        if (_mm_movemask_epi8(cmp) != 0xFFFF) return false;
    }
    i += simdLen * 16;
    for (; i < len; ++i) if (ptr[i] != 0) return false;
    return true;
#else
    for (std::size_t i = 0; i < len; ++i) if (ptr[i] != 0) return false;
    return true;
#endif
}

#endif // SIMD_UTILS_H


