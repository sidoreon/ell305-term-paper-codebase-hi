/* p1_mulconst.c : multiplication by a constant k, for k = 2..32.  The
 * "const" version lets the compiler pick shift/add sequences (or a plain
 * multiply); the "runtime" version forces a real mul.  A dependent chain is
 * used so that the latency, not the throughput, is measured. */
#include "bench.h"
#define N 50000000L
static volatile long sink; static volatile long rt_k;
#define CHAIN_CONST(K) static void mul_c##K(void *u){ long x = 1; for (long i = 0; i < N; i++) { x = x * K + i; __asm__ volatile("" : "+r"(x)); } sink = x; }
CHAIN_CONST(3) CHAIN_CONST(5) CHAIN_CONST(6) CHAIN_CONST(7) CHAIN_CONST(9) CHAIN_CONST(10)
CHAIN_CONST(11) CHAIN_CONST(12) CHAIN_CONST(13) CHAIN_CONST(15) CHAIN_CONST(17) CHAIN_CONST(25)
static void mul_rt(void *u) { long k = rt_k; long x = 1; for (long i = 0; i < N; i++) { x = x * k + i; __asm__ volatile("" : "+r"(x)); } sink = x; }
int main(void) {
    bench_init();
    struct { int k; void (*fn)(void *); } t[] = { {3,mul_c3},{5,mul_c5},{6,mul_c6},{7,mul_c7},{9,mul_c9},{10,mul_c10},
        {11,mul_c11},{12,mul_c12},{13,mul_c13},{15,mul_c15},{17,mul_c17},{25,mul_c25} };
    printf("k,variant,ns_per_op,q1,q3\n");
    for (unsigned i = 0; i < sizeof t / sizeof *t; i++) {
        stats_t s = bench_run(t[i].fn, NULL, 9);
        printf("%d,const,%.4f,%.4f,%.4f\n", t[i].k, s.med / N, s.q1 / N, s.q3 / N);
        rt_k = t[i].k;
        s = bench_run(mul_rt, NULL, 9);
        printf("%d,runtime,%.4f,%.4f,%.4f\n", t[i].k, s.med / N, s.q1 / N, s.q3 / N);
    }
    return 0;
}
