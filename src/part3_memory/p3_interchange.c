/* p3_interchange.c : loop interchange.  Summing a row-major N x N matrix by
 * rows (inner loop walks contiguous memory) and by columns (inner loop
 * strides by one row).  Also the AoS vs SoA hot-field loop. */
#include "bench.h"
#define N 2048
static double *m; static volatile double dsink;
static void by_rows(void *u) { double s = 0; for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) s += m[i * N + j]; dsink = s; }
static void by_cols(void *u) { double s = 0; for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) s += m[i * N + j]; dsink = s; }
/* array of structures: 64-byte particles, we only need the mass field */
typedef struct { double x, y, z, vx, vy, vz, mass, charge; } particle_t;
typedef struct { double *x, *y, *z, *vx, *vy, *vz, *mass, *charge; } particles_soa_t;
#define P (1 << 20)
static particle_t *aos; static particles_soa_t soa;
static void sum_aos(void *u) { double s = 0; for (int i = 0; i < P; i++) s += aos[i].mass; dsink = s; }
static void sum_soa(void *u) { double s = 0; for (int i = 0; i < P; i++) s += soa.mass[i]; dsink = s; }
/* all-fields update, where AoS and SoA touch the same number of bytes */
static void update_aos(void *u) { for (int i = 0; i < P; i++) { aos[i].x += aos[i].vx; aos[i].y += aos[i].vy; aos[i].z += aos[i].vz; } }
static void update_soa(void *u) { for (int i = 0; i < P; i++) { soa.x[i] += soa.vx[i]; soa.y[i] += soa.vy[i]; soa.z[i] += soa.vz[i]; } }
int main(void) {
    bench_init();
    m = malloc(sizeof(double) * N * N); for (long i = 0; i < (long)N * N; i++) m[i] = i & 7;
    aos = malloc(sizeof(particle_t) * P);
    double **f = (double **)&soa; for (int k = 0; k < 8; k++) f[k] = malloc(sizeof(double) * P);
    for (int i = 0; i < P; i++) { aos[i].mass = i & 3; aos[i].vx = aos[i].vy = aos[i].vz = 0.5; soa.mass[i] = i & 3; soa.vx[i] = soa.vy[i] = soa.vz[i] = 0.5; }
    printf("variant,ns_per_elem,q1,q3\n");
    stats_t s;
    s = bench_run(by_rows, NULL, 11); printf("rows,%.4f,%.4f,%.4f\n", s.med / ((double)N * N), s.q1 / ((double)N * N), s.q3 / ((double)N * N));
    s = bench_run(by_cols, NULL, 11); printf("cols,%.4f,%.4f,%.4f\n", s.med / ((double)N * N), s.q1 / ((double)N * N), s.q3 / ((double)N * N));
    s = bench_run(sum_aos, NULL, 11); printf("aos_hot,%.4f,%.4f,%.4f\n", s.med / P, s.q1 / P, s.q3 / P);
    s = bench_run(sum_soa, NULL, 11); printf("soa_hot,%.4f,%.4f,%.4f\n", s.med / P, s.q1 / P, s.q3 / P);
    s = bench_run(update_aos, NULL, 11); printf("aos_update,%.4f,%.4f,%.4f\n", s.med / P, s.q1 / P, s.q3 / P);
    s = bench_run(update_soa, NULL, 11); printf("soa_update,%.4f,%.4f,%.4f\n", s.med / P, s.q1 / P, s.q3 / P);
    return 0;
}
