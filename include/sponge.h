#pragma once

typedef struct sponge_s sponge_t;

sponge_t *sponge_new(void);
void sponge_destroy(sponge_t *sponge);
