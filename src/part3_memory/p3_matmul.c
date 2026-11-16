/* p3_matmul.c : matrix multiplication C = A*B for n = 512 doubles.
 * (a) all six loop orders;  (b) the ikj order tiled with a (Ti x Tj) tile
 * sweep;  (c) the waterfall: naive ijk -> ikj -> tiled -> tiled at -O3.
 * The program is compiled twice (-O2 -fno-tree-vectorize and -O3) and the
 * driver script pastes the two outputs together. */
#include "bench.h"
#ifndef n
#define n 512          /* overridden with -Dn=2048 for the out-of-L2 runs */
#endif
static double *A, *B, *C;
static int TI = 32, TJ = 32;
#define IDX(i, j) ((i) * n + (j))
static void zero(void) { memset(C, 0, sizeof(double) * n * n); }
static void mm_ijk(void *u) { zero(); for (int i = 0; i < n; i++) for (int j = 0; j < n; j++) { double s = 0; for (int k = 0; k < n; k++) s += A[IDX(i,k)] * B[IDX(k,j)]; C[IDX(i,j)] = s; } }
static void mm_ikj(void *u) { zero(); for (int i = 0; i < n; i++) for (int k = 0; k < n; k++) { double a = A[IDX(i,k)]; for (int j = 0; j < n; j++) C[IDX(i,j)] += a * B[IDX(k,j)]; } }
static void mm_jik(void *u) { zero(); for (int j = 0; j < n; j++) for (int i = 0; i < n; i++) { double s = 0; for (int k = 0; k < n; k++) s += A[IDX(i,k)] * B[IDX(k,j)]; C[IDX(i,j)] = s; } }
static void mm_jki(void *u) { zero(); for (int j = 0; j < n; j++) for (int k = 0; k < n; k++) { double b = B[IDX(k,j)]; for (int i = 0; i < n; i++) C[IDX(i,j)] += A[IDX(i,k)] * b; } }
static void mm_kij(void *u) { zero(); for (int k = 0; k < n; k++) for (int i = 0; i < n; i++) { double a = A[IDX(i,k)]; for (int j = 0; j < n; j++) C[IDX(i,j)] += a * B[IDX(k,j)]; } }
static void mm_kji(void *u) { zero(); for (int k = 0; k < n; k++) for (int j = 0; j < n; j++) { double b = B[IDX(k,j)]; for (int i = 0; i < n; i++) C[IDX(i,j)] += A[IDX(i,k)] * b; } }
/* tiled ikj: tiles of TI rows of A/C and TJ columns of B/C; k is tiled with TJ too */
static void mm_tiled(void *u) {
    zero();
    for (int ii = 0; ii < n; ii += TI) for (int kk = 0; kk < n; kk += TJ) for (int jj = 0; jj < n; jj += TJ)
        for (int i = ii; i < ii + TI; i++) for (int k = kk; k < kk + TJ; k++) { double a = A[IDX(i,k)]; for (int j = jj; j < jj + TJ; j++) C[IDX(i,j)] += a * B[IDX(k,j)]; }
}
int main(int argc, char **argv) {
    bench_init();
    A = malloc(sizeof(double) * n * n); B = malloc(sizeof(double) * n * n); C = malloc(sizeof(double) * n * n);
    for (int i = 0; i < n * n; i++) { A[i] = (i % 7) * 0.5; B[i] = (i % 5) * 0.25; }
    const char *mode = argc > 1 ? argv[1] : "orders";
    double flops = 2.0 * n * n * n;
    if (strcmp(mode, "orders") == 0) {
        struct { const char *nm; void (*f)(void *); } t[] = { {"ijk", mm_ijk}, {"ikj", mm_ikj}, {"jik", mm_jik}, {"jki", mm_jki}, {"kij", mm_kij}, {"kji", mm_kji} };
        printf("order,ms,q1,q3,gflops\n");
        for (int i = 0; i < 6; i++) { stats_t s = bench_run(t[i].f, NULL, 5); printf("%s,%.2f,%.2f,%.2f,%.3f\n", t[i].nm, s.med / 1e6, s.q1 / 1e6, s.q3 / 1e6, flops / s.med); }
    } else if (strcmp(mode, "waterfall") == 0) {
        TI = TJ = 64;
        struct { const char *nm; void (*f)(void *); } t[] = { {"ijk", mm_ijk}, {"ikj", mm_ikj}, {"tiled64", mm_tiled} };
        printf("stage,ms,q1,q3,gflops\n");
        for (int i = 0; i < 3; i++) { stats_t s = bench_run(t[i].f, NULL, 3); printf("%s,%.2f,%.2f,%.2f,%.3f\n", t[i].nm, s.med / 1e6, s.q1 / 1e6, s.q3 / 1e6, flops / s.med); fflush(stdout); }
    } else {
        int sizes[] = { 8, 16, 32, 64, 128, 256 };
        printf("ti,tj,ms,q1,q3,gflops\n");
        for (int a = 0; a < 6; a++) for (int b = 0; b < 6; b++) {
            TI = sizes[a]; TJ = sizes[b];
            stats_t s = bench_run(mm_tiled, NULL, n > 1024 ? 3 : 5);
            printf("%d,%d,%.2f,%.2f,%.2f,%.3f\n", TI, TJ, s.med / 1e6, s.q1 / 1e6, s.q3 / 1e6, flops / s.med);
            fflush(stdout);
        }
    }
    return 0;
}
