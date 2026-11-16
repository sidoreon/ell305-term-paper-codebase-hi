/* p2_inline.c : function-call overhead.  A tiny body applied to every array
 * element, once as an out-of-line call (noinline), once inlined, and once
 * through a function pointer.  The elements are independent, so the loop is
 * throughput-bound and the call/return instructions cannot hide behind a
 * data dependence chain.  Vectorisation is disabled for all three. */
#include "bench.h"
#define N (1 << 22)
static int *a; static volatile long lsink;
__attribute__((noinline)) static int f_noinline(int x) { return x * 3 + 1; }
static inline int f_inline(int x) { return x * 3 + 1; }
static int (*volatile fptr)(int) = f_noinline;
static void call_noinline(void *u) { long s = 0; for (int i = 0; i < N; i++) s += f_noinline(a[i]); lsink = s; }
static void call_inline(void *u)   { long s = 0; for (int i = 0; i < N; i++) s += f_inline(a[i]); lsink = s; }
static void call_pointer(void *u)  { long s = 0; int (*f)(int) = fptr; for (int i = 0; i < N; i++) s += f(a[i]); lsink = s; }
int main(void) {
    bench_init();
    a = malloc(sizeof(int) * N); for (int i = 0; i < N; i++) a[i] = i & 1023;
    printf("variant,ns_per_call,q1,q3\n");
    stats_t s = bench_run(call_noinline, NULL, 15); printf("noinline,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(call_inline, NULL, 15);  printf("inline,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(call_pointer, NULL, 15); printf("pointer,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
