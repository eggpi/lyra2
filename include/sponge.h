#pragma once

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
