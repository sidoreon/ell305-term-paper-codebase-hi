/* p3_snippets.c : functions whose assembly is shown in Part III.  Not timed. */
#define N 1024
double rows(double *m) { double s = 0; for (int i = 0; i < N; i++) for (int j = 0; j < N; j++) s += m[i * N + j]; return s; }
double cols(double *m) { double s = 0; for (int j = 0; j < N; j++) for (int i = 0; i < N; i++) s += m[i * N + j]; return s; }
typedef struct { char tag; double v; int id; } padded_t;
typedef struct { double v; int id; char tag; } reordered_t;
typedef struct __attribute__((packed)) { char tag; double v; int id; } packed_t;
unsigned sizes(void) { return sizeof(padded_t) * 10000 + sizeof(reordered_t) * 100 + sizeof(packed_t); }
long walk_pf(long *data, int *idx, int n) { long s = 0; for (int i = 0; i < n - 16; i++) { __builtin_prefetch(&data[idx[i + 16]]); s += data[idx[i]]; } return s; }
