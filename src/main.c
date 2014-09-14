#include "lyra2.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#define NMEASUREMENTS 1000

int
cmp(const void *xv, const void *yv) {
    unsigned long x = *((unsigned long *) xv), y = *((unsigned long *) yv);
    if (x < y) return -1;
    return x != y;
}

int
main(void) {
    char key[64];
    char *pwd = "Lyra sponge";
    char *salt = "saltsaltsaltsalt";
    unsigned long results[NMEASUREMENTS] = {0};

    for (int i = 0; i < NMEASUREMENTS; i++) {
        struct timeval t0;
        struct timeval t1;
        gettimeofday(&t0, 0);

        lyra2(key, sizeof(key), pwd, strlen(pwd), salt, strlen(salt), 16, 64, 10);

        gettimeofday(&t1, 0);
        results[i] = (t1.tv_sec-t0.tv_sec) * 1000000 + t1.tv_usec-t0.tv_usec;
    }

    for (unsigned int i = 0; i < sizeof(key); i++) {
        printf("%.2x ", ((uint8_t *) key)[i]);
    }
    printf("\n");

    qsort(results, NMEASUREMENTS, sizeof(results[0]), cmp);
    printf("median time: %lu us\n", results[NMEASUREMENTS/2]);

    return 0;
}
