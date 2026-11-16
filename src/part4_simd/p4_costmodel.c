/* p4_costmodel.c : cost-model refusal.  The same legal loop with a compile-
 * time trip count n = 1 .. 32.  -fopt-info-vec-all reports which trip counts
 * the vectoriser accepts.  Each function is also timed on many small arrays. */
#include "bench.h"
#define TRIP(K) __attribute__((noinline)) void trip_##K(float *__restrict a, const float *__restrict b) { for (int i = 0; i < K; i++) a[i] = b[i] * 2.0f + 1.0f; }
TRIP(1)
TRIP(2)
TRIP(3)
TRIP(4)
TRIP(5)
TRIP(6)
TRIP(7)
TRIP(8)
TRIP(12)
TRIP(16)
TRIP(24)
TRIP(32)
int main(void) { return 0; }
