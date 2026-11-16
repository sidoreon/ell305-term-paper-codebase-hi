/* p1_idiom.c : idiom recognition.  A hand-written popcount loop and a
 * byte-swap loop, timed against what the compiler emits for them, and against
 * the builtin.  Compile once at -O2 (loop kept) and once at -O3 -march=native. */
#include "bench.h"
#define N (1 << 20)
static unsigned *a; static unsigned *out;
__attribute__((noinline)) static unsigned popcount_loop(unsigned v) { unsigned c = 0; while (v) { c += v & 1u; v >>= 1; } return c; }
__attribute__((noinline)) static unsigned popcount_kernighan(unsigned v) { unsigned c = 0; while (v) { v &= v - 1; c++; } return c; }
__attribute__((noinline)) static unsigned popcount_builtin(unsigned v) { return (unsigned)__builtin_popcount(v); }
__attribute__((noinline)) static unsigned bswap_manual(unsigned v) { return (v >> 24) | ((v >> 8) & 0xff00u) | ((v << 8) & 0xff0000u) | (v << 24); }
static void run_loop(void *u)      { for (int i = 0; i < N; i++) out[i] = popcount_loop(a[i]); }
static void run_kernighan(void *u) { for (int i = 0; i < N; i++) out[i] = popcount_kernighan(a[i]); }
static void run_builtin(void *u)   { for (int i = 0; i < N; i++) out[i] = popcount_builtin(a[i]); }
static void run_bswap(void *u)     { for (int i = 0; i < N; i++) out[i] = bswap_manual(a[i]); }
int main(void) {
    bench_init();
    a = malloc(sizeof(unsigned) * N); out = malloc(sizeof(unsigned) * N);
    srand(5); for (int i = 0; i < N; i++) a[i] = (unsigned)rand() ^ ((unsigned)rand() << 16);
    struct { const char *n; void (*f)(void *); } t[] = { {"popcount_loop", run_loop}, {"popcount_kernighan", run_kernighan},
        {"popcount_builtin", run_builtin}, {"bswap_manual", run_bswap} };
    printf("variant,ns_per_elem,q1,q3\n");
    for (unsigned k = 0; k < 4; k++) { stats_t s = bench_run(t[k].f, NULL, 15); printf("%s,%.4f,%.4f,%.4f\n", t[k].n, s.med / N, s.q1 / N, s.q3 / N); }
    return 0;
}
