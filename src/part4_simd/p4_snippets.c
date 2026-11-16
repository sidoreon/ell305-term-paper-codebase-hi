/* p4_snippets.c : functions whose assembly is shown in Part IV.  Not timed. */
void saxpy(float *__restrict y, const float *__restrict x, float a, int n) { for (int i = 0; i < n; i++) y[i] = a * x[i] + y[i]; }
void slp(float *__restrict d, const float *__restrict a, const float *__restrict b) { d[0] = a[0] + b[0]; d[1] = a[1] + b[1]; d[2] = a[2] + b[2]; d[3] = a[3] + b[3]; }
float reduce(const float *a, int n) { float s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
int reduce_int(const int *a, int n) { int s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
void plain(float *a, float *b, float *c, int n) { for (int i = 0; i < n; i++) c[i] = a[i] * 2.0f + b[i]; }
void fill(char *p, int n) { for (int i = 0; i < n; i++) p[i] = 0; }
void copy(int *__restrict d, const int *__restrict s, int n) { for (int i = 0; i < n; i++) d[i] = s[i]; }
void trip4(float *__restrict a, const float *__restrict b) { for (int i = 0; i < 4; i++) a[i] = b[i] * 2.0f; }
void trip6(float *__restrict a, const float *__restrict b) { for (int i = 0; i < 6; i++) a[i] = b[i] * 2.0f; }
void trip3(float *__restrict a, const float *__restrict b) { for (int i = 0; i < 3; i++) a[i] = b[i] * 2.0f; }
