/* p1_sizespeed.c : size versus speed.  Three small kernels (a byte histogram,
 * a float dot product, a string-ish byte scan) timed in one binary; the
 * driver compiles this file at -O0, -O1, -Os, -O2 and -O3 and records the
 * text-segment size of each binary alongside the time. */
#include "bench.h"
#define N (1 << 20)
static unsigned char *bytes; static float *x, *y; static volatile long lsink; static volatile float fsink;
static void hist(void *u) { long h[256] = {0}; for (int i = 0; i < N; i++) h[bytes[i]]++; long s = 0; for (int k = 0; k < 256; k++) s += h[k] * k; lsink = s; }
static void dot(void *u) { float s = 0; for (int i = 0; i < N; i++) s += x[i] * y[i]; fsink = s; }
static void scan(void *u) { long c = 0; for (int i = 0; i < N; i++) if (bytes[i] == 'a' || bytes[i] == 'e') c++; lsink = c; }
int main(int argc, char **argv) {
    bench_init();
    bytes = malloc(N); x = malloc(4 * N); y = malloc(4 * N);
    srand(8); for (int i = 0; i < N; i++) { bytes[i] = (unsigned char)(rand() & 0x7f); x[i] = 1.0f; y[i] = 0.5f; }
    const char *lvl = argc > 1 ? argv[1] : "?";
    struct { const char *nm; void (*f)(void *); } t[] = { {"histogram", hist}, {"dot", dot}, {"scan", scan} };
    for (int i = 0; i < 3; i++) { stats_t s = bench_run(t[i].f, NULL, 11); printf("%s,%s,%.4f,%.4f,%.4f\n", lvl, t[i].nm, s.med / N, s.q1 / N, s.q3 / N); }
    return 0;
}
