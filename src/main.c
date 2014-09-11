#include "lyra2.h"
#include <stdio.h>

#include <string.h>

int
main(void) {
    char key[64];
    char *pwd = "Lyra sponge";
    char *salt = "saltsaltsaltsalt";

    lyra2(key, sizeof(key), pwd, strlen(pwd), salt, strlen(salt), 10, 64, 10);
    for (unsigned int i = 0; i < sizeof(key); i++) {
        printf("%.2x ", ((uint8_t *) key)[i]);
    }
    printf("\n");
    return 0;
}
