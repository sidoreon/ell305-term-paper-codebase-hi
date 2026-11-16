/* p4_ladder.c : the legality ladder.  Five loops that differ by one
 * dependence each.  Compile with -O2 -fopt-info-vec-all to see which rungs
 * the vectoriser accepts; run to time them. */
#include "bench.h"
#define N (1 << 20)
/* The arrays are separate allocations, declared restrict so that the only
 * thing standing between each rung and the vectoriser is its dependence. */
static float *__restrict a; static float *__restrict b; static float *__restrict c; static int *__restrict idx;
static volatile float fsink;
#define R float *__restrict
/* rung 0: no loop-carried dependence at all */
__attribute__((noinline)) static void rung0_k(R a, R b, R c) { for (int i = 0; i < N; i++) c[i] = a[i] + b[i]; }
/* rung 1: a[i] depends on a[i-1]: true dependence, distance 1 -> not vectorisable */
__attribute__((noinline)) static void rung1_k(R a, R b, R c) { for (int i = 1; i < N; i++) a[i] = a[i - 1] + b[i]; }
/* rung 2: a[i] depends on a[i-8]: true dependence, distance 8 -> vectorisable with <= 8 lanes */
__attribute__((noinline)) static void rung2_k(R a, R b, R c) { for (int i = 8; i < N; i++) a[i] = a[i - 8] + b[i]; }
/* rung 3: a[i] written, a[i+1] read: anti-dependence, distance 1 -> vectorisable */
__attribute__((noinline)) static void rung3_k(R a, R b, R c) { for (int i = 0; i < N - 1; i++) a[i] = a[i + 1] + b[i]; }
/* rung 4: a float reduction: legal only with reassociation (-ffast-math) */
__attribute__((noinline)) static float rung4_k(R a, R b, R c) { float s = 0; for (int i = 0; i < N; i++) s += a[i]; return s; }
/* rung 5: an indirect store c[idx[i]]: unknown distance -> not vectorisable */
__attribute__((noinline)) static void rung5_k(R a, R b, R c, int *__restrict idx) { for (int i = 0; i < N; i++) c[idx[i]] = a[i] + b[i]; }
static void rung0(void *u) { rung0_k(a, b, c); }
static void rung1(void *u) { rung1_k(a, b, c); }
static void rung2(void *u) { rung2_k(a, b, c); }
static void rung3(void *u) { rung3_k(a, b, c); }
static void rung4(void *u) { fsink = rung4_k(a, b, c); }
static void rung5(void *u) { rung5_k(a, b, c, idx); }
int main(void) {
    bench_init();
    a = malloc(4 * N); b = malloc(4 * N); c = malloc(4 * N); idx = malloc(4 * N);
    for (int i = 0; i < N; i++) { a[i] = 1.0f; b[i] = 0.5f; c[i] = 0; idx[i] = i; }
    struct { const char *nm; void (*f)(void *); } t[] = { {"rung0_none", rung0}, {"rung1_true_d1", rung1}, {"rung2_true_d8", rung2}, {"rung3_anti_d1", rung3}, {"rung4_reduction", rung4}, {"rung5_indirect", rung5} };
    printf("rung,ns_per_elem,q1,q3\n");
    for (int i = 0; i < 6; i++) { stats_t s = bench_run(t[i].f, NULL, 15); printf("%s,%.4f,%.4f,%.4f\n", t[i].nm, s.med / N, s.q1 / N, s.q3 / N); }
    return 0;
}
