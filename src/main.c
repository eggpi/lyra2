#include "lyra2.h"

#include <math.h>
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
    { 64,  64, 128}
};

float
compute_standard_deviation(unsigned long *values) {
    float avg = 0.0f, var = 0.0f;
    for (int i = 0; i < NMEASUREMENTS; i++) {
        avg += ((float) values[i]) / ((float) NMEASUREMENTS);
    }
    for (int i = 0; i < NMEASUREMENTS; i++) {
        var += pow(((float) values[i]) - avg, 2) / ((float) NMEASUREMENTS);
    }
    return sqrt(var);
}

int
main(void) {
    char key[64] = {0};
    char *pwd = "Lyra sponge";
    char *salt = "saltsaltsaltsalt";

    printf("Input:\n  Password: '%s'\n  Salt: '%s'\n\n", pwd, salt);
    for (unsigned int i = 0; i < sizeof(params) / sizeof(params[0]); i++) {
        unsigned long results[NMEASUREMENTS] = {0};
        for (int j = 0; j < NMEASUREMENTS; j++) {
            struct timeval t0;
            struct timeval t1;
            gettimeofday(&t0, 0);

#ifdef USE_PHS_INTERFACE
            PHS(key, sizeof(key), pwd, strlen(pwd), salt, strlen(salt),
                params[i].R, params[i].T);
#else
            lyra2(key, sizeof(key), pwd, strlen(pwd), salt, strlen(salt),
                  params[i].R, params[i].C, params[i].T);
#endif

            gettimeofday(&t1, 0);
            results[j] = (t1.tv_sec - t0.tv_sec) * 1000000 + t1.tv_usec - t0.tv_usec;
        }

#ifdef USE_PHS_INTERFACE
        printf("Parameters: R = %u, C = %u, T = %u\n",
               params[i].R, PHS_NCOLS, params[i].T);
#else
        printf("Parameters: R = %u, C = %u, T = %u\n",
               params[i].R, params[i].C, params[i].T);
#endif

        for (unsigned int k = 0; k < sizeof(key); k++) {
            printf("%.2x ", (uint8_t) key[k]);
        }
        printf("\n");

        qsort(results, NMEASUREMENTS, sizeof(results[0]), cmp);
        printf("Median time: %lu us\n", results[NMEASUREMENTS/2]);
        printf("Standard deviation: %.2f us\n",
               compute_standard_deviation(results));
        printf("\n");
    }

    return 0;
}
