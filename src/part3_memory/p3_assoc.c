/* p3_assoc.c : measuring associativity and power-of-two aliasing without
 * performance counters.  k cache lines spaced exactly one "set stride" apart
 * (L1: 128 KiB / 8 ways = 16 KiB) all map to the same L1 set.  A pointer
 * chase that cycles through those k lines stays at L1 latency while
 * k <= ways and jumps to L2 latency when k > ways.  The same k lines spaced
 * by set stride + 128 B (Rivera-Tseng inter-array padding) fall in different
 * sets and stay at L1 latency for any k that fits the cache. */
#include "bench.h"
#define HOPS 20000000L
/* cache geometry: defaults are the Apple M5 values; override with -D for another machine
 * (Intel i5-12500H P-core: -DSETSTRIDE=4096 -DLINE=64, since 48 KiB / 12 ways = 4 KiB) */
#ifndef SETSTRIDE
#define SETSTRIDE 16384
#endif
#ifndef LINE
#define LINE 128
#endif
static char *buf; static int K; static long STRIDE; static volatile void *sink;
static void chase(void *p) { void **q = (void **)p; for (long i = 0; i < HOPS; i++) { q = (void **)*q; __asm__ volatile("" : "+r"(q)); } sink = q; }
int main(void) {
    bench_init();
    buf = malloc(64L << 20); memset(buf, 0, 64L << 20);
    printf("k,stride,ns_per_load,q1,q3\n");
    long strides[] = { SETSTRIDE, SETSTRIDE + LINE, SETSTRIDE / 4 };
    for (int si = 0; si < 3; si++) {
        STRIDE = strides[si];
        for (K = 1; K <= 24; K++) {
            for (int k = 0; k < K; k++)                    /* cyclic chain through the K lines */
                *(void **)(buf + k * STRIDE) = buf + ((k + 1) % K) * STRIDE;
            stats_t s = bench_run(chase, buf, 7);
            printf("%d,%ld,%.4f,%.4f,%.4f\n", K, STRIDE, s.med / HOPS, s.q1 / HOPS, s.q3 / HOPS);
        }
    }
    return 0;
}
