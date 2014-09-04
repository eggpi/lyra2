#pragma once

#include <stdint.h>
#include <stdlib.h>
#include <stdbool.h>

typedef struct sponge_s sponge_t;

sponge_t *sponge_new(void);
void sponge_destroy(sponge_t *sponge);
void sponge_absorb(sponge_t *sponge, uint8_t *data, size_t datalen);
void sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen, bool reduced);
void sponge_reduced_duplexing(sponge_t *sponge, const uint8_t *inblock, uint8_t *outblock);
