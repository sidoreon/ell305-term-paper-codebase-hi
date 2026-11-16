/* p4_intrin.c : intrinsics versus auto-vectorisation.  A dot product written
 * three ways: plain C (the compiler vectorises with -ffast-math), plain C
 * without fast-math (a single chain), and hand-written NEON intrinsics with
 * four accumulators. */
#include "bench.h"
#include <arm_neon.h>
#define N (1 << 16)
#define REPS 64
static float *x, *y; static volatile float fsink;
__attribute__((noinline)) static float dot_c(const float *__restrict a, const float *__restrict b) { float s = 0; for (int i = 0; i < N; i++) s += a[i] * b[i]; return s; }
__attribute__((noinline)) static float dot_neon(const float *a, const float *b) {
    float32x4_t s0 = vdupq_n_f32(0), s1 = s0, s2 = s0, s3 = s0;
    for (int i = 0; i < N; i += 16) {
        s0 = vfmaq_f32(s0, vld1q_f32(a + i), vld1q_f32(b + i));
        s1 = vfmaq_f32(s1, vld1q_f32(a + i + 4), vld1q_f32(b + i + 4));
        s2 = vfmaq_f32(s2, vld1q_f32(a + i + 8), vld1q_f32(b + i + 8));
        s3 = vfmaq_f32(s3, vld1q_f32(a + i + 12), vld1q_f32(b + i + 12));
    }
    return vaddvq_f32(vaddq_f32(vaddq_f32(s0, s1), vaddq_f32(s2, s3)));
}
static void r_c(void *u) { float s = 0; const float *p = x, *q = y; for (int r = 0; r < REPS; r++) { __asm__ volatile("" : "+r"(p), "+r"(q)); s += dot_c(p, q); } fsink = s; }
static void r_neon(void *u) { float s = 0; const float *p = x, *q = y; for (int r = 0; r < REPS; r++) { __asm__ volatile("" : "+r"(p), "+r"(q)); s += dot_neon(p, q); } fsink = s; }
int main(void) {
    bench_init();
    x = aligned_alloc(16, 4 * N); y = aligned_alloc(16, 4 * N); for (int i = 0; i < N; i++) { x[i] = 1; y[i] = 0.5f; }
    printf("variant,ns_per_elem,q1,q3\n");
    stats_t s = bench_run(r_c, NULL, 15); printf("compiler,%.4f,%.4f,%.4f\n", s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS));
    s = bench_run(r_neon, NULL, 15); printf("intrinsics,%.4f,%.4f,%.4f\n", s.med / ((double)N * REPS), s.q1 / ((double)N * REPS), s.q3 / ((double)N * REPS));
    return 0;
}
