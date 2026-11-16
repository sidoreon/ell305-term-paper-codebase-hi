/* p4_reduce.c : a float reduction with and without -ffast-math.  Compile
 * twice; the program prints the summed value (bit pattern) and the time. */
#include "bench.h"
#define N (1 << 20)
static float *a; static volatile float fsink;
static void reduce(void *u) { float s = 0; for (int i = 0; i < N; i++) s += a[i]; fsink = s; }
int main(void) {
    bench_init();
    a = malloc(4 * N); srand(4); for (int i = 0; i < N; i++) a[i] = (float)(rand() % 1000) / 7.0f;
    stats_t s = bench_run(reduce, NULL, 21);
    union { float f; uint32_t u; } v; v.f = fsink;
    double exact = 0; for (int i = 0; i < N; i++) exact += a[i];
    printf("ns_per_elem,%.4f,%.4f,%.4f\nsum,%.9g,bits,0x%08x,exact_double,%.9g\n", s.med / N, s.q1 / N, s.q3 / N, v.f, v.u, exact);
    return 0;
}
