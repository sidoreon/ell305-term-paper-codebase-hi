/* cal_latency.c : dependent-chain latency of add, mul, sdiv, fadd, fmul, fdiv.
 * Each iteration feeds its result into the next, so the loop runs at the
 * latency of one operation.  Output: ns per operation. */
#include "bench.h"
#define N 100000000L
static volatile long sink;
/* register constraint for a floating-point value: "w" on AArch64, "x" on x86-64 */
#if defined(__x86_64__)
#define FPREG "+x"
#else
#define FPREG "+w"
#endif
static volatile double fsink;

static void chain_add(void *a) { long x = *(long*)a; for (long i = 0; i < N; i++) { x = x + i; __asm__ volatile("" : "+r"(x)); } sink = x; }
static void chain_mul(void *a) { long x = *(long*)a; for (long i = 0; i < N; i++) { x = x * 3 + i; __asm__ volatile("" : "+r"(x)); } sink = x; }
/* the compiler must not turn the division into a multiply: divisor is unknown */
static void chain_div(void *a) { long x = *(long*)a; long d = 7 + (sink & 1); for (long i = 0; i < N; i++) { x = x / d + N; } sink = x; }
static void chain_div32(void *a) { int x = (int)*(long*)a; int d = 7 + (int)(sink & 1); for (long i = 0; i < N; i++) { x = x / d + 1000000; } sink = x; }
static void chain_fadd(void *a) { double x = (double)*(long*)a; for (long i = 0; i < N; i++) { x = x + 1.5; __asm__ volatile("" : FPREG(x)); } fsink = x; }
static void chain_fmul(void *a) { double x = 1.0000001; for (long i = 0; i < N; i++) { x = x * 1.0000001; __asm__ volatile("" : FPREG(x)); } fsink = x; }
static void chain_fdiv(void *a) { double x = 1e300; double d = 1.0000001 + (double)(sink & 1); for (long i = 0; i < N; i++) { x = x / d; } fsink = x; }

int main(void) {
    bench_init();
    long seed = 12345;
    struct { const char *name; void (*fn)(void*); } tests[] = {
        {"add", chain_add}, {"mul", chain_mul}, {"sdiv64", chain_div}, {"sdiv32", chain_div32},
        {"fadd", chain_fadd}, {"fmul", chain_fmul}, {"fdiv", chain_fdiv} };
    printf("op,ns_per_op,q1,q3\n");
    for (unsigned k = 0; k < sizeof tests / sizeof *tests; k++) {
        stats_t s = bench_run(tests[k].fn, &seed, 11);
        printf("%s,%.4f,%.4f,%.4f\n", tests[k].name, s.med / N, s.q1 / N, s.q3 / N);
    }
    return 0;
}
