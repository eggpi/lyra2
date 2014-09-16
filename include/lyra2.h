#pragma once

#include <stdlib.h>
#include <stdint.h>

#define PHS_NCOLS 64

int lyra2(char *key, uint32_t keylen, const char *pwd, uint32_t pwdlen, const char *salt, uint32_t saltlen, uint32_t R, uint32_t C, uint32_t T);
int PHS(void *out, size_t outlen, const void *in, size_t inlen, const void *salt, size_t saltlen, unsigned int t_cost, unsigned int m_cost);
