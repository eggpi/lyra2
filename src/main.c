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

struct lyra2_parameters {
    unsigned int R, T, C;
};

struct lyra2_parameters params[] = {
    { 16,  16,  64},
    { 32,  32, 128},
    { 64,  64, 128},
    {128, 128, 128}
};

int
main(void) {
    char key[64] = {0};
    char *pwd = "Lyra sponge";
    char *salt = "saltsaltsaltsalt";

    for (unsigned int i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
        unsigned long results[NMEASUREMENTS] = {0};
        for (int j = 0; j < NMEASUREMENTS; j++) {
            struct timeval t0;
            struct timeval t1;
            gettimeofday(&t0, 0);

            lyra2(key, sizeof(key), pwd, strlen(pwd), salt, strlen(salt),
                  params[i].R, params[i].C, params[i].T);

            gettimeofday(&t1, 0);
            results[j] = (t1.tv_sec-t0.tv_sec) * 1000000 + t1.tv_usec-t0.tv_usec;
        }

        printf("Parameters: R = %u, C = %u, T = %u\n",
               params[i].R, params[i].C, params[i].T);

        for (unsigned int k = 0; k < sizeof(key); k++) {
            printf("%.2x ", (uint8_t) key[k]);
        }
        printf("\n");

        qsort(results, NMEASUREMENTS, sizeof(results[0]), cmp);
        printf("median time: %lu us\n", results[NMEASUREMENTS/2]);
        printf("\n");
    }

    return 0;
}
