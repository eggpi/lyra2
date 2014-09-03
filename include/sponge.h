#pragma once

#include <stdint.h>
#include <stdlib.h>

typedef struct sponge_s sponge_t;

sponge_t *sponge_new(void);
void sponge_destroy(sponge_t *sponge);
void sponge_squeeze(sponge_t *sponge, uint8_t *out, size_t outlen_bytes);
