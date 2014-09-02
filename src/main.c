#include "sponge.h"

int
main(void) {
    sponge_t *sponge = sponge_new();
    sponge_destroy(sponge);
    return 0;
}
