/* p3_fusion.c : loop fusion and fission.  (a) Two loops over the same arrays
 * fused into one: the second pass over a[] and b[] is saved.  (b) One loop
 * that touches eight large arrays, split into two loops of four: each loop
 * then has fewer concurrent streams for the prefetcher and the cache. */
#include "bench.h"
#define N (1 << 23)
static double *a, *b, *c, *d; static double *s[8]; static volatile double dsink;
static void fissioned(void *u) { for (int i = 0; i < N; i++) c[i] = a[i] + b[i]; for (int i = 0; i < N; i++) d[i] = a[i] * b[i]; }
static void fused(void *u)     { for (int i = 0; i < N; i++) { c[i] = a[i] + b[i]; d[i] = a[i] * b[i]; } }
static void eight_streams(void *u) { double t = 0; for (int i = 0; i < N; i++) t += s[0][i] + s[1][i] + s[2][i] + s[3][i] + s[4][i] + s[5][i] + s[6][i] + s[7][i]; dsink = t; }
static void four_plus_four(void *u) { double t = 0; for (int i = 0; i < N; i++) t += s[0][i] + s[1][i] + s[2][i] + s[3][i]; for (int i = 0; i < N; i++) t += s[4][i] + s[5][i] + s[6][i] + s[7][i]; dsink = t; }
int main(void) {
    bench_init();
    a = malloc(8 * N); b = malloc(8 * N); c = malloc(8 * N); d = malloc(8 * N);
    for (int k = 0; k < 8; k++) { s[k] = malloc(8 * N); for (int i = 0; i < N; i++) s[k][i] = 1.0; }
    for (int i = 0; i < N; i++) { a[i] = i; b[i] = 2; }
    printf("variant,ns_per_elem,q1,q3\n"); stats_t t;
    t = bench_run(fissioned, NULL, 9);     printf("two_loops,%.4f,%.4f,%.4f\n", t.med / N, t.q1 / N, t.q3 / N);
    t = bench_run(fused, NULL, 9);         printf("fused,%.4f,%.4f,%.4f\n", t.med / N, t.q1 / N, t.q3 / N);
    t = bench_run(eight_streams, NULL, 9); printf("eight_streams,%.4f,%.4f,%.4f\n", t.med / N, t.q1 / N, t.q3 / N);
    t = bench_run(four_plus_four, NULL, 9);printf("four_plus_four,%.4f,%.4f,%.4f\n", t.med / N, t.q1 / N, t.q3 / N);
    return 0;
}
