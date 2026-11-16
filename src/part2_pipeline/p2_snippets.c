/* p2_snippets.c : small functions whose assembly is shown in Part II.  Not timed. */
long branchy(int *a, int n)  { long s = 0; for (int i = 0; i < n; i++) if (a[i] >= 128) s += a[i]; return s; }
int  maxsel(int a, int b)    { return a > b ? a : b; }
int  abs_sel(int x)          { return x < 0 ? -x : x; }
long expect_hot(int *a, int n){ long s = 0; for (int i = 0; i < n; i++) { if (__builtin_expect(a[i] < 0, 0)) s -= 1000; else s += a[i]; } return s; }
long no_expect(int *a, int n) { long s = 0; for (int i = 0; i < n; i++) { if (a[i] < 0) s -= 1000; else s += a[i]; } return s; }
long load_use(long *p)       { long x = p[0]; long y = x + 1; return y * 2; }
long sum1(double *a, int n)  { double s = 0; for (int i = 0; i < n; i++) s += a[i]; return (long)s; }
