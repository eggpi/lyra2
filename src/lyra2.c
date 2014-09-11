#include "sponge.h"
#include "lyra2.h"

#include <string.h>
#include <immintrin.h>
#include <assert.h>

static const size_t b = SPONGE_EXTENDED_RATE_SIZE_BYTES;
static const size_t b128 = SPONGE_EXTENDED_RATE_LENGTH_I128;
typedef __m128i block_t[SPONGE_EXTENDED_RATE_LENGTH_I128];

#define GEN_BLOCK_OPERATION(name, expr)                                     \
static inline void                                                          \
block_##name(block_t bdst, const block_t bsrc1, const block_t bsrc2) {      \
    const size_t nwords = sizeof(block_t) / sizeof(bdst[0]);                \
    for (unsigned int i = 0; i < nwords; i++) {                             \
        expr;                                                               \
    }                                                                       \
}

GEN_BLOCK_OPERATION(xor, bdst[i] = bsrc1[i] ^ bsrc2[i])
// FIXME should xor_rotw and wordwise_add be based on the word size (64 bits)?
GEN_BLOCK_OPERATION(xor_rotw, bdst[i] = bsrc1[i] ^ bsrc2[(i+nwords-1) % nwords])
GEN_BLOCK_OPERATION(wordwise_add, bdst[i] = bsrc1[i] + bsrc2[i]);

// FIXME should we be considering a word to be 128 bits here too?
static inline const __m128i *
block_get_lsw(const block_t block) {
    return &block[0];
}

static inline void
write_basil(uint8_t *buf, uint32_t keylen, const char *pwd,
            uint32_t pwdlen, const char *salt, uint32_t saltlen,
            uint32_t R, uint32_t C, uint32_t T) {
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
lyra2(char *key, uint32_t keylen, const char *pwd, uint32_t pwdlen,
      const char *salt, uint32_t saltlen, uint32_t R, uint32_t C,
      uint32_t T) {

    sponge_t *sponge = sponge_new();

    size_t basil_size = pwdlen + saltlen + 6 * sizeof(int);
    size_t row_length_i128 = b128 * C;
    size_t matrix_size = R * row_length_i128 * sizeof(__m128i);
    if (matrix_size < basil_size + SPONGE_RATE_SIZE_BYTES) {
        // can't fit the basil inside the matrix for the initial
        // absorb operation
        return -1;
    }

    block_t (*matrix)[C] = malloc(R * sizeof(*matrix));
    assert(matrix_size == R * sizeof(*matrix));

    /* Setup phase */
    write_basil((uint8_t *) matrix, keylen, pwd, pwdlen, salt, saltlen, R, C, T);
    sponge_absorb(sponge, (uint8_t *) matrix, basil_size, 0);

    for (unsigned int col = 0; col < C; col++) {
        int flags = 0;
        flags |= SPONGE_FLAG_REDUCED;
        flags |= SPONGE_FLAG_EXTENDED_RATE;
        flags |= SPONGE_FLAG_ASSUME_PADDING;
        sponge_squeeze(sponge, (uint8_t *) &matrix[0][C-1-col], b, flags);
    }

    for (unsigned int col = 0; col < C; col++) {
        sponge_reduced_extended_duplexing(sponge,
            (const uint8_t *) &matrix[0][col],
            (uint8_t *) &matrix[1][C-1-col]);

        block_xor(matrix[1][C-1-col], matrix[1][C-1-col], matrix[0][col]);
    }

    /* Filling loop */
    block_t rand;
    int64_t gap = 1, stp = 1;
    uint64_t rrow = 0, prev = 1, row = 2, wnd = 2;

    do {
        for (unsigned int col = 0; col < C; col++) {
            block_wordwise_add(rand, matrix[prev][col], matrix[rrow][col]);
            sponge_reduced_extended_duplexing(sponge, (uint8_t *) rand,
                                              (uint8_t *) rand);
            block_xor(matrix[row][C-1-col], matrix[prev][col], rand);
            block_xor_rotw(matrix[rrow][col], matrix[rrow][col], rand);
        }
        rrow = (rrow + stp) % wnd;
        prev = row;
        row += 1;
        if (rrow == 0) {
            stp = wnd + gap;
            wnd = 2*wnd;
            gap = -gap;
        }
    } while (row <= R - 1);

    /* Wandering phase */
    row = 0;
    for (unsigned int tau = 1; tau <= T; tau++) {
        stp = (tau % 2 == 0) ? -1 : R/2 - 1;
        do {
            // FIXME is this really what the spec says?
            rrow = *((uint32_t *) block_get_lsw(rand)) % R;
            for (unsigned int col = 0; col < C; col++) {
                block_wordwise_add(rand, matrix[prev][col], matrix[rrow][col]);
                sponge_reduced_extended_duplexing(sponge,
                    (const uint8_t *) rand, (uint8_t *) rand);
                block_xor(matrix[row][col], matrix[row][col], rand);
                block_xor_rotw(matrix[rrow][col], matrix[rrow][col], rand);
            }
            prev = row;
            row = (row + stp) % R;
        } while (row != 0);
    }

    sponge_absorb(sponge, (uint8_t *) matrix[rrow][0], sizeof(block_t),
        SPONGE_FLAG_ASSUME_PADDING | SPONGE_FLAG_EXTENDED_RATE);
    sponge_squeeze(sponge, (uint8_t *) key, keylen, SPONGE_FLAG_EXTENDED_RATE);

    free(matrix);
    sponge_destroy(sponge);
    return 0;
}
