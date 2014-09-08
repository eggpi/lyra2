#include "sponge.h"
#include "lyra2.h"

#include <string.h>
#include <immintrin.h>
#include <assert.h>

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

    size_t basil_size = pwdlen + saltlen + 6 * sizeof(int);
    size_t row_length_i128 = SPONGE_EXTENDED_RATE_LENGTH_I128 * C;
    size_t matrix_size = R * row_length_i128 * sizeof(__m128i);
    if (matrix_size < basil_size + SPONGE_RATE_SIZE_BYTES) {
        // can't fit the basil inside the matrix for the initial
        // absorb operation
        return -1;
    }

    __m128i (*matrix)[row_length_i128] = malloc(R * sizeof(*matrix));
    assert(matrix_size == R * sizeof(*matrix));

    sponge_t *sponge = sponge_new();

    write_basil((uint8_t *) matrix, keylen, pwd, pwdlen, salt, saltlen, R, C, T);
    sponge_absorb(sponge, (uint8_t *) matrix, basil_size, 0);

    key++; // avoid warning for now
    free(matrix);
    sponge_destroy(sponge);
    return 0;
}
