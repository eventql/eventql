/**
 * This code is released under a BSD License.
 */
#ifndef SIMDBITPACKING_H_
#define SIMDBITPACKING_H_

#include "portability.h"

/* SSE2 is required */
#include <emmintrin.h>
/* for memset */
#include <string.h>

/* reads 128 values from "in", writes  "bit" 128-bit vectors to "out" */
void simdpack(const uint32_t *  in,__m128i *  out, const uint32_t bit);

/* reads 128 values from "in", writes  "bit" 128-bit vectors to "out" */
void simdpackwithoutmask(const uint32_t *  in,__m128i *  out, const uint32_t bit);

/* reads  "bit" 128-bit vectors from "in", writes  128 values to "out" */
void simdunpack(const __m128i *  in,uint32_t *  out, const uint32_t bit);


#endif /* SIMDBITPACKING_H_ */
