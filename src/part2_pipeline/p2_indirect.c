/* p2_indirect.c : indirect-branch prediction.  A tiny interpreter with eight
 * opcodes, dispatched by a switch (one indirect jump through a table) and by
 * computed goto (one indirect jump per opcode, so each has its own predictor
 * entry).  The opcode stream is either a repeating pattern or random. */
#include "bench.h"
#define N (1 << 22)
static unsigned char *ops; static volatile long lsink;
static void run_switch(void *u) {
    long acc = 0;
    for (int i = 0; i < N; i++) {
        switch (ops[i]) {
        case 0: acc += 1; break;   case 1: acc -= 2; break;   case 2: acc ^= 3; break;   case 3: acc += i; break;
        case 4: acc <<= 1; break;  case 5: acc |= 5; break;   case 6: acc -= i; break;   default: acc += 7; break;
        }
    }
    lsink = acc;
}
static void run_goto(void *u) {
    static void *tab[8] = { &&l0, &&l1, &&l2, &&l3, &&l4, &&l5, &&l6, &&l7 };
    long acc = 0; int i = 0;
    #define NEXT if (++i < N) goto *tab[ops[i]]; else goto done
    goto *tab[ops[0]];
    l0: acc += 1; NEXT;   l1: acc -= 2; NEXT;   l2: acc ^= 3; NEXT;   l3: acc += i; NEXT;
    l4: acc <<= 1; NEXT;  l5: acc |= 5; NEXT;   l6: acc -= i; NEXT;   l7: acc += 7; NEXT;
    done: lsink = acc;
}
int main(void) {
    bench_init();
    ops = malloc(N);
    printf("stream,dispatch,ns_per_op,q1,q3\n");
    for (int i = 0; i < N; i++) ops[i] = (unsigned char)(i % 8);                 /* pattern of period 8 */
    stats_t s = bench_run(run_switch, NULL, 11); printf("pattern,switch,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(run_goto, NULL, 11);   printf("pattern,goto,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    srand(3); for (int i = 0; i < N; i++) ops[i] = (unsigned char)(rand() & 7);   /* random */
    s = bench_run(run_switch, NULL, 11); printf("random,switch,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    s = bench_run(run_goto, NULL, 11);   printf("random,goto,%.4f,%.4f,%.4f\n", s.med / N, s.q1 / N, s.q3 / N);
    return 0;
}
