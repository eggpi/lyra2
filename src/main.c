#include "lyra2.h"
#include <stdio.h>

#include <string.h>
#include <sys/time.h>

int
main(void) {
    char key[64];
    char *pwd = "Lyra sponge";
    char *salt = "saltsaltsaltsalt";

    struct timeval t0;
    struct timeval t1;
    gettimeofday(&t0, 0);

    lyra2(key, sizeof(key), pwd, strlen(pwd), salt, strlen(salt), 10, 64, 10);

    gettimeofday(&t1, 0);
    unsigned long elapsed = (t1.tv_sec-t0.tv_sec) * 1000000 + t1.tv_usec-t0.tv_usec;
    printf("Execution Time: %lu us\n", elapsed);

    for (unsigned int i = 0; i < sizeof(key); i++) {
        printf("%.2x ", ((uint8_t *) key)[i]);
    }
    printf("\n");
    return 0;
}
