/* p3_tlb.c : TLB reach.  A pointer chase that touches exactly one line per
 * page (pages are 16 KiB here), over k pages, for k = 1 .. 4096.  The lines
 * are chosen so that they all fit in L1 (one line per page, and different
 * sets), so a rise in latency with k is a TLB miss, not a cache miss. */
#include "bench.h"
#define HOPS 20000000L
/* defaults: macOS 16 KiB pages and 128-byte lines; Linux/WSL on x86-64: -DPAGE=4096 -DLINE=64 */
#ifndef PAGE
#define PAGE 16384
#endif
#ifndef LINE
#define LINE 128
#endif
static char *buf; static volatile void *sink;
static void chase(void *p) { void **q = (void **)p; for (long i = 0; i < HOPS; i++) { q = (void **)*q; __asm__ volatile("" : "+r"(q)); } sink = q; }
int main(void) {
    bench_init();
    long maxk = 8192; buf = malloc((maxk + 1) * (long)PAGE); memset(buf, 0, (maxk + 1) * (long)PAGE);
    printf("pages,ns_per_load,q1,q3\n");
    for (long k = 1; k <= maxk; k *= 2) {
        /* page j holds its line at offset (j * LINE) % PAGE so that consecutive pages use different L1 sets */
        for (long j = 0; j < k; j++) { long off = j * PAGE + (j * LINE) % PAGE; long nxt = (j + 1) % k; *(void **)(buf + off) = buf + nxt * PAGE + (nxt * LINE) % PAGE; }
        stats_t s = bench_run(chase, buf, 5);
        printf("%ld,%.4f,%.4f,%.4f\n", k, s.med / HOPS, s.q1 / HOPS, s.q3 / HOPS); fflush(stdout);
    }
    return 0;
}
