#pragma once

#ifdef TIMEIT
#include <stdio.h>
#include <sys/time.h>

#define TIMEIT_MAXTIMERS 100

static int timeit_top_;
static struct {
    struct timeval t0, t1;
} timeit_timers_[TIMEIT_MAXTIMERS];

#define TIMEIT_BEGIN \
    gettimeofday(&timeit_timers_[timeit_top_++].t0, 0);

#define TIMEIT_END(label)                                    \
    gettimeofday(&timeit_timers_[--timeit_top_].t1, 0);      \
    fprintf(stderr, "timeit(" label ") = %lu us\n",          \
        (timeit_timers_[timeit_top_].t1.tv_sec -             \
         timeit_timers_[timeit_top_].t0.tv_sec) * 1000000 +  \
        (timeit_timers_[timeit_top_].t1.tv_usec -            \
         timeit_timers_[timeit_top_].t0.tv_usec));
#endif
