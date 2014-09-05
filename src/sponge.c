#include "sponge.h"
#include "blake2b/blake2b-round.h"

#include <assert.h>
#include <string.h>

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
  __m128i state[SPONGE_STATE_LENGTH_I128];
};

static inline void sponge_pad(uint8_t *data, size_t *datalen);
static inline void sponge_compress(sponge_t *sponge, bool reduced);

sponge_t *
sponge_new(void) {
    sponge_t *sponge = _mm_malloc(sizeof(sponge_t), 16);
    for (unsigned int i = 0; i < SPONGE_STATE_LENGTH_I128; i++) {
        sponge->state[i] = *((__m128i *) (sponge_blake2b_IV + 2*i));
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

    size_t rate = SPONGE_RATE_LENGTH_I128;
    if (flags & SPONGE_FLAG_EXTENDED_RATE) {
        rate = SPONGE_EXTENDED_RATE_LENGTH_I128;
    }

    const __m128i *data128 = (const __m128i *) data;
    size_t datalen128 = datalen / sizeof(__m128i);

    assert(datalen128 % rate == 0);
    while (datalen128) {
        for (unsigned int i = 0; i < rate; i++) {
            sponge->state[i] ^= data128[i];
        }

        sponge_compress(sponge, !!(flags & SPONGE_FLAG_REDUCED));
        data128 += rate;
        datalen128 -= rate;
    }

    return;
}

void
sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen, int flags) {
    __m128i *out128 = (__m128i *) out;
    size_t outlen128 = outlen / sizeof(__m128i);

    while (outlen128 >= SPONGE_EXTENDED_RATE_LENGTH_I128) {
        for (unsigned int i = 0; i < SPONGE_EXTENDED_RATE_LENGTH_I128; i++) {
            out128[i] = sponge->state[i];
        }

        sponge_compress(sponge, !!(flags & SPONGE_FLAG_REDUCED));
        out128 += SPONGE_EXTENDED_RATE_LENGTH_I128;
        outlen128 -= SPONGE_EXTENDED_RATE_LENGTH_I128;
    }

    for (unsigned int i = 0; i < outlen128; i++) {
        out128[i] = sponge->state[i];
    }

    return;
}

void
sponge_reduced_duplexing(sponge_t *sponge, const uint8_t *inblock,
                         uint8_t *outblock) {
    // Lyra2 always duplexes single blocks of SPONGE_RATE_SIZE_BYTES bytes,
    // and doesn't pad them.
    const __m128i *inblock128 = (const __m128i *) inblock;
    for (unsigned int i = 0; i < SPONGE_RATE_LENGTH_I128; i++) {
        sponge->state[i] ^= inblock128[i];
    }

    sponge_compress(sponge, true);

    __m128i *outblock128 = (__m128i *) outblock;
    for (unsigned int i = 0; i < SPONGE_RATE_LENGTH_I128; i++) {
        outblock128[i] = sponge->state[i];
    }

    return;
}

/**
 * Append 10*1 padding to a chunk of data so its size is a multiple
 * of SPONGE_RATE_SIZE_BYTES.
 * Note: this function assumes there is extra space after |datalen_bytes|
 * in |data| to accommodate the padding; it will not allocate extra memory.
 */
static inline void
sponge_pad(uint8_t *data, size_t *datalen_bytes) {
    data[(*datalen_bytes)++] = 0x80;

    size_t mod = *datalen_bytes % SPONGE_RATE_SIZE_BYTES;
    if (mod) {
        size_t remaining = SPONGE_RATE_SIZE_BYTES - mod;
        memset(data + *datalen_bytes, 0, remaining);
        *datalen_bytes += remaining;
    }

    data[*datalen_bytes - 1] |= 0x01;
    assert(*datalen_bytes % SPONGE_RATE_SIZE_BYTES == 0);

    return;
}

static inline void
sponge_compress(sponge_t *sponge, bool reduced) {
    __m128i t0, t1, *v = sponge->state;

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
