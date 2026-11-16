/* p1_snippets.c : small functions whose assembly is shown in Part I.  Not timed. */
#include <stdint.h>
int  div7_signed(int n)          { return n / 7; }
unsigned div7_unsigned(unsigned n){ return n / 7u; }
int  div8_signed(int n)          { return n / 8; }
unsigned div8_unsigned(unsigned n){ return n / 8u; }
int  mod7(int n)                 { return n % 7; }
int  divisible7(unsigned n)      { return n % 7u == 0; }
long mul9(long x)                { return x * 9; }
long mul10(long x)               { return x * 10; }
long mul7(long x)                { return x * 7; }
long addr(long *p, long i)       { return p[i + 3]; }
int  fold(void)                  { int a = 6, b = 7; return a * b + (1 << 4); }
int  propagate(int x)            { int k = 5; int y = k * 2; return x + y + k; }
double assoc_l(double a, double b, double c) { return (a + b) + c; }
double assoc_r(double a, double b, double c) { return a + (b + c); }
float fdiv3(float x)             { return x / 3.0f; }
float fma_c(float a, float b, float c) { return a * b + c; }
unsigned popcnt_loop(unsigned v) { unsigned c = 0; while (v) { v &= v - 1; c++; } return c; }
unsigned bswap_manual(unsigned v){ return (v >> 24) | ((v >> 8) & 0xff00u) | ((v << 8) & 0xff0000u) | (v << 24); }
long sum_index(int *a, int n)    { long s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
int overflow_int(int *a, int n)  { int s = 0; for (int i = 0; i < n; i++) s += a[i]; return s; }
unsigned overflow_unsigned(int *a, unsigned n) { unsigned s = 0; for (unsigned i = 0; i < n; i++) s += a[i]; return s; }
