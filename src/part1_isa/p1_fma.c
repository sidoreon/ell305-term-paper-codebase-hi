/* p1_fma.c : FMA contraction.  a*b+c computed with separate multiply and add
 * (two roundings) and as a fused multiply-add (one rounding).  The program is
 * compiled twice, with -ffp-contract=off and -ffp-contract=fast, and prints
 * the time per element, the mean error against a double reference, and the
 * results themselves to a file so that the two builds can be compared. */
#include "bench.h"
#include <math.h>
#define N (1 << 20)
static float *a, *b, *c, *d; static double *ref;
static void kern(void *u) { for (int i = 0; i < N; i++) d[i] = a[i] * b[i] + c[i]; }
int main(int argc, char **argv) {
    bench_init();
    a = malloc(4 * N); b = malloc(4 * N); c = malloc(4 * N); d = malloc(4 * N); ref = malloc(8 * N);
    srand(6); for (int i = 0; i < N; i++) { a[i] = (float)rand() / RAND_MAX; b[i] = (float)rand() / RAND_MAX; c[i] = -(float)rand() / RAND_MAX; ref[i] = (double)a[i] * b[i] + c[i]; }
    stats_t s = bench_run(kern, NULL, 21);
    double err = 0; for (int i = 0; i < N; i++) err += fabs(d[i] - ref[i]);
    FILE *out = fopen(argc > 1 ? argv[1] : "fma_out.bin", "wb"); fwrite(d, 4, N, out); fclose(out);
    printf("ns_per_elem,%.4f,%.4f,%.4f\nmean_abs_error_vs_double,%.3e\n", s.med / N, s.q1 / N, s.q3 / N, err / N);
    return 0;
}
