/* p3_bandwidth.c : copying 256 MiB with memcpy, with a plain byte loop, with
 * a plain 8-byte loop, and with the compiler's vectorised loop; and the
 * same 256 MiB read sequentially, at a 1 KiB stride, and in random line
 * order, to show what the prefetcher can and cannot follow. */
#include "bench.h"
#define BYTES (256L << 20)
#ifndef LINE
#define LINE 128
#endif
static char *src, *dst; static long *idx; static volatile long lsink;
static void copy_memcpy(void *u) { memcpy(dst, src, BYTES); }
__attribute__((optimize("no-tree-vectorize"))) static void copy_bytes(void *u) { for (long i = 0; i < BYTES; i++) dst[i] = src[i]; }
__attribute__((optimize("no-tree-vectorize"))) static void copy_words(void *u) { long *d = (long *)dst, *s = (long *)src; for (long i = 0; i < BYTES / 8; i++) d[i] = s[i]; }
static void copy_vector(void *u) { long *__restrict d = (long *)dst; const long *__restrict s = (const long *)src; for (long i = 0; i < BYTES / 8; i++) d[i] = s[i]; }
static void read_seq(void *u)    { long s = 0; long *p = (long *)src; for (long i = 0; i < BYTES / 8; i += LINE / 8) s += p[i]; lsink = s; }
static void read_stride(void *u) { long s = 0; long *p = (long *)src; for (long i = 0; i < BYTES / 8; i += 128) s += p[i]; lsink = s; }
static void read_random(void *u) { long s = 0; long *p = (long *)src; for (long i = 0; i < BYTES / LINE; i++) s += p[idx[i]]; lsink = s; }
int main(void) {
    bench_init();
    src = malloc(BYTES); dst = malloc(BYTES); memset(src, 1, BYTES); memset(dst, 0, BYTES);
    long nl = BYTES / LINE; idx = malloc(8 * nl); for (long i = 0; i < nl; i++) idx[i] = i * (LINE / 8);
    srand(5); for (long i = nl - 1; i > 0; i--) { long j = rand() % (i + 1); long t = idx[i]; idx[i] = idx[j]; idx[j] = t; }
    struct { const char *nm; void (*f)(void *); double bytes; } t[] = { {"copy_memcpy", copy_memcpy, 2.0 * BYTES}, {"copy_bytes", copy_bytes, 2.0 * BYTES}, {"copy_words", copy_words, 2.0 * BYTES}, {"copy_vector", copy_vector, 2.0 * BYTES},
        {"read_sequential", read_seq, BYTES}, {"read_stride1K", read_stride, BYTES / 8}, {"read_random_lines", read_random, BYTES} };
    printf("variant,ms,GBps,q1_ms,q3_ms\n");
    for (int i = 0; i < 7; i++) { stats_t s = bench_run(t[i].f, NULL, 5); printf("%s,%.2f,%.2f,%.2f,%.2f\n", t[i].nm, s.med / 1e6, t[i].bytes / s.med, s.q1 / 1e6, s.q3 / 1e6); fflush(stdout); }
    return 0;
}
