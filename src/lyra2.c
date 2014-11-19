#include "sponge.h"
#include "lyra2.h"
#include "static_assert.h"
#include "timeit.h"

#include <string.h>
#include <immintrin.h>
#include <assert.h>

#ifdef __WORDSIZE
#define W (__WORDSIZE)
#else
#define W (sizeof(void *) * CHAR_BIT)
#endif
STATIC_ASSERT(W == 64, word_size_is_64bit);

typedef sponge_word_t bword_t;
STATIC_ASSERT(SPONGE_EXTENDED_RATE_SIZE_BYTES % sizeof(bword_t) == 0, L_divides_sponge_extended_rate);
STATIC_ASSERT(sizeof(bword_t) % W, L_is_a_multiple_of_the_word_size);
#define nbwords (SPONGE_EXTENDED_RATE_SIZE_BYTES / sizeof(bword_t))
typedef bword_t block_t[nbwords];
STATIC_ASSERT(sizeof(block_t) % SPONGE_MEM_ALIGNMENT == 0, blocks_are_properly_aligned);

#define GEN_BLOCK_OPERATION(name, expr, ...)                                          \
static inline void                                                                    \
block_##name(block_t bdst, const block_t bsrc1, const block_t bsrc2, ##__VA_ARGS__) { \
    for (unsigned int i = 0; i < nbwords; i++) {                                      \
        expr;                                                                         \
    }                                                                                 \
}

GEN_BLOCK_OPERATION(xor, bdst[i] = bsrc1[i] ^ bsrc2[i])
GEN_BLOCK_OPERATION(wordwise_add, bdst[i] = bsrc1[i] + bsrc2[i])
GEN_BLOCK_OPERATION(xor_rotR, bdst[i] = bsrc1[i] ^ bsrc2[(i+rot) % nbwords], unsigned int rot)

static inline uint64_t
block_get_lsw_from_bword(const block_t block, unsigned int bwordidx) {
    return *((uint64_t *) &block[bwordidx]);
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

#ifdef USE_PHS_INTERFACE
static
#endif
int
lyra2(const char *key, uint32_t keylen, const char *pwd, uint32_t pwdlen,
      const char *salt, uint32_t saltlen, uint32_t R, uint32_t C,
      uint32_t T) {

    sponge_t *sponge = sponge_new();

    size_t basil_size = pwdlen + saltlen + 6 * sizeof(int);
    size_t matrix_size = R * C * sizeof(block_t);
    if (matrix_size < basil_size + SPONGE_RATE_SIZE_BYTES) {
        // can't fit the basil inside the matrix for the initial
        // absorb operation
        return -1;
    }

    block_t (*matrix)[C] = _mm_malloc(R * sizeof(*matrix), SPONGE_MEM_ALIGNMENT);
    assert(matrix_size == R * sizeof(*matrix));

    /* Bootstrapping phase */
    block_t rand;
    int64_t gap = 1, stp = 1;
    uint64_t prev0 = 2, row0 = 0, row1 = 1, prev1 = 0, wnd = 2;

    write_basil((uint8_t *) matrix, keylen, pwd, pwdlen, salt, saltlen, R, C, T);
    sponge_absorb(sponge, (uint8_t *) matrix, basil_size, 0);

    /* Setup phase */
    for (unsigned int col = 0; col < C; col++) {
        int flags = 0;
        flags |= SPONGE_FLAG_REDUCED;
        flags |= SPONGE_FLAG_EXTENDED_RATE;
        flags |= SPONGE_FLAG_ASSUME_PADDING;
        sponge_squeeze(sponge, (uint8_t *) &matrix[0][C-1-col], sizeof(block_t), flags);
    }

    for (unsigned int col = 0; col < C; col++) {
        sponge_reduced_extended_duplexing(sponge,
            (const uint8_t *) &matrix[0][col],
            (uint8_t *) &matrix[1][C-1-col]);

        block_xor(matrix[1][C-1-col], matrix[1][C-1-col], matrix[0][col]);
    }

    for (unsigned int col = 0; col < C; col++) {
        block_wordwise_add(rand, matrix[0][col], matrix[1][col]);
        sponge_reduced_extended_duplexing(sponge,
            (const uint8_t *) rand,
            (uint8_t *) rand);
        block_xor(matrix[2][C-1-col], matrix[1][col], rand);
        block_xor_rotR(matrix[0][col], matrix[0][col], rand, 1);
    }

    /* Filling loop */
    for (row0 = 3; row0 < R; row0++) {
        for (unsigned int col = 0; col < C; col++) {
            block_wordwise_add(rand, matrix[row1][col], matrix[prev0][col]);
            block_wordwise_add(rand, rand, matrix[prev1][col]);
            sponge_reduced_extended_duplexing(sponge, (uint8_t *) rand,
                                              (uint8_t *) rand);
            block_xor(matrix[row0][C-1-col], matrix[prev0][col], rand);
            block_xor_rotR(matrix[row1][col], matrix[row1][col], rand, 1);
        }
        prev0 = row0;
        prev1 = row1;
        row1 = (row1 + stp) & (wnd - 1);
        if (row1 == 0) {
            stp = wnd + gap;
            wnd = 2*wnd;
            gap = -gap;
        }
    }

    /* Wandering phase */
    uint64_t col0 = 0, col1 = 0;
    for (unsigned int tau = 1; tau <= T; tau++) {
        for (unsigned int i = 0; i < R; i++) {
            row0 = block_get_lsw_from_bword(rand, 0) % R;
            row1 = block_get_lsw_from_bword(rand, 1) % R;

            for (unsigned int col = 0; col < C; col++) {
                col0 = block_get_lsw_from_bword(rand, 2) % C,
                col1 = block_get_lsw_from_bword(rand, 3) % C;

                block_wordwise_add(rand, matrix[row0][col], matrix[row1][col]);
                block_wordwise_add(rand, rand, matrix[prev0][col0]);
                block_wordwise_add(rand, rand, matrix[prev1][col1]);
                sponge_reduced_extended_duplexing(sponge,
                    (const uint8_t *) rand, (uint8_t *) rand);

                block_xor_rotR(matrix[row0][col], matrix[row0][col], rand, 0);
                block_xor_rotR(matrix[row1][col], matrix[row1][col], rand, 1);
            }
            prev0 = row0;
            prev1 = row1;
        }
    }

    sponge_absorb(sponge, (uint8_t *) matrix[row0][col0], sizeof(block_t),
        SPONGE_FLAG_ASSUME_PADDING | SPONGE_FLAG_EXTENDED_RATE);
    sponge_squeeze_unaligned(sponge, (uint8_t *) key, keylen, SPONGE_FLAG_EXTENDED_RATE);

    _mm_free(matrix);
    sponge_destroy(sponge);
    return 0;
}

#ifdef USE_PHS_INTERFACE
int
PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt,
    size_t saltlen, unsigned int t_cost, unsigned int m_cost) {
    return lyra2(out, outlen, in, inlen, salt, saltlen, m_cost, PHS_NCOLS, t_cost);
}
#endif
