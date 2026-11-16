/* bench.h : tiny timing harness shared by every experiment in the paper.
 * Steady clock, N repetitions, median and spread, optimisation barriers. */
#ifndef BENCH_H
#define BENCH_H
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h>
#ifdef __APPLE__
#include <pthread.h>
#include <sys/qos.h>
#endif

static inline double now_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC_RAW, &ts);
    return ts.tv_sec * 1e9 + ts.tv_nsec;
}
/* Ask the scheduler for a performance core (Apple silicon). */
static inline void bench_init(void) {
#ifdef __APPLE__
    pthread_set_qos_class_self_np(QOS_CLASS_USER_INTERACTIVE, 0);
#endif
}
/* Barriers: DoNotOptimize / ClobberMemory in the Google Benchmark style. */
#define KEEP(x)   __asm__ volatile("" : : "g"(x) : "memory")
#define CLOBBER() __asm__ volatile("" : : : "memory")

static int cmp_dbl(const void *a, const void *b) {
    double x = *(const double *)a, y = *(const double *)b;
    return (x > y) - (x < y);
}
/* Run fn(arg) reps times; report median, min, and inter-quartile spread in ns. */
typedef struct { double med, min, q1, q3; } stats_t;
static inline stats_t bench_run(void (*fn)(void *), void *arg, int reps) {
    double *t = (double *)malloc(sizeof(double) * reps);
    for (int r = 0; r < reps; r++) {
        double t0 = now_ns(); CLOBBER(); fn(arg); CLOBBER(); double t1 = now_ns();
        t[r] = t1 - t0;
    }
    qsort(t, reps, sizeof(double), cmp_dbl);
    stats_t s = { t[reps / 2], t[0], t[reps / 4], t[(3 * reps) / 4] };
    free(t);
    return s;
}
#endif
