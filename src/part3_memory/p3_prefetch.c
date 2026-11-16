/* p3_prefetch.c : software prefetch distance sweep.  A linked list whose nodes
 * are scattered randomly through 256 MiB is traversed by following next
 * pointers, so each load depends on the previous one and the hardware can
 * neither predict nor overlap them.  The program, however, knows the future:
 * an array holds the node addresses in traversal order, so it can issue
 * __builtin_prefetch(order[i + d]) d nodes ahead.  This is the situation
 * Mowry et al. describe: the compiler can see addresses the hardware cannot. */
#include "bench.h"
#define N (1 << 21)
#ifndef LINE
#define LINE 128
#endif
typedef struct node { struct node *next; long val; char pad[LINE - 16]; } node_t;   /* one cache line per node */
static node_t *nodes; static node_t **order; static int D; static volatile long lsink;
static void walk(void *u) {
    long s = 0; node_t *p = order[0];
    for (int i = 0; i < N - 64; i++) {
        if (D) __builtin_prefetch(order[i + D], 0, 0);
        s += p->val; p = p->next;
    }
    lsink = s;
}
int main(void) {
    bench_init();
    nodes = malloc(sizeof(node_t) * N); order = malloc(sizeof(node_t *) * N);
    int *perm = malloc(sizeof(int) * N); for (int i = 0; i < N; i++) perm[i] = i;
    srand(9); for (int i = N - 1; i > 0; i--) { int j = rand() % (i + 1); int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
    for (int i = 0; i < N; i++) { order[i] = &nodes[perm[i]]; nodes[perm[i]].val = i; nodes[perm[i]].next = (i + 1 < N) ? &nodes[perm[i + 1]] : &nodes[perm[0]]; }
    printf("distance,ns_per_elem,q1,q3\n");
    int ds[] = { 0, 1, 2, 4, 8, 16, 24, 32, 48, 64 };
    for (int k = 0; k < 10; k++) { D = ds[k]; stats_t s = bench_run(walk, NULL, 5); printf("%d,%.4f,%.4f,%.4f\n", D, s.med / N, s.q1 / N, s.q3 / N); fflush(stdout); }
    return 0;
}
