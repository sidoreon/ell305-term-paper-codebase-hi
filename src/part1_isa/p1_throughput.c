/* p1_throughput.c : int, float and double throughput.  Eight independent
 * chains of multiply-add per type, so the loop runs at the throughput of the
 * units rather than at their latency; compare with the latency chains of the
 * calibration chapter. */
#include "bench.h"
#define N 50000000L
static volatile long lsink; static volatile double dsink; static volatile float fsink;
#define OP(x) x = x * 3 + 1
static void int_tp(void *u) { long a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8; for (long i = 0; i < N; i++) { OP(a);OP(b);OP(c);OP(d);OP(e);OP(f);OP(g);OP(h); __asm__ volatile("" : "+r"(a),"+r"(b),"+r"(c),"+r"(d),"+r"(e),"+r"(f),"+r"(g),"+r"(h)); } lsink = a+b+c+d+e+f+g+h; }
static void flt_tp(void *u) { float a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8; for (long i = 0; i < N; i++) { OP(a);OP(b);OP(c);OP(d);OP(e);OP(f);OP(g);OP(h); __asm__ volatile("" : "+w"(a),"+w"(b),"+w"(c),"+w"(d),"+w"(e),"+w"(f),"+w"(g),"+w"(h)); } fsink = a+b+c+d+e+f+g+h; }
static void dbl_tp(void *u) { double a=1,b=2,c=3,d=4,e=5,f=6,g=7,h=8; for (long i = 0; i < N; i++) { OP(a);OP(b);OP(c);OP(d);OP(e);OP(f);OP(g);OP(h); __asm__ volatile("" : "+w"(a),"+w"(b),"+w"(c),"+w"(d),"+w"(e),"+w"(f),"+w"(g),"+w"(h)); } dsink = a+b+c+d+e+f+g+h; }
static void int_lat(void *u) { long a=1; for (long i = 0; i < N; i++) { OP(a); __asm__ volatile("" : "+r"(a)); } lsink = a; }
static void flt_lat(void *u) { float a=1; for (long i = 0; i < N; i++) { OP(a); __asm__ volatile("" : "+w"(a)); } fsink = a; }
static void dbl_lat(void *u) { double a=1; for (long i = 0; i < N; i++) { OP(a); __asm__ volatile("" : "+w"(a)); } dsink = a; }
int main(void) {
    bench_init();
    struct { const char *nm; void (*f)(void *); int chains; } t[] = { {"int", int_lat, 1}, {"float", flt_lat, 1}, {"double", dbl_lat, 1}, {"int", int_tp, 8}, {"float", flt_tp, 8}, {"double", dbl_tp, 8} };
    printf("type,chains,ns_per_op,q1,q3\n");
    for (int i = 0; i < 6; i++) { stats_t s = bench_run(t[i].f, NULL, 9); double ops = (double)N * t[i].chains; printf("%s,%d,%.4f,%.4f,%.4f\n", t[i].nm, t[i].chains, s.med / ops, s.q1 / ops, s.q3 / ops); }
    return 0;
}
