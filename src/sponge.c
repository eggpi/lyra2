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
