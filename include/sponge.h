#pragma once

#include "static_assert.h"
#include "blake2b/blake2b-round.h"

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <immintrin.h>
#include <assert.h>
#include <string.h>

#ifdef USE_AVX
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

sponge_t *sponge_new(void);
void sponge_destroy(sponge_t *sponge);
void sponge_absorb(sponge_t *sponge, uint8_t *data, size_t datalen, int flags);
void sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen, int flags);
void sponge_reduced_extended_duplexing(sponge_t *sponge, const uint8_t inblock[static SPONGE_EXTENDED_RATE_SIZE_BYTES], uint8_t outblock[static SPONGE_EXTENDED_RATE_SIZE_BYTES]);

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

static inline void sponge_pad(uint8_t *data, size_t *datalen);
static inline void sponge_compress(sponge_t *sponge, bool reduced);

sponge_t *
sponge_new(void) {
    sponge_t *sponge = _mm_malloc(sizeof(sponge_t), SPONGE_MEM_ALIGNMENT);
    const size_t step = sizeof(sponge_word_t) / sizeof(uint64_t);
    for (unsigned int i = 0; i < SPONGE_STATE_LENGTH; i++) {
        sponge->state[i] = *((sponge_word_t *) (sponge_blake2b_IV + step*i));
    }

    return sponge;
}

void
sponge_destroy(sponge_t *sponge) {
    _mm_free(sponge);
}

void
sponge_absorb(sponge_t *sponge, uint8_t *data, size_t datalen, int flags) {
    if (!(flags & SPONGE_FLAG_ASSUME_PADDING)) {
        sponge_pad(data, &datalen);
    }

    size_t rate = SPONGE_RATE_LENGTH;
    if (flags & SPONGE_FLAG_EXTENDED_RATE) {
        rate = SPONGE_EXTENDED_RATE_LENGTH;
    }

    const sponge_word_t *dataw = (const sponge_word_t *) data;
    size_t datalenw = datalen / sizeof(sponge_word_t);

    assert(datalenw % rate == 0);
    while (datalenw) {
        for (unsigned int i = 0; i < rate; i++) {
            sponge->state[i] ^= dataw[i];
        }

        sponge_compress(sponge, !!(flags & SPONGE_FLAG_REDUCED));
        dataw += rate;
        datalenw -= rate;
    }

    return;
}

void
sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen, int flags) {
    sponge_word_t *outw = (sponge_word_t *) out;
    size_t outlenw = outlen / sizeof(sponge_word_t);

    while (outlenw >= SPONGE_EXTENDED_RATE_LENGTH) {
        for (unsigned int i = 0; i < SPONGE_EXTENDED_RATE_LENGTH; i++) {
            outw[i] = sponge->state[i];
        }

        sponge_compress(sponge, !!(flags & SPONGE_FLAG_REDUCED));
        outw += SPONGE_EXTENDED_RATE_LENGTH;
        outlenw -= SPONGE_EXTENDED_RATE_LENGTH;
    }

    for (unsigned int i = 0; i < outlenw; i++) {
        outw[i] = sponge->state[i];
    }

    return;
}

void
sponge_reduced_extended_duplexing(sponge_t *sponge,
        const uint8_t inblock[static SPONGE_EXTENDED_RATE_SIZE_BYTES],
        uint8_t outblock[static SPONGE_EXTENDED_RATE_SIZE_BYTES]) {
    // Lyra2 always duplexes single blocks of SPONGE_RATE_SIZE_BYTES bytes,
    // and doesn't pad them.
    const sponge_word_t *inblockw = (const sponge_word_t *) inblock;
    for (unsigned int i = 0; i < SPONGE_EXTENDED_RATE_LENGTH; i++) {
        sponge->state[i] ^= inblockw[i];
    }

    sponge_compress(sponge, true);

    sponge_word_t *outblockw = (sponge_word_t *) outblock;
    for (unsigned int i = 0; i < SPONGE_EXTENDED_RATE_LENGTH; i++) {
        outblockw[i] = sponge->state[i];
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
sponge_pad(uint8_t *data, size_t *datalen) {
    data[(*datalen)++] = 0x80;

    size_t mod = *datalen % SPONGE_RATE_SIZE_BYTES;
    if (mod) {
        size_t remaining = SPONGE_RATE_SIZE_BYTES - mod;
        memset(data + *datalen, 0, remaining);
        *datalen += remaining;
    }

    data[*datalen - 1] |= 0x01;
    assert(*datalen % SPONGE_RATE_SIZE_BYTES == 0);

    return;
}

static inline void
sponge_compress(sponge_t *sponge, bool reduced) {
    __m128i t0, t1, *v = (__m128i *) sponge->state;

    ROUND(0);
    if (!reduced) {
        ROUND(1);
        ROUND(2);
        ROUND(3);
        ROUND(4);
        ROUND(5);
        ROUND(6);
        ROUND(7);
        ROUND(8);
        ROUND(9);
        ROUND(10);
        ROUND(11);
    }

    return;
}
