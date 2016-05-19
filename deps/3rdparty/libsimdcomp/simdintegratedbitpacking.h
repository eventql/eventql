/**
 * This code is released under a BSD License.
 */

#ifndef SIMD_INTEGRATED_BITPACKING_H
#define SIMD_INTEGRATED_BITPACKING_H

#include "portability.h"

/* SSE2 is required */
#include <emmintrin.h>

#include "simdcomputil.h"

/* reads 128 values from "in", writes  "bit" 128-bit vectors to "out"
   integer values should be in sorted order (for best results) */
void simdpackd1(uint32_t initvalue, const uint32_t *  in,__m128i *  out, const uint32_t bit);


/* reads 128 values from "in", writes  "bit" 128-bit vectors to "out"
   integer values should be in sorted order (for best results) */
void simdpackwithoutmaskd1(uint32_t initvalue, const uint32_t *  in,__m128i *  out, const uint32_t bit);


/* reads "bit" 128-bit vectors from "in", writes  128 values to "out" */
void simdunpackd1(uint32_t initvalue, const __m128i *  in,uint32_t *  out, const uint32_t bit);


#endif
