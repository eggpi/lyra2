#include "sponge.h"
#include "lyra2.h"

#include <string.h>
#include <immintrin.h>
#include <assert.h>

static const size_t b = SPONGE_EXTENDED_RATE_SIZE_BYTES;
static const size_t b128 = SPONGE_EXTENDED_RATE_LENGTH_I128;
typedef __m128i block_t[b128];

static inline void
write_basil(uint8_t *buf, unsigned int keylen, const char *pwd,
            unsigned int pwdlen, const char *salt, unsigned int saltlen,
            unsigned int R, unsigned int C, unsigned int T) {
#define WRITE_PTR(ptr, size) \
    memcpy(buf, ptr, size);  \
    buf += size;

#define WRITE_VALUE(value)              \
    memcpy(buf, &value, sizeof(value)); \
    buf += sizeof(value);

    // FIXME: using memcpy assumes little-endian architecture

    WRITE_PTR(pwd, pwdlen)
    WRITE_PTR(salt, saltlen)
    WRITE_VALUE(keylen)
    WRITE_VALUE(pwdlen)
    WRITE_VALUE(saltlen)
    WRITE_VALUE(T)
    WRITE_VALUE(R)
    WRITE_VALUE(C)

#undef WRITE_PTR
#undef WRITE_VALUE
    return;
}

int
lyra2(char *key, unsigned int keylen, const char *pwd, unsigned int pwdlen,
      const char *salt, unsigned int saltlen, unsigned int R, unsigned int C,
      unsigned int T) {

    sponge_t *sponge = sponge_new();

    size_t basil_size = pwdlen + saltlen + 6 * sizeof(int);
    size_t row_length_i128 = b128 * C;
    size_t matrix_size = R * row_length_i128 * sizeof(__m128i);
    if (matrix_size < basil_size + SPONGE_RATE_SIZE_BYTES) {
        // can't fit the basil inside the matrix for the initial
        // absorb operation
        return -1;
    }

    __m128i (*matrix)[row_length_i128] = malloc(R * sizeof(*matrix));
    assert(matrix_size == R * sizeof(*matrix));

    write_basil((uint8_t *) matrix, keylen, pwd, pwdlen, salt, saltlen, R, C, T);
    sponge_absorb(sponge, (uint8_t *) matrix, basil_size, 0);

    for (unsigned int col = 0; col < C; col++) {
        int flags = 0;
        flags |= SPONGE_FLAG_REDUCED;
        flags |= SPONGE_FLAG_EXTENDED_RATE;
        flags |= SPONGE_FLAG_ASSUME_PADDING;
        sponge_squeeze(sponge, (uint8_t *) &matrix[0][(C-1-col)*b128], b, flags);
    }

    for (unsigned int col = 0; col < C; col++) {
        sponge_reduced_extended_duplexing(sponge,
            (const uint8_t *) &matrix[0][col*b128],
            (uint8_t *) &matrix[1][(C-1-col)*b128]);

        for (unsigned int i = 0; i < b128; i++) {
            matrix[1][(C-1-col)*b128+i] ^= matrix[0][col*b128+i];
        }
    }

    key++; // avoid warning for now
    free(matrix);
    sponge_destroy(sponge);
    return 0;
}
