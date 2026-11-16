/* p2_chains.c : dependency-chain length.  A fixed budget of 64 M integer
 * multiply-adds is executed as k independent chains of 64M/k operations
 * each, k = 1 .. 12, all interleaved in one loop.  Time against k shows the
 * transition from latency-bound to throughput-bound. */
#include "bench.h"
#define TOTAL 67108864L
static volatile long lsink; static int K;
#define STEP(x) x = x * 3 + 1
static void run(void *u) {
    long x[16] = {1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16};
    long iters = TOTAL / K;
    switch (K) {
    case 1:  for (long i = 0; i < iters; i++) { STEP(x[0]); __asm__ volatile("" : "+r"(x[0])); } break;
    case 2:  for (long i = 0; i < iters; i++) { STEP(x[0]); STEP(x[1]); __asm__ volatile("" : "+r"(x[0]), "+r"(x[1])); } break;
    case 3:  for (long i = 0; i < iters; i++) { STEP(x[0]); STEP(x[1]); STEP(x[2]); __asm__ volatile("" : "+r"(x[0]), "+r"(x[1]), "+r"(x[2])); } break;
    case 4:  for (long i = 0; i < iters; i++) { STEP(x[0]); STEP(x[1]); STEP(x[2]); STEP(x[3]); __asm__ volatile("" : "+r"(x[0]), "+r"(x[1]), "+r"(x[2]), "+r"(x[3])); } break;
    case 6:  for (long i = 0; i < iters; i++) { STEP(x[0]); STEP(x[1]); STEP(x[2]); STEP(x[3]); STEP(x[4]); STEP(x[5]); __asm__ volatile("" : "+r"(x[0]), "+r"(x[1]), "+r"(x[2]), "+r"(x[3]), "+r"(x[4]), "+r"(x[5])); } break;
    case 8:  for (long i = 0; i < iters; i++) { STEP(x[0]); STEP(x[1]); STEP(x[2]); STEP(x[3]); STEP(x[4]); STEP(x[5]); STEP(x[6]); STEP(x[7]); __asm__ volatile("" : "+r"(x[0]), "+r"(x[1]), "+r"(x[2]), "+r"(x[3]), "+r"(x[4]), "+r"(x[5]), "+r"(x[6]), "+r"(x[7])); } break;
    case 12: for (long i = 0; i < iters; i++) { STEP(x[0]); STEP(x[1]); STEP(x[2]); STEP(x[3]); STEP(x[4]); STEP(x[5]); STEP(x[6]); STEP(x[7]); STEP(x[8]); STEP(x[9]); STEP(x[10]); STEP(x[11]); __asm__ volatile("" : "+r"(x[0]), "+r"(x[1]), "+r"(x[2]), "+r"(x[3]), "+r"(x[4]), "+r"(x[5]), "+r"(x[6]), "+r"(x[7]), "+r"(x[8]), "+r"(x[9]), "+r"(x[10]), "+r"(x[11])); } break;
    }
    long s = 0; for (int k = 0; k < 16; k++) s += x[k]; lsink = s;
}
int main(void) {
    bench_init();
    int ks[] = { 1, 2, 3, 4, 6, 8, 12 };
    printf("chains,ns_per_op,q1,q3\n");
    for (int i = 0; i < 7; i++) { K = ks[i]; stats_t s = bench_run(run, NULL, 9); printf("%d,%.4f,%.4f,%.4f\n", K, s.med / TOTAL, s.q1 / TOTAL, s.q3 / TOTAL); }
    return 0;
}
