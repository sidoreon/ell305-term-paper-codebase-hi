/* cal_branch.c : branch misprediction penalty.  A loop with a data-dependent
 * branch runs over (a) sorted data, where the branch is trivially predictable,
 * and (b) shuffled data with a taken-fraction p swept from 0 to 1.  The
 * compiler is prevented from converting the branch into a conditional select
 * by -fno-if-conversion -fno-if-conversion2 -fno-tree-loop-if-convert, and the
 * vectoriser (which would also remove the branch) is disabled with
 * -fno-tree-vectorize. */
#include "bench.h"
#define N 16777216
static int *data; static long acc;
static void run(void *unused) {
    long s = 0;
    for (int i = 0; i < N; i++) { if (data[i] >= 128) s += data[i]; __asm__ volatile("" : "+r"(s)); }
    acc = s;
}
int main(void) {
    bench_init();
    data = (int *)malloc(sizeof(int) * N);
    printf("p_taken,ns_per_elem,q1,q3\n");
    srand(7);
    for (int pc = 0; pc <= 100; pc += 5) {
        for (int i = 0; i < N; i++) data[i] = (rand() % 100 < pc) ? 200 : 50;
        stats_t s = bench_run(run, NULL, 11);
        printf("%.2f,%.4f,%.4f,%.4f\n", pc / 100.0, s.med / N, s.q1 / N, s.q3 / N);
    }
    /* sorted version of the p = 0.5 case */
    for (int i = 0; i < N; i++) data[i] = (i < N / 2) ? 50 : 200;
    stats_t s = bench_run(run, NULL, 11);
    printf("sorted,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
