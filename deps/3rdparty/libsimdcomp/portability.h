/**
 * This code is released under a BSD License.
 */
#ifndef SIMDBITCOMPAT_H_
#define SIMDBITCOMPAT_H_


#if SIMDCOMP_DEBUG
# define SIMDCOMP_ALWAYS_INLINE inline
# define SIMDCOMP_NEVER_INLINE
# define SIMDCOMP_PURE
#else
# if defined(__GNUC__)
#  if __GNUC__ >= 3
#   define SIMDCOMP_ALWAYS_INLINE inline __attribute__((always_inline))
#   define SIMDCOMP_NEVER_INLINE __attribute__((noinline))
#   define SIMDCOMP_PURE __attribute__((pure))
#  else
#   define SIMDCOMP_ALWAYS_INLINE inline
#   define SIMDCOMP_NEVER_INLINE
#   define SIMDCOMP_PURE
#  endif
# elif defined(_MSC_VER)
#  define SIMDCOMP_ALWAYS_INLINE __forceinline
#  define SIMDCOMP_NEVER_INLINE
#  define SIMDCOMP_PURE
# else
#  if __has_attribute(always_inline)
#   define SIMDCOMP_ALWAYS_INLINE inline __attribute__((always_inline))
#  else
#   define SIMDCOMP_ALWAYS_INLINE inline
#  endif
#  if __has_attribute(noinline)
#   define SIMDCOMP_NEVER_INLINE __attribute__((noinline))
#  else
#   define SIMDCOMP_NEVER_INLINE
#  endif
#  if __has_attribute(pure)
#   define SIMDCOMP_PURE __attribute__((pure))
#  else
#   define SIMDCOMP_PURE
#  endif
# endif
#endif

typedef unsigned int uint32_t;
typedef unsigned char uint8_t;

#endif /* SIMDBITCOMPAT_H_ */
