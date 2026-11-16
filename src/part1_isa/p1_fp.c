/* p1_fp.c : floating point is not associative.  (a) bit-level differences of
 * left- and right-associated sums and of a reciprocal-multiply division;
 * (b) timing of a float division loop vs. a reciprocal multiply loop. */
#include "bench.h"
#include <math.h>
#define N (1 << 22)
static float *x, *y; static volatile float fsink;
static void div_loop(void *u) { float d = 3.0f + (fsink == 1.0f); for (int i = 0; i < N; i++) y[i] = x[i] / d; }
static void recip_loop(void *u){ float d = 3.0f + (fsink == 1.0f); float r = 1.0f / d; for (int i = 0; i < N; i++) y[i] = x[i] * r; }
int main(void) {
    bench_init();
    /* (a) associativity */
    double a = 1e16, b = -1e16, c = 1.0;
    printf("assoc,(a+b)+c=%.17g,a+(b+c)=%.17g\n", (a + b) + c, a + (b + c));
    float s1 = 0.0f, s2 = 0.0f;                     /* sum of 0.1f a million times, two orders */
    for (int i = 0; i < 1000000; i++) s1 += 0.1f;
    float part[8] = {0};
    for (int i = 0; i < 1000000; i++) part[i & 7] += 0.1f;
    for (int i = 0; i < 8; i++) s2 += part[i];
    printf("sum1e6_0.1f,serial=%.9g,8lanes=%.9g,exact=100000\n", s1, s2);
    x = malloc(sizeof(float) * N); y = malloc(sizeof(float) * N);
    srand(3); for (int i = 0; i < N; i++) x[i] = (float)rand() / 7.0f;
    /* how many elements differ between x/d and x*(1/d)? */
    long diff = 0; float maxulp = 0;
    for (int i = 0; i < N; i++) { float q = x[i] / 3.0f, r = x[i] * (1.0f / 3.0f); if (q != r) { diff++; float u = fabsf(q - r) / (fabsf(q) * 1.1920929e-7f); if (u > maxulp) maxulp = u; } }
    printf("recip_mismatch,%ld,of,%d,max_ulp,%.2f\n", diff, N, maxulp);
    /* (b) timing */
    stats_t s = bench_run(div_loop, NULL, 15);
    printf("timing,div,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(recip_loop, NULL, 15);
    printf("timing,recip,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
