#pragma once

/*
 * An implementation of a sponge suitable for use with Lyra2.
 *
 * This exposes an opaque sponge_t data type whose internal state is an array of
 * sponge_word_t. The following functions are used to create, destroy and
 * manipulate sponge_t instances:
 *
 *   static sponge_t *sponge_new(void)
 *   static void sponge_destroy(sponge_t *sponge)
 * Create and destroy sponge instances.
 *
 *   void sponge_absorb(sponge_t *sponge, sponge_word_t *data,
 *       size_t databytes, int flags);
 * Absorb the data in the buffer given by |data|, which is of length |databytes|
 * in bytes.
 *
 *   static void sponge_squeeze(sponge_t *sponge, sponge_word_t *out,
 *       size_t outbytes, int flags);
 *   static void sponge_squeeze_unaligned(sponge_t *sponge, sponge_word_t *out,
 *       size_t outbytes, int flags);
 * Squeeze |outbytes| bytes out of the sponge and into the buffer pointed to by
 * |out|.
 *
 *   static void sponge_reduced_extended_duplexing(sponge_t *sponge,
 *       const sponge_word_t inblock[static SPONGE_EXTENDED_RATE_LENGTH],
 *       sponge_word_t outblock[static SPONGE_EXTENDED_RATE_LENGTH]);
 * Duplex |inblock|, which is assumed to be of size
 * SPONGE_EXTENDED_RATE_SIZE_BYTES, leaving the result in |outblock|. As its
 * name (cryptically) implies, this will always use the extended rate and
 * reduced-round compression function. See the flags section below for an
 * explanation of these terms.
 *
 * Aside from the in/out sponge_word_t data parameters and their respective
 * lengths in bytes, most of these functions also accept a |flags| parameter,
 * which is the OR of an applicable subset of the following flags:
 *
 * - SPONGE_FLAG_ASSUME_PADDING: if OR-ed into the flags, no padding will be
 *   applied to the input data. Otherwise, it will be 10*1 padded to a length
 *   that is a multiple of SPONGE_RATE_SIZE_BYTES. It will be assumed that the
 *   input buffer contains enough space to hold the padding. The presence or
 *   absence of this flag only affects sponge_absorb; the other API functions
 *   will implicitly assume padding.
 * - SPONGE_FLAG_EXTENDED_RATE: if OR-ed into the flags, the sponge will treat
 *   its internal state as having a rate of SPONGE_EXTENDED_RATE_SIZE_BYTES
 *   bytes, as opposed to SPONGE_RATE_SIZE_BYTES, which is used when this flag
 *   is absent. The presence or absence of this flag only affects sponge_absorb;
 *   the other API functions will implicitly use the extended rate. When using
 *   the extended rate, the length of the input data is also assumed to be a
 *   multiple of SPONGE_EXTENDED_RATE_SIZE_BYTES bytes.
 * - SPONGE_FLAG_REDUCED: if OR-ed into the flags, the sponge will apply a
 *   faster reduced-round compression function to its internal state instead of
 *   the normal full-round compression function. This flag only alters the
 *   behavior of sponge_absorb and sponge_squeeze{,unaligned}.
 *
 * Except for sponge_squeeze_unaligned, all of the sponge_* functions assume
 * that its sponge_word_t (or const sponge_word_t *) parameters are aligned to
 * a SPONGE_MEM_ALIGNMENT-byte boundary.
 *
 * All other symbols in this file which are not mentioned in the above
 * description are to be considered implementation details.
 */

#include "static_assert.h"
#include "blake2b/blake2b-round.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <immintrin.h>
#include <assert.h>
#include <string.h>

#ifdef HAVE_AVX2
typedef __m256i sponge_word_t;
#define SPONGE_MEM_ALIGNMENT 32
#else
typedef __m128i sponge_word_t;
#define SPONGE_MEM_ALIGNMENT 16
#endif

#define SPONGE_STATE_SIZE_BYTES ((size_t) 128)

STATIC_ASSERT(SPONGE_STATE_SIZE_BYTES % sizeof(sponge_word_t) == 0, sponge_word_divides_state_size);
#define SPONGE_STATE_LENGTH (SPONGE_STATE_SIZE_BYTES / sizeof(sponge_word_t))

STATIC_ASSERT(SPONGE_STATE_LENGTH % 2 == 0, state_length_is_even);
#define SPONGE_RATE_LENGTH (SPONGE_STATE_LENGTH / 2)
#define SPONGE_RATE_SIZE_BYTES (SPONGE_RATE_LENGTH * sizeof(sponge_word_t))

#define SPONGE_EXTENDED_RATE_SIZE_BYTES 96
STATIC_ASSERT(SPONGE_EXTENDED_RATE_SIZE_BYTES % sizeof(sponge_word_t) == 0, sponge_word_divides_extended_rate);
#define SPONGE_EXTENDED_RATE_LENGTH (SPONGE_EXTENDED_RATE_SIZE_BYTES / sizeof(sponge_word_t))

#define SPONGE_FLAG_EXTENDED_RATE  (1 << 0)
#define SPONGE_FLAG_ASSUME_PADDING (1 << 1)
#define SPONGE_FLAG_REDUCED        (1 << 2)

typedef struct sponge_s sponge_t;

static sponge_t *sponge_new(void);
static void sponge_destroy(sponge_t *sponge);
static void sponge_absorb(sponge_t *sponge, sponge_word_t *data, size_t databytes, int flags);
static void sponge_squeeze(sponge_t *sponge, sponge_word_t *out, size_t outbytes, int flags);
static void sponge_squeeze_unaligned(sponge_t *sponge, sponge_word_t *out, size_t outbytes, int flags);
static void sponge_reduced_extended_duplexing(sponge_t *sponge, const sponge_word_t inblock[static SPONGE_EXTENDED_RATE_LENGTH], sponge_word_t outblock[static SPONGE_EXTENDED_RATE_LENGTH]);

#if defined(_MSC_VER)
#define ALIGN(x) __declspec(align(x))
#else
#define ALIGN(x) __attribute__ ((__aligned__(x)))
#endif

ALIGN(SPONGE_MEM_ALIGNMENT)
static const uint64_t sponge_blake2b_IV[16] = {
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x0000000000000000ULL, 0x0000000000000000ULL,
    0x6a09e667f3bcc908ULL, 0xbb67ae8584caa73bULL,
    0x3c6ef372fe94f82bULL, 0xa54ff53a5f1d36f1ULL,
    0x510e527fade682d1ULL, 0x9b05688c2b3e6c1fULL,
    0x1f83d9abfb41bd6bULL, 0x5be0cd19137e2179ULL
};

struct sponge_s {
  sponge_word_t state[SPONGE_STATE_LENGTH];
};

static inline void sponge_pad(uint8_t *data, size_t *databytes);
static inline void sponge_compress(sponge_t *sponge, bool reduced);

static sponge_t *
sponge_new(void) {
    sponge_t *sponge = _mm_malloc(sizeof(sponge_t), SPONGE_MEM_ALIGNMENT);
    const size_t step = sizeof(sponge_word_t) / sizeof(uint64_t);
    for (unsigned int i = 0; i < SPONGE_STATE_LENGTH; i++) {
        sponge->state[i] = *((sponge_word_t *) (sponge_blake2b_IV + step*i));
    }

    return sponge;
}

static void
sponge_destroy(sponge_t *sponge) {
    _mm_free(sponge);
}

static inline void
sponge_absorb(sponge_t *sponge, sponge_word_t *data, size_t databytes, int flags) {
    if (!(flags & SPONGE_FLAG_ASSUME_PADDING)) {
        sponge_pad((uint8_t *) data, &databytes);
    }

    size_t rate = SPONGE_RATE_LENGTH;
    if (flags & SPONGE_FLAG_EXTENDED_RATE) {
        rate = SPONGE_EXTENDED_RATE_LENGTH;
    }

    size_t datalenw = databytes / sizeof(sponge_word_t);
    assert(datalenw % rate == 0);

    while (datalenw) {
        for (unsigned int i = 0; i < rate; i++) {
            sponge->state[i] ^= data[i];
        }

        sponge_compress(sponge, !!(flags & SPONGE_FLAG_REDUCED));
        data += rate;
        datalenw -= rate;
    }

    return;
}

#define SPONGE_SQUEEZE_BODY                                                \
    size_t outlenw = outbytes / sizeof(sponge_word_t);                     \
                                                                           \
    while (outlenw >= SPONGE_EXTENDED_RATE_LENGTH) {                       \
        COPY_SPONGE_WORDS(out, sponge->state, SPONGE_EXTENDED_RATE_LENGTH) \
                                                                           \
        sponge_compress(sponge, !!(flags & SPONGE_FLAG_REDUCED));          \
        out += SPONGE_EXTENDED_RATE_LENGTH;                                \
        outlenw -= SPONGE_EXTENDED_RATE_LENGTH;                            \
    }                                                                      \
                                                                           \
    COPY_SPONGE_WORDS(out, sponge->state, outlenw)                         \
    return;

static inline void
sponge_squeeze(sponge_t *sponge, sponge_word_t *out, size_t outbytes, int flags) {
#define COPY_SPONGE_WORDS(dst, src, nwords)      \
    for (unsigned int i = 0; i < nwords; i++) {  \
        dst[i] = src[i];                         \
    }
    SPONGE_SQUEEZE_BODY
#undef COPY_SPONGE_WORDS
}

static inline void
sponge_squeeze_unaligned(sponge_t *sponge, sponge_word_t *out, size_t outbytes, int flags) {
#define COPY_SPONGE_WORDS(dst, src, nwords) \
    memcpy(dst, src, nwords * sizeof(sponge_word_t));
    SPONGE_SQUEEZE_BODY
#undef COPY_SPONGE_WORDS
}

static inline void
sponge_reduced_extended_duplexing(sponge_t *sponge,
        const sponge_word_t inblock[static SPONGE_EXTENDED_RATE_LENGTH],
        sponge_word_t outblock[static SPONGE_EXTENDED_RATE_LENGTH]) {
    // Lyra2 always duplexes single blocks of SPONGE_RATE_SIZE_BYTES bytes,
    // and doesn't pad them.
    for (unsigned int i = 0; i < SPONGE_EXTENDED_RATE_LENGTH; i++) {
        sponge->state[i] ^= inblock[i];
    }

    sponge_compress(sponge, true);

    for (unsigned int i = 0; i < SPONGE_EXTENDED_RATE_LENGTH; i++) {
        outblock[i] = sponge->state[i];
    }

    return;
}

/**
 * Append 10*1 padding to a chunk of data so its size is a multiple
 * of SPONGE_RATE_SIZE_BYTES.
 * Note: this function assumes there is extra space after |datalen|
 * in |data| to accommodate the padding; it will not allocate extra memory.
 */
static inline void
sponge_pad(uint8_t *data, size_t *databytes) {
    data[(*databytes)++] = 0x80;

    size_t mod = *databytes % SPONGE_RATE_SIZE_BYTES;
    if (mod) {
        size_t remaining = SPONGE_RATE_SIZE_BYTES - mod;
        memset(data + *databytes, 0, remaining);
        *databytes += remaining;
    }

    data[*databytes - 1] |= 0x01;
    assert(*databytes % SPONGE_RATE_SIZE_BYTES == 0);

    return;
}

static inline void
sponge_compress(sponge_t *sponge, bool reduced) {
    BLAKE2B_ROUND(sponge->state)
    if (!reduced) {
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
        BLAKE2B_ROUND(sponge->state)
    }

    return;
}
