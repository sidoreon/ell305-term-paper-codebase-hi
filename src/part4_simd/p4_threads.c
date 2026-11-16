/* p4_threads.c : thread scaling.  Summing a 64 M element array with T
 * threads, T = 1, 2, 4, 8: (a) every thread adds into one shared atomic,
 * (b) every thread keeps a private partial sum and the main thread adds
 * the partials at the end. */
#include "bench.h"
#include <pthread.h>
#include <stdatomic.h>
#define N (1L << 26)
static int *a; static int T; static int MODE; static _Atomic long shared; static long partial[16] __attribute__((aligned(128)));
typedef struct { int id; } arg_t;
static void *worker(void *v) {
    int id = ((arg_t *)v)->id; long lo = N / T * id, hi = N / T * (id + 1); long s = 0;
    if (MODE == 0) { for (long i = lo; i < hi; i++) atomic_fetch_add_explicit(&shared, a[i], memory_order_relaxed); }
    else { for (long i = lo; i < hi; i++) s += a[i]; partial[id] = s; }
    return NULL;
}
static volatile long lsink;
static void run(void *u) {
    pthread_t th[16]; arg_t args[16]; shared = 0; memset(partial, 0, sizeof partial);
    for (int t = 0; t < T; t++) { args[t].id = t; pthread_create(&th[t], NULL, worker, &args[t]); }
    for (int t = 0; t < T; t++) pthread_join(th[t], NULL);
    long tot = shared; for (int t = 0; t < T; t++) tot += partial[t]; lsink = tot;
}
int main(void) {
    bench_init();
    a = malloc(4 * N); for (long i = 0; i < N; i++) a[i] = 1;
    printf("threads,mode,ms,q1,q3,sum\n");
    for (MODE = 1; MODE >= 0; MODE--) for (T = 1; T <= 8; T *= 2) {
        stats_t s = bench_run(run, NULL, MODE == 0 ? 3 : 7);
        printf("%d,%s,%.2f,%.2f,%.2f,%ld\n", T, MODE ? "private_partials" : "shared_atomic", s.med / 1e6, s.q1 / 1e6, s.q3 / 1e6, lsink); fflush(stdout);
    }
    return 0;
}
