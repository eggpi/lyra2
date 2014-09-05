#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>
#include <immintrin.h>

#define SPONGE_STATE_LENGTH_I128 (size_t) 8 // 8 * 128bits = 1024bits
#define SPONGE_STATE_SIZE_BYTES (SPONGE_STATE_LENGTH_I128 * sizeof(__m128i))

#define SPONGE_RATE_LENGTH_I128 (SPONGE_STATE_LENGTH_I128 / 2)
#define SPONGE_RATE_SIZE_BYTES (SPONGE_RATE_LENGTH_I128 * sizeof(__m128i))

#define SPONGE_EXTENDED_RATE_LENGTH_I128 6 // 6 * 128bits = 768bits

typedef struct sponge_s sponge_t;

sponge_t *sponge_new(void);
void sponge_destroy(sponge_t *sponge);
void sponge_absorb(sponge_t *sponge, uint8_t *data, size_t datalen);
void sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen, bool reduced);
void sponge_reduced_duplexing(sponge_t *sponge, const uint8_t *inblock, uint8_t *outblock);
