/* cal_memory.c : pointer-chase latency staircase.  A random cyclic permutation
 * of cache lines is followed for a fixed number of hops; the working set is
 * swept from 4 KiB to 256 MiB.  Output: ns per load for each size. */
#include "bench.h"
#define LINE 64
#define HOPS 50000000L
static volatile void *chase_next;
static void chase(void *p) {
    void **q = (void **)p;
    for (long i = 0; i < HOPS; i++) { q = (void **)*q; __asm__ volatile("" : "+r"(q)); }
    chase_next = q;
}
int main(void) {
    bench_init();
    printf("bytes,ns_per_load,q1,q3\n");
    for (long bytes = 4096; bytes <= (256L << 20); bytes *= 2) {
        long n = bytes / LINE;
        char *buf = (char *)malloc(bytes + LINE);
        long *perm = (long *)malloc(sizeof(long) * n);
        for (long i = 0; i < n; i++) perm[i] = i;
        srand(42);
        for (long i = n - 1; i > 0; i--) { long j = rand() % (i + 1); long t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
        for (long i = 0; i < n; i++)               /* single cycle through all lines */
            *(void **)(buf + perm[i] * LINE) = buf + perm[(i + 1) % n] * LINE;
        int reps = bytes >= (64L << 20) ? 3 : 5;
        stats_t s = bench_run(chase, buf + perm[0] * LINE, reps);
        printf("%ld,%.3f,%.3f,%.3f\n", bytes, s.med / HOPS, s.q1 / HOPS, s.q3 / HOPS);
        fflush(stdout);
        free(buf); free(perm);
    }
    return 0;
}
