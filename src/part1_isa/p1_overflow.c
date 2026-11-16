/* p1_overflow.c : signed overflow as a licence.  The same loop with an int
 * and an unsigned counter, indexing a[i + 1] on a 64-bit machine.  With int
 * the compiler may assume i + 1 never wraps and turns the index into a
 * pointer bump; with unsigned it must honour wrap-around at 2^32 and
 * recompute the 32-bit index every iteration. */
#include "bench.h"
#define N (1 << 22)
static int *a; static volatile long lsink;
__attribute__((noinline)) static long sum_int(int *p, long n)      { long s = 0; for (int i = 0; i < n - 1; i++) s += p[i + 1]; return s; }
__attribute__((noinline)) static long sum_unsigned(int *p, long n) { long s = 0; for (unsigned i = 0; i < n - 1; i++) s += p[i + 1]; return s; }
static void r_int(void *u) { lsink = sum_int(a, N); }
static void r_uns(void *u) { lsink = sum_unsigned(a, N); }
int main(void) {
    bench_init();
    a = malloc(4 * N); for (int i = 0; i < N; i++) a[i] = i & 15;
    printf("counter,ns_per_elem,q1,q3\n");
    stats_t s = bench_run(r_int, NULL, 15); printf("int,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(r_uns, NULL, 15); printf("unsigned,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
