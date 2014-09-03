#include "sponge.h"

int
main(void) {
    uint8_t squeezed[4096];
    sponge_t *sponge = sponge_new();
    sponge_squeeze(sponge, squeezed, sizeof(squeezed));
    sponge_destroy(sponge);
    return 0;
}
