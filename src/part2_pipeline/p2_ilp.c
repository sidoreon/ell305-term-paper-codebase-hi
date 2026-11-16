/* p2_ilp.c : instruction-level parallelism.  A sum over an array with 1, 2, 4
 * and 8 independent accumulators.  With one accumulator the loop is bound by
 * the latency of the add; with more, by the number of adders and the load
 * bandwidth.  Vectorisation is disabled so that only scalar ILP is measured. */
#include "bench.h"
#define N 8192           /* 64 KiB of doubles: stays in L1 so only the adders are measured */
#define REPS 256
static double *a; static volatile double dsink;
static void acc1(void *u) { double s = 0; for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) s += a[i]; dsink = s; }
static void acc2(void *u) { double s0 = 0, s1 = 0; for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i += 2) { s0 += a[i]; s1 += a[i + 1]; } dsink = s0 + s1; }
static void acc4(void *u) { double s0 = 0, s1 = 0, s2 = 0, s3 = 0; for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i += 4) { s0 += a[i]; s1 += a[i + 1]; s2 += a[i + 2]; s3 += a[i + 3]; } dsink = (s0 + s1) + (s2 + s3); }
static void acc8(void *u) { double s0 = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0, s7 = 0;
    for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i += 8) { s0 += a[i]; s1 += a[i + 1]; s2 += a[i + 2]; s3 += a[i + 3]; s4 += a[i + 4]; s5 += a[i + 5]; s6 += a[i + 6]; s7 += a[i + 7]; }
    dsink = ((s0 + s1) + (s2 + s3)) + ((s4 + s5) + (s6 + s7)); }
static void acc16(void *u) { double s0 = 0, s1 = 0, s2 = 0, s3 = 0, s4 = 0, s5 = 0, s6 = 0, s7 = 0, t0 = 0, t1 = 0, t2 = 0, t3 = 0, t4 = 0, t5 = 0, t6 = 0, t7 = 0;
    for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i += 16) { s0 += a[i]; s1 += a[i + 1]; s2 += a[i + 2]; s3 += a[i + 3]; s4 += a[i + 4]; s5 += a[i + 5]; s6 += a[i + 6]; s7 += a[i + 7];
        t0 += a[i + 8]; t1 += a[i + 9]; t2 += a[i + 10]; t3 += a[i + 11]; t4 += a[i + 12]; t5 += a[i + 13]; t6 += a[i + 14]; t7 += a[i + 15]; }
    dsink = (((s0 + s1) + (s2 + s3)) + ((s4 + s5) + (s6 + s7))) + (((t0 + t1) + (t2 + t3)) + ((t4 + t5) + (t6 + t7))); }
int main(void) {
    bench_init();
    a = malloc(sizeof(double) * N); for (int i = 0; i < N; i++) a[i] = i * 0.5;
    struct { int k; void (*f)(void *); } t[] = { {1, acc1}, {2, acc2}, {4, acc4}, {8, acc8}, {16, acc16} };
    printf("accumulators,ns_per_elem,q1,q3\n");
    for (int i = 0; i < 5; i++) { stats_t s = bench_run(t[i].f, NULL, 21); printf("%d,%.4f,%.4f,%.4f\n", t[i].k, s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS)); }
    return 0;
}
