/*
   BLAKE2 reference source code package - optimized C implementations

   Written in 2012 by Samuel Neves <sneves@dei.uc.pt>

   To the extent possible under law, the author(s) have dedicated all copyright
   and related and neighboring rights to this software to the public domain
   worldwide. This software is distributed without any warranty.

   You should have received a copy of the CC0 Public Domain Dedication along with
   this software. If not, see <http://creativecommons.org/publicdomain/zero/1.0/>.
*/
#pragma once
#ifndef __BLAKE2B_ROUND_H__
#define __BLAKE2B_ROUND_H__

#include <immintrin.h>
#include "blake2b/blake2-config.h"

#define LOAD(p)  _mm_load_si128( (const __m128i *)(p) )
#define STORE(p,r) _mm_store_si128((__m128i *)(p), r)

#define LOADU(p)  _mm_loadu_si128( (const __m128i *)(p) )
#define STOREU(p,r) _mm_storeu_si128((__m128i *)(p), r)

#define TOF(reg) _mm_castsi128_ps((reg))
#define TOI(reg) _mm_castps_si128((reg))

#define LIKELY(x) __builtin_expect((x),1)


/* Microarchitecture-specific macros */
#ifdef HAVE_AVX
#define r16_256 _mm256_setr_epi8( 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9, 18, 19, 20, 21, 22, 23, 16, 17, 26, 27, 28, 29, 30, 31, 24, 25 )
#define r24_256 _mm256_setr_epi8( 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10, 19, 20, 21, 22, 23, 16, 17, 18, 27, 28, 29, 30, 31, 24, 25, 26 )
#define _mm256_roti_epi64(x, c) \
    (-(c) == 32) ? _mm256_shuffle_epi32((x), _MM_SHUFFLE(2,3,0,1))  \
    : (-(c) == 24) ? _mm256_shuffle_epi8((x), r24_256) \
    : (-(c) == 16) ? _mm256_shuffle_epi8((x), r16_256) \
    : (-(c) == 63) ? _mm256_xor_si256(_mm256_srli_epi64((x), -(c)), _mm256_add_epi64((x), (x)))  \
    : _mm256_xor_si256(_mm256_srli_epi64((x), -(c)), _mm256_slli_epi64((x), 64-(-(c))))
#else
#  ifndef HAVE_XOP
#    ifdef HAVE_SSSE3
#      define r16 _mm_setr_epi8( 2, 3, 4, 5, 6, 7, 0, 1, 10, 11, 12, 13, 14, 15, 8, 9 )
#      define r24 _mm_setr_epi8( 3, 4, 5, 6, 7, 0, 1, 2, 11, 12, 13, 14, 15, 8, 9, 10 )
#      define _mm_roti_epi64(x, c) \
          (-(c) == 32) ? _mm_shuffle_epi32((x), _MM_SHUFFLE(2,3,0,1))  \
          : (-(c) == 24) ? _mm_shuffle_epi8((x), r24) \
          : (-(c) == 16) ? _mm_shuffle_epi8((x), r16) \
          : (-(c) == 63) ? _mm_xor_si128(_mm_srli_epi64((x), -(c)), _mm_add_epi64((x), (x)))  \
          : _mm_xor_si128(_mm_srli_epi64((x), -(c)), _mm_slli_epi64((x), 64-(-(c))))
#    else
#      define _mm_roti_epi64(r, c) _mm_xor_si128(_mm_srli_epi64( (r), -(c) ),_mm_slli_epi64( (r), 64-(-c) ))
#    endif
#  else
/* ... */
#  endif
#endif

#ifdef HAVE_AVX
#define G1(row1,row2,row3,row4)        \
  row1 = _mm256_add_epi64(row1, row2); \
  row4 = _mm256_xor_si256(row4, row1); \
  row4 = _mm256_roti_epi64(row4, -32); \
  row3 = _mm256_add_epi64(row3, row4); \
  row2 = _mm256_xor_si256(row2, row3); \
  row2 = _mm256_roti_epi64(row2, -24); \

#define G2(row1,row2,row3,row4)        \
  row1 = _mm256_add_epi64(row1, row2); \
  row4 = _mm256_xor_si256(row4, row1); \
  row4 = _mm256_roti_epi64(row4, -16); \
  row3 = _mm256_add_epi64(row3, row4); \
  row2 = _mm256_xor_si256(row2, row3); \
  row2 = _mm256_roti_epi64(row2, -63); \

#define DIAGONALIZE(row1, row2, row3, row4) \
  row2 = _mm256_roti_epi64(row2, -192);     \
  row3 = _mm256_roti_epi64(row3, -128);     \
  row4 = _mm256_roti_epi64(row4, -64);      \

#define UNDIAGONALIZE(row1, row2, row3, row4) \
  row2 = _mm256_roti_epi64(row2, -64);        \
  row3 = _mm256_roti_epi64(row3, -128);       \
  row4 = _mm256_roti_epi64(row4, -192);       \

#define BLAKE2B_ROUND(v)                 \
  G1(v[0], v[1], v[2], v[3]);            \
  G2(v[0], v[1], v[2], v[3]);            \
  DIAGONALIZE(v[0], v[1], v[2], v[3]);   \
  G1(v[0], v[1], v[2], v[3]);            \
  G2(v[0], v[1], v[2], v[3]);            \
  UNDIAGONALIZE(v[0], v[1], v[2], v[3]); \

#else

#define G1(row1l,row2l,row3l,row4l,row1h,row2h,row3h,row4h) \
  row1l = _mm_add_epi64(row1l, row2l); \
  row1h = _mm_add_epi64(row1h, row2h); \
  \
  row4l = _mm_xor_si128(row4l, row1l); \
  row4h = _mm_xor_si128(row4h, row1h); \
  \
  row4l = _mm_roti_epi64(row4l, -32); \
  row4h = _mm_roti_epi64(row4h, -32); \
  \
  row3l = _mm_add_epi64(row3l, row4l); \
  row3h = _mm_add_epi64(row3h, row4h); \
  \
  row2l = _mm_xor_si128(row2l, row3l); \
  row2h = _mm_xor_si128(row2h, row3h); \
  \
  row2l = _mm_roti_epi64(row2l, -24); \
  row2h = _mm_roti_epi64(row2h, -24); \
 
#define G2(row1l,row2l,row3l,row4l,row1h,row2h,row3h,row4h) \
  row1l = _mm_add_epi64(row1l, row2l); \
  row1h = _mm_add_epi64(row1h, row2h); \
  \
  row4l = _mm_xor_si128(row4l, row1l); \
  row4h = _mm_xor_si128(row4h, row1h); \
  \
  row4l = _mm_roti_epi64(row4l, -16); \
  row4h = _mm_roti_epi64(row4h, -16); \
  \
  row3l = _mm_add_epi64(row3l, row4l); \
  row3h = _mm_add_epi64(row3h, row4h); \
  \
  row2l = _mm_xor_si128(row2l, row3l); \
  row2h = _mm_xor_si128(row2h, row3h); \
  \
  row2l = _mm_roti_epi64(row2l, -63); \
  row2h = _mm_roti_epi64(row2h, -63); \
 
#if defined(HAVE_SSSE3)
#define DIAGONALIZE(row1l,row2l,row3l,row4l,row1h,row2h,row3h,row4h) { \
  __m128i t0 = _mm_alignr_epi8(row2h, row2l, 8); \
  __m128i t1 = _mm_alignr_epi8(row2l, row2h, 8); \
  row2l = t0; \
  row2h = t1; \
  \
  t0 = row3l; \
  row3l = row3h; \
  row3h = t0;    \
  \
  t0 = _mm_alignr_epi8(row4h, row4l, 8); \
  t1 = _mm_alignr_epi8(row4l, row4h, 8); \
  row4l = t1; \
  row4h = t0; \
}

#define UNDIAGONALIZE(row1l,row2l,row3l,row4l,row1h,row2h,row3h,row4h) { \
  __m128i t0 = _mm_alignr_epi8(row2l, row2h, 8); \
  __m128i t1 = _mm_alignr_epi8(row2h, row2l, 8); \
  row2l = t0; \
  row2h = t1; \
  \
  t0 = row3l; \
  row3l = row3h; \
  row3h = t0; \
  \
  t0 = _mm_alignr_epi8(row4l, row4h, 8); \
  t1 = _mm_alignr_epi8(row4h, row4l, 8); \
  row4l = t1; \
  row4h = t0; \
}

#else

#define DIAGONALIZE(row1l,row2l,row3l,row4l,row1h,row2h,row3h,row4h) { \
  __m128i t0 = row4l;\
  __m128i t1 = row2l;\
  row4l = row3l;\
  row3l = row3h;\
  row3h = row4l;\
  row4l = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t0, t0)); \
  row4h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row4h, row4h)); \
  row2l = _mm_unpackhi_epi64(row2l, _mm_unpacklo_epi64(row2h, row2h)); \
  row2h = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(t1, t1)); \
}

#define UNDIAGONALIZE(row1l,row2l,row3l,row4l,row1h,row2h,row3h,row4h) { \
  __m128i t0 = row3l;\
  row3l = row3h;\
  row3h = t0;\
  t0 = row2l;\
  __m128i t1 = row4l;\
  row2l = _mm_unpackhi_epi64(row2h, _mm_unpacklo_epi64(row2l, row2l)); \
  row2h = _mm_unpackhi_epi64(t0, _mm_unpacklo_epi64(row2h, row2h)); \
  row4l = _mm_unpackhi_epi64(row4l, _mm_unpacklo_epi64(row4h, row4h)); \
  row4h = _mm_unpackhi_epi64(row4h, _mm_unpacklo_epi64(t1, t1)); \
}

#endif // HAVE_SSSE3

#define BLAKE2B_ROUND(v) \
  G1(v[0],v[2],v[4],v[6],v[1],v[3],v[5],v[7]); \
  G2(v[0],v[2],v[4],v[6],v[1],v[3],v[5],v[7]); \
  DIAGONALIZE(v[0],v[2],v[4],v[6],v[1],v[3],v[5],v[7]); \
  G1(v[0],v[2],v[4],v[6],v[1],v[3],v[5],v[7]); \
  G2(v[0],v[2],v[4],v[6],v[1],v[3],v[5],v[7]); \
  UNDIAGONALIZE(v[0],v[2],v[4],v[6],v[1],v[3],v[5],v[7]);

#endif // HAVE_AVX
#endif
