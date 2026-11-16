/* p3_stride.c : stride sweep.  A 512 MiB buffer is swept once from start to
 * end at stride S bytes, S = 8 .. 8192 (at most 4 M loads).  Line size and
 * the hardware prefetcher show up as steps and cliffs in ns per load.
 * Also: a linked-list traversal vs an array traversal at equal element count. */
#include "bench.h"
#define BYTES (512L << 20)
#define MAXLOADS (1L << 22)
static char *buf; static long S, LOADS; static volatile long lsink;
static void strided(void *u) { long s = 0; char *p = buf; for (long i = 0; i < LOADS; i++) { s += *(long *)p; p += S; } lsink = s; }
typedef struct node { struct node *next; long val; char pad[48]; } node_t;
#define M (1 << 22)
static node_t *nodes; static node_t *head; static long *arr;
static void list_walk(void *u) { long s = 0; for (node_t *p = head; p; p = p->next) s += p->val; lsink = s; }
static void array_walk(void *u) { long s = 0; for (int i = 0; i < M; i++) s += arr[i]; lsink = s; }
int main(void) {
    bench_init();
    buf = malloc(BYTES); memset(buf, 1, BYTES);
    printf("stride,loads,ns_per_load,q1,q3\n");
    for (S = 8; S <= 8192; S *= 2) {
        LOADS = BYTES / S; if (LOADS > MAXLOADS) LOADS = MAXLOADS;
        stats_t s = bench_run(strided, NULL, 7);
        printf("%ld,%ld,%.4f,%.4f,%.4f\n", S, LOADS, s.med / LOADS, s.q1 / LOADS, s.q3 / LOADS); fflush(stdout);
    }
    /* list vs array: nodes shuffled in memory so that next is unpredictable */
    nodes = malloc(sizeof(node_t) * M); arr = malloc(sizeof(long) * M);
    int *perm = malloc(sizeof(int) * M); for (int i = 0; i < M; i++) { perm[i] = i; arr[i] = i; }
    srand(2); for (int i = M - 1; i > 0; i--) { int j = rand() % (i + 1); int t = perm[i]; perm[i] = perm[j]; perm[j] = t; }
    for (int i = 0; i < M; i++) { nodes[perm[i]].val = i; nodes[perm[i]].next = (i + 1 < M) ? &nodes[perm[i + 1]] : NULL; }
    head = &nodes[perm[0]];
    stats_t s = bench_run(list_walk, NULL, 5); printf("list_random,%ld,%.4f,%.4f,%.4f\n", (long)M, s.med / M, s.q1 / M, s.q3 / M);
    for (int i = 0; i < M; i++) { nodes[i].val = i; nodes[i].next = (i + 1 < M) ? &nodes[i + 1] : NULL; } head = &nodes[0];
    s = bench_run(list_walk, NULL, 5); printf("list_sequential,%ld,%.4f,%.4f,%.4f\n", (long)M, s.med / M, s.q1 / M, s.q3 / M);
    s = bench_run(array_walk, NULL, 9); printf("array,%ld,%.4f,%.4f,%.4f\n", (long)M, s.med / M, s.q1 / M, s.q3 / M);
    return 0;
}
