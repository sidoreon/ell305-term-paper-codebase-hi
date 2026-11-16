/* p4_false.c : false sharing.  Four threads each increment their own counter
 * 20 M times with a relaxed atomic add (the thread-safe way to count).  The
 * counters are PAD bytes apart; PAD is swept from 0 (all four in one line)
 * to 256.  Also: all four threads on one shared atomic counter. */
#include "bench.h"
#include <pthread.h>
#include <stdatomic.h>
#define T 4
#define ITERS 20000000L
static char *area; static int PAD;
static void *worker(void *arg) {
    long id = (long)arg; _Atomic long *c = (_Atomic long *)(area + id * (8 + PAD));
    for (long i = 0; i < ITERS; i++) atomic_fetch_add_explicit(c, 1, memory_order_relaxed);
    return NULL;
}
static void run_threads(void *u) {
    pthread_t th[T];
    for (long t = 0; t < T; t++) pthread_create(&th[t], NULL, worker, (void *)t);
    for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
}
static _Atomic long shared;
static void *worker_atomic(void *arg) { for (long i = 0; i < ITERS; i++) atomic_fetch_add_explicit(&shared, 1, memory_order_relaxed); return NULL; }
static void run_atomic(void *u) {
    pthread_t th[T];
    for (long t = 0; t < T; t++) pthread_create(&th[t], NULL, worker_atomic, (void *)t);
    for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
}
int main(void) {
    bench_init();
    area = aligned_alloc(4096, 8192); memset(area, 0, 8192);
    printf("pad,ns_per_increment,q1,q3\n");
    int pads[] = { 0, 8, 24, 56, 64, 72, 88, 104, 120, 128, 136, 184, 248, 256 };
    for (int k = 0; k < 14; k++) { PAD = pads[k]; stats_t s = bench_run(run_threads, NULL, 5); printf("%d,%.4f,%.4f,%.4f\n", PAD, s.med / ITERS, s.q1 / ITERS, s.q3 / ITERS); fflush(stdout); }
    stats_t s = bench_run(run_atomic, NULL, 5); printf("atomic_shared,%.4f,%.4f,%.4f\n", s.med / ITERS, s.q1 / ITERS, s.q3 / ITERS);
    return 0;
}
