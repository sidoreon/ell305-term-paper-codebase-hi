/* p3_transpose.c : matrix transpose, naive and blocked, n = 4096 doubles
 * (128 MiB per matrix).  The naive version reads rows and writes columns,
 * so one of the two streams is strided by a whole row; the blocked version
 * transposes B x B tiles that fit in L1. */
#include "bench.h"
#define n 4096
static double *A, *B; static int BS = 32;
static void naive(void *u)   { for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) B[j * n + i] = A[i * n + j]; }
static void blocked(void *u) { for (int ii = 0; ii < n; ii += BS) for (int jj = 0; jj < n; jj += BS) for (int i = ii; i < ii + BS; i++) for (int j = jj; j < jj + BS; j++) B[j * n + i] = A[i * n + j]; }
int main(void) {
    bench_init();
    A = malloc(8L * n * n); B = malloc(8L * n * n); for (long i = 0; i < (long)n * n; i++) A[i] = i;
    printf("variant,block,ms,q1,q3\n");
    stats_t s = bench_run(naive, NULL, 5); printf("naive,0,%.2f,%.2f,%.2f\n", s.med / 1e6, s.q1 / 1e6, s.q3 / 1e6);
    int bs[] = { 8, 16, 32, 64, 128 };
    for (int k = 0; k < 5; k++) { BS = bs[k]; s = bench_run(blocked, NULL, 5); printf("blocked,%d,%.2f,%.2f,%.2f\n", BS, s.med / 1e6, s.q1 / 1e6, s.q3 / 1e6); fflush(stdout); }
    return 0;
}
