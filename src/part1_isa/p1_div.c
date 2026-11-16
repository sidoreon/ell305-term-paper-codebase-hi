/* p1_div.c : division by a constant.  Three variants of "divide an array by 7":
 *  (a) divisor known at compile time     -> compiler emits multiply-shift
 *  (b) divisor read at run time          -> compiler must emit sdiv/udiv
 *  (c) unsigned versions of (a) and (b)  -> no sign correction needed
 * Output: ns per element for each variant. */
#include "bench.h"
#define N (1 << 22)
static int *a; static int *b; static unsigned *ua; static unsigned *ub;
static volatile int rt_div = 7;   /* run-time divisor the compiler cannot see through */

static void div_const_signed(void *u)   { for (int i = 0; i < N; i++) b[i] = a[i] / 7; }
static void div_runtime_signed(void *u) { int d = rt_div; for (int i = 0; i < N; i++) b[i] = a[i] / d; }
static void div_const_unsigned(void *u) { for (int i = 0; i < N; i++) ub[i] = ua[i] / 7u; }
static void div_runtime_unsigned(void *u){ unsigned d = rt_div; for (int i = 0; i < N; i++) ub[i] = ua[i] / d; }
static void div_const_pow2_signed(void *u)  { for (int i = 0; i < N; i++) b[i] = a[i] / 8; }
static void div_const_pow2_unsigned(void *u){ for (int i = 0; i < N; i++) ub[i] = ua[i] / 8u; }
/* dependent chains: each quotient feeds the next division, exposing latency */
#define CH 20000000L
static volatile long lsink;
static void chain_const(void *u)   { long x = 0x7fffffffffffL; for (long i = 0; i < CH; i++) { x = x / 7 + 0x7fffffffffffL; __asm__ volatile("" : "+r"(x)); } lsink = x; }
static void chain_runtime(void *u) { long d = rt_div; long x = 0x7fffffffffffL; for (long i = 0; i < CH; i++) { x = x / d + 0x7fffffffffffL; __asm__ volatile("" : "+r"(x)); } lsink = x; }
static void mod_const_signed(void *u)   { for (int i = 0; i < N; i++) b[i] = a[i] % 7; }
static void divisible_by_7(void *u)     { for (int i = 0; i < N; i++) b[i] = (a[i] % 7 == 0); }

int main(void) {
    bench_init();
    a = malloc(sizeof(int) * N); b = malloc(sizeof(int) * N);
    ua = malloc(sizeof(unsigned) * N); ub = malloc(sizeof(unsigned) * N);
    srand(1);
    for (int i = 0; i < N; i++) { a[i] = rand() - RAND_MAX / 2; ua[i] = (unsigned)rand(); }
    struct { const char *name; void (*fn)(void *); } t[] = {
        {"const_signed", div_const_signed}, {"runtime_signed", div_runtime_signed},
        {"const_unsigned", div_const_unsigned}, {"runtime_unsigned", div_runtime_unsigned},
        {"pow2_signed", div_const_pow2_signed}, {"pow2_unsigned", div_const_pow2_unsigned},
        {"mod7_signed", mod_const_signed}, {"divisible7", divisible_by_7} };
    printf("variant,ns_per_elem,q1,q3\n");
    for (unsigned k = 0; k < sizeof t / sizeof *t; k++) {
        stats_t s = bench_run(t[k].fn, NULL, 15);
        printf("%s,%.4f,%.4f,%.4f\n", t[k].name, s.med / N, s.q1 / N, s.q3 / N);
    }
    stats_t s = bench_run(chain_const, NULL, 9);
    printf("chain_const,%.4f,%.4f,%.4f\n", s.med / CH, s.q1 / CH, s.q3 / CH);
    s = bench_run(chain_runtime, NULL, 9);
    printf("chain_runtime,%.4f,%.4f,%.4f\n", s.med / CH, s.q1 / CH, s.q3 / CH);
    return 0;
}
