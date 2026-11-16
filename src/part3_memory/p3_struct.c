/* p3_struct.c : struct layout and compiler padding.  Three declarations of
 * the same three fields: in a wasteful order, reordered by size, and packed.
 * sizeof shows the padding; a hot-field sum over an array of each shows the
 * cost of the wasted bytes; the packed version also pays for misaligned loads. */
#include "bench.h"
#define P (1 << 21)
typedef struct { char tag; double v; int id; } wasteful_t;                        /* 1 + 7 pad + 8 + 4 + 4 pad = 24 */
typedef struct { double v; int id; char tag; } reordered_t;                       /* 8 + 4 + 1 + 3 pad = 16 */
typedef struct __attribute__((packed)) { char tag; double v; int id; } packed_t;  /* 13, v misaligned */
static wasteful_t *w; static reordered_t *r; static packed_t *p; static volatile double dsink;
static void sum_w(void *u) { double s = 0; for (int i = 0; i < P; i++) s += w[i].v; dsink = s; }
static void sum_r(void *u) { double s = 0; for (int i = 0; i < P; i++) s += r[i].v; dsink = s; }
static void sum_p(void *u) { double s = 0; for (int i = 0; i < P; i++) s += p[i].v; dsink = s; }
int main(void) {
    bench_init();
    w = malloc(sizeof(wasteful_t) * P); r = malloc(sizeof(reordered_t) * P); p = malloc(sizeof(packed_t) * P);
    for (int i = 0; i < P; i++) { w[i].v = r[i].v = p[i].v = i & 7; }
    printf("layout,sizeof,bytes_per_array_MiB,ns_per_elem,q1,q3\n");
    stats_t s;
    s = bench_run(sum_w, NULL, 11); printf("wasteful,%zu,%.1f,%.4f,%.4f,%.4f\n", sizeof(wasteful_t), sizeof(wasteful_t) * (double)P / 1048576, s.med / P, s.q1 / P, s.q3 / P);
    s = bench_run(sum_r, NULL, 11); printf("reordered,%zu,%.1f,%.4f,%.4f,%.4f\n", sizeof(reordered_t), sizeof(reordered_t) * (double)P / 1048576, s.med / P, s.q1 / P, s.q3 / P);
    s = bench_run(sum_p, NULL, 11); printf("packed,%zu,%.1f,%.4f,%.4f,%.4f\n", sizeof(packed_t), sizeof(packed_t) * (double)P / 1048576, s.med / P, s.q1 / P, s.q3 / P);
    return 0;
}
