/* p4_aos_simd.c : AoS versus SoA for SIMD.  Scaling the x field of a
 * particle array: with AoS the x values are 32 bytes apart, so a vector
 * load would need a gather; with SoA they are contiguous. */
#include "bench.h"
#define N (1 << 16)
#define REPS 64
typedef struct { float x, y, z, w, vx, vy, vz, vw; } particle_t;
static particle_t *aos; static float *sx, *svx;
static void aos_kern(void *u) { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) aos[i].x += 0.5f * aos[i].vx; }
static void soa_kern(void *u) { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) sx[i] += 0.5f * svx[i]; }
int main(void) {
    bench_init();
    aos = malloc(sizeof(particle_t) * N); sx = malloc(4 * N); svx = malloc(4 * N);
    for (int i = 0; i < N; i++) { aos[i].x = sx[i] = 1; aos[i].vx = svx[i] = 1; }
    printf("layout,ns_per_elem,q1,q3\n");
    stats_t s = bench_run(aos_kern, NULL, 15); printf("AoS,%.4f,%.4f,%.4f\n", s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS));
    s = bench_run(soa_kern, NULL, 15); printf("SoA,%.4f,%.4f,%.4f\n", s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS));
    return 0;
}
