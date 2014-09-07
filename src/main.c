#include "sponge.h"
#include <string.h>

int
main(void) {
    uint8_t squeezed[SPONGE_EXTENDED_RATE_SIZE_BYTES];
    memset(squeezed, 'A', sizeof(squeezed));
    sponge_t *sponge = sponge_new();
    sponge_absorb(sponge, squeezed, 63, SPONGE_FLAG_EXTENDED_RATE);
    sponge_reduced_extended_duplexing(sponge, squeezed, squeezed);
    sponge_destroy(sponge);
    return 0;
}
