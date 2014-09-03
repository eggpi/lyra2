#include "sponge.h"
#include "blake2b/blake2b-round.h"

#include "immintrin.h"

const unsigned int SPONGE_STATE_LENGTH_I128  = 8; // 8 * 128bits = 1024bits
const unsigned int SPONGE_RATE_LENGTH_I128 = SPONGE_STATE_LENGTH_I128 / 2;

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

static inline void sponge_compress(sponge_t *sponge);

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
sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen_bytes) {
    __m128i *out128 = (__m128i *) out;
    size_t outlen128 = outlen_bytes / sizeof(__m128i);

    while (outlen128 > SPONGE_RATE_LENGTH_I128) {
        for (unsigned int i = 0; i < SPONGE_RATE_LENGTH_I128; i++) {
            out128[i] = sponge->state[i];
        }

        sponge_compress(sponge);
        out128 += SPONGE_RATE_LENGTH_I128;
        outlen128 -= SPONGE_RATE_LENGTH_I128;
    }

    for (unsigned int i = 0; i < outlen128; i++) {
        out128[i] = sponge->state[i];
    }

    return;
}

static inline void
sponge_compress(sponge_t *sponge) {
    __m128i t0, t1, *v = sponge->state;

    ROUND(0);
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

    return;
}
