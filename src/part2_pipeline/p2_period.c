/* p2_period.c : probing the branch predictor's history length.  A branch whose
 * outcome repeats with period P (a random pattern of length P, repeated) is
 * predictable only if the predictor can remember at least about P outcomes.
 * Time per element vs P locates where prediction breaks down. */
#include "bench.h"
#define N (1 << 22)
static int *d; static volatile long lsink;
static void run(void *u) { long s = 0; for (int i = 0; i < N; i++) { if (d[i]) s += i; __asm__ volatile("" : "+r"(s)); } lsink = s; }
int main(void) {
    bench_init();
    d = malloc(sizeof(int) * N);
    printf("period,ns_per_elem,q1,q3\n");
    srand(11);
    for (int P = 1; P <= 65536; P *= 2) {
        int pat[65536];
        for (int i = 0; i < P; i++) pat[i] = rand() & 1;
        if (P == 1) pat[0] = 1;
        for (int i = 0; i < N; i++) d[i] = pat[i % P];
        stats_t s = bench_run(run, NULL, 11);
        printf("%d,%.4f,%.4f,%.4f\n", P, s.med / N, s.q1 / N, s.q3 / N);
    }
    for (int i = 0; i < N; i++) d[i] = rand() & 1;
    stats_t s = bench_run(run, NULL, 11);
    printf("random,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
