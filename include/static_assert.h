#pragma once

#if __STDC_VERSION__ == 201112L
#include <assert.h>
#define STATIC_ASSERT(expr, name) _Static_assert((expr), #name)
#else
#define STATIC_ASSERT(expr, name) \
    typedef char static_assert_ ## name [(expr) ? 1 : -1]
#endif
