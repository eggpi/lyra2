#include "sponge.h"
#include <string.h>

int
main(void) {
    uint8_t squeezed[64];
    memset(squeezed, 'A', sizeof(squeezed));
    sponge_t *sponge = sponge_new();
    sponge_absorb(sponge, squeezed, 63, 0);
    sponge_squeeze(sponge, squeezed, sizeof(squeezed), 0);
    sponge_destroy(sponge);
    return 0;
}
