/* p4_align.c : alignment.  The vectorised saxpy runs on buffers whose start
 * is offset by 0, 4, 8 and 12 bytes from a 16-byte boundary, and on buffers
 * offset by 60 bytes so that every vector straddles a 128-byte line. */
#include "bench.h"
#define N (1 << 16)
#define REPS 64
static char *raw_x, *raw_y; static float *x, *y;
static void kern(void *u) { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) y[i] = 2.0f * x[i] + y[i]; }
int main(void) {
    bench_init();
    raw_x = aligned_alloc(128, 4 * N + 256); raw_y = aligned_alloc(128, 4 * N + 256);
    x = (float *)raw_x; y = (float *)raw_y; for (int i = 0; i < N; i++) { x[i] = 1; y[i] = 1; } bench_run(kern, NULL, 5);   /* warm up */
    printf("offset_bytes,ns_per_elem,q1,q3\n");
    int offs[] = { 124, 60, 12, 8, 4, 0, 124, 0 };
    for (int k = 0; k < 8; k++) {
        x = (float *)(raw_x + offs[k]); y = (float *)(raw_y + offs[k]);
        for (int i = 0; i < N; i++) { x[i] = 1; y[i] = 1; }
        stats_t s = bench_run(kern, NULL, 15); printf("%d,%.4f,%.4f,%.4f\n", offs[k], s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS));
    }
    return 0;
}
