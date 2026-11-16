/* p4_alias.c : aliasing blocks vectorisation.  The same loop with plain
 * pointers, with __restrict, and with "#pragma omp simd" (needs -fopenmp-simd). */
#include "bench.h"
#define N (1 << 20)
static float *x, *y, *z;
__attribute__((noinline)) static void plain(float *a, float *b, float *c, int n) { for (int i = 0; i < n; i++) c[i] = a[i] * 2.0f + b[i]; }
__attribute__((noinline)) static void restricted(float *__restrict a, float *__restrict b, float *__restrict c, int n) { for (int i = 0; i < n; i++) c[i] = a[i] * 2.0f + b[i]; }
__attribute__((noinline)) static void omp_simd(float *a, float *b, float *c, int n) {
    #pragma omp simd
    for (int i = 0; i < n; i++) c[i] = a[i] * 2.0f + b[i];
}
static void r_plain(void *u) { plain(x, y, z, N); }
static void r_restr(void *u) { restricted(x, y, z, N); }
static void r_omp(void *u)   { omp_simd(x, y, z, N); }
int main(void) {
    bench_init();
    x = malloc(4 * N); y = malloc(4 * N); z = malloc(4 * N); for (int i = 0; i < N; i++) { x[i] = i; y[i] = 1; }
    printf("variant,ns_per_elem,q1,q3\n");
    stats_t s = bench_run(r_plain, NULL, 21); printf("plain,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(r_restr, NULL, 21); printf("restrict,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(r_omp, NULL, 21);   printf("omp_simd,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
