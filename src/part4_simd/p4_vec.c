/* p4_vec.c : auto-vectorisation.  saxpy-style kernels on int8/int16/int32/
 * int64/float/double, compiled once with -fno-tree-vectorize and once with
 * -O3 (vectorised, 128-bit NEON; -O2's cost model refuses because the global
 * pointers might alias).  The arrays fit in L1 so that memory bandwidth does
 * not cap the gain; speedup vs element width then shows that the gain is
 * the number of lanes per vector register. */
#include "bench.h"
#define N 4096          /* small enough that every array stays in L1 */
#define REPS 512
static int8_t *a8, *b8; static int16_t *a16, *b16; static int32_t *a32, *b32; static int64_t *a64, *b64; static float *af, *bf; static double *ad, *bd;
static void k8(void *u)  { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) b8[i]  = (int8_t)(a8[i] * 3 + b8[i]); }
static void k16(void *u) { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) b16[i] = (int16_t)(a16[i] * 3 + b16[i]); }
static void k32(void *u) { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) b32[i] = a32[i] * 3 + b32[i]; }
static void k64(void *u) { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) b64[i] = a64[i] * 3 + b64[i]; }
static void kf(void *u)  { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) bf[i]  = af[i] * 3.0f + bf[i]; }
static void kd(void *u)  { for (int r = 0; r < REPS; r++) for (int i = 0; i < N; i++) bd[i]  = ad[i] * 3.0 + bd[i]; }
int main(void) {
    bench_init();
    a8 = malloc(N); b8 = malloc(N); a16 = malloc(2 * N); b16 = malloc(2 * N); a32 = malloc(4 * N); b32 = malloc(4 * N);
    a64 = malloc(8 * N); b64 = malloc(8 * N); af = malloc(4 * N); bf = malloc(4 * N); ad = malloc(8 * N); bd = malloc(8 * N);
    for (int i = 0; i < N; i++) { a8[i] = b8[i] = i & 7; a16[i] = b16[i] = i & 7; a32[i] = b32[i] = i & 7; a64[i] = b64[i] = i & 7; af[i] = bf[i] = i & 7; ad[i] = bd[i] = i & 7; }
    struct { const char *nm; void (*f)(void *); } t[] = { {"int8", k8}, {"int16", k16}, {"int32", k32}, {"int64", k64}, {"float", kf}, {"double", kd} };
    printf("type,ns_per_elem,q1,q3\n");
    for (int i = 0; i < 6; i++) { stats_t s = bench_run(t[i].f, NULL, 21); printf("%s,%.4f,%.4f,%.4f\n", t[i].nm, s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS)); }
    return 0;
}
