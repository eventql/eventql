#include "simdcomputil.h"
#include <assert.h>

#define Delta(curr, prev) \
    _mm_sub_epi32(curr, \
            _mm_or_si128(_mm_slli_si128(curr, 4), _mm_srli_si128(prev, 12)))

/* returns the integer logarithm of v (bit width) */
uint32_t bits(const uint32_t v) {
#ifdef _MSC_VER
    unsigned long answer;
    if (v == 0) {
        return 0;
    }
    _BitScanReverse(&answer, v);
    return answer + 1;
#else
    return v == 0 ? 0 : 32 - __builtin_clz(v); /* assume GCC-like compiler if not microsoft */
#endif
}

SIMDCOMP_PURE uint32_t maxbits(const uint32_t * begin) {
    uint32_t accumulator = 0;
    const uint32_t * k = begin;
    for (; k != begin + SIMDBlockSize; ++k) {
        accumulator |= *k;
    }
    return bits(accumulator);
}

static uint32_t maxbitas32int(const __m128i accumulator) {
    uint32_t tmparray[4];
    _mm_storeu_si128((__m128i *) (tmparray), accumulator);
    return bits(tmparray[0] | tmparray[1] | tmparray[2] | tmparray[3]);
}


/* maxbit over 128 integers (SIMDBlockSize) with provided initial value */
uint32_t simdmaxbitsd1(uint32_t initvalue, const uint32_t * in) {
    __m128i  initoffset = _mm_set1_epi32 (initvalue);
    const __m128i* pin = (const __m128i*)(in);
    __m128i newvec = _mm_load_si128(pin);
    __m128i accumulator = Delta(newvec , initoffset);
    __m128i oldvec = newvec;
    uint32_t k = 1;
    for(; 4*k < SIMDBlockSize; ++k) {
        newvec = _mm_load_si128(pin+k);
        accumulator = _mm_or_si128(accumulator,Delta(newvec , oldvec));
        oldvec = newvec;
    }
    initoffset = oldvec;
    return maxbitas32int(accumulator);
}


/* maxbit over |length| integers with provided initial value */
uint32_t simdmaxbitsd1_length(uint32_t initvalue, const uint32_t * in,
                uint32_t length) {
    __m128i newvec;
    __m128i oldvec;
    __m128i initoffset;
    __m128i accumulator;
    const __m128i *pin;
    uint32_t tmparray[4];
    uint32_t k = 1;
    uint32_t acc;

    assert(length > 0);

    pin = (const __m128i *)(in);
    initoffset = _mm_set1_epi32(initvalue);
    switch (length) {
      case 1:
        newvec = _mm_set1_epi32(in[0]);
        break;
      case 2:
        newvec = _mm_setr_epi32(in[0], in[1], in[1], in[1]);
        break;
      case 3:
        newvec = _mm_setr_epi32(in[0], in[1], in[2], in[2]);
        break;
      default:
        newvec = _mm_load_si128(pin);
        break;
    }
    accumulator = Delta(newvec, initoffset);
    oldvec = newvec;

    /* process 4 integers and build an accumulator */
    while (k * 4 + 4 <= length) {
        newvec = _mm_load_si128(pin + k);
        accumulator = _mm_or_si128(accumulator, Delta(newvec, oldvec));
        oldvec = newvec;
        k++;
    }

    /* extract the accumulator as an integer */
    _mm_storeu_si128((__m128i *)(tmparray), accumulator);
    acc = tmparray[0] | tmparray[1] | tmparray[2] | tmparray[3];

    /* now process the remaining integers */
    for (k *= 4; k < length; k++)
        acc |= in[k] - (k == 0 ? initvalue : in[k - 1]);

    /* return the number of bits */
    return bits(acc);
}
