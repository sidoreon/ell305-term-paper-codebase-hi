/* p2_unroll.c : unroll-factor sweep.  A loop with a short body is unrolled
 * 1, 2, 4, 8, 16 and 32 times using "#pragma GCC unroll".  The body is a
 * multiply-add on an array element, so there is no loop-carried dependence
 * and unrolling can only reduce loop overhead and expose ILP. */
#include "bench.h"
#define N (1 << 20)
static int *a, *b;
static void unroll1(void *u)  { _Pragma("GCC unroll 1")  for (int i = 0; i < N; i++) b[i] = a[i] * 3 + 1; }
static void unroll2(void *u)  { _Pragma("GCC unroll 2")  for (int i = 0; i < N; i++) b[i] = a[i] * 3 + 1; }
static void unroll4(void *u)  { _Pragma("GCC unroll 4")  for (int i = 0; i < N; i++) b[i] = a[i] * 3 + 1; }
static void unroll8(void *u)  { _Pragma("GCC unroll 8")  for (int i = 0; i < N; i++) b[i] = a[i] * 3 + 1; }
static void unroll16(void *u) { _Pragma("GCC unroll 16") for (int i = 0; i < N; i++) b[i] = a[i] * 3 + 1; }
static void unroll32(void *u) { _Pragma("GCC unroll 32") for (int i = 0; i < N; i++) b[i] = a[i] * 3 + 1; }
int main(void) {
    bench_init();
    a = malloc(sizeof(int) * N); b = malloc(sizeof(int) * N); for (int i = 0; i < N; i++) a[i] = i;
    struct { int k; void (*f)(void *); } t[] = { {1, unroll1}, {2, unroll2}, {4, unroll4}, {8, unroll8}, {16, unroll16}, {32, unroll32} };
    printf("unroll,ns_per_elem,q1,q3\n");
    for (int i = 0; i < 6; i++) { stats_t s = bench_run(t[i].f, NULL, 21); printf("%d,%.4f,%.4f,%.4f\n", t[i].k, s.med / N, s.q1 / N, s.q3 / N); }
    return 0;
}
