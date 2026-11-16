"""magic_verify.py : verify the Granlund-Montgomery multiply-shift identity
for division by 7 that gcc/clang emit, over an exhaustive 32-bit unsigned sweep
and a random signed sample, and print the constants for a table."""
import random, sys

def magic_unsigned(d, N=32):
    """Return (m, s, needs_add) such that floor(n/d) == (n*m) >> (N+s) for all n < 2^N."""
    for s in range(0, N + 1):
        m = (2 ** (N + s) + d - 1) // d                  # ceil(2^(N+s)/d)
        if m < 2 ** N:                                   # fits in N bits: plain multiply-shift
            if all(((n * m) >> (N + s)) == n // d for n in (0, 1, d - 1, d, 2 ** N - 1)):
                # sufficient condition from the paper: 2^(N+s) <= m*d <= 2^(N+s) + 2^s
                if 2 ** (N + s) <= m * d <= 2 ** (N + s) + 2 ** s:
                    return m, s, False
    # otherwise the N+1 bit constant with the add-and-shift fix-up
    s = N.bit_length() - 1
    l = (d - 1).bit_length()
    m = (2 ** (N + l) + 2 ** l) // d
    return m, l, True

def check_unsigned(d, m, s, needs_add, N=32, exhaustive=True):
    bad = 0
    rng = range(2 ** N) if exhaustive else (random.getrandbits(N) for _ in range(10 ** 6))
    for n in rng:
        if needs_add:
            t = (n * (m - 2 ** N)) >> N                 # high half of n * (m mod 2^N)
            q = (t + ((n - t) >> 1)) >> (s - 1)
        else:
            q = (n * m) >> (N + s)
        if q != n // d:
            bad += 1
    return bad

def check_signed7(samples=2 * 10 ** 6, N=32):
    """clang's sequence for n/7 (signed 32-bit): m = 0x92492493 interpreted as
    a signed 32-bit constant, hi = high32(n*m) + n, q = (hi >> 2) + (hi >>> 31)."""
    m = 0x92492493 - 2 ** 32                             # signed interpretation: -1840700269
    bad = 0
    for _ in range(samples):
        n = random.randint(-2 ** 31, 2 ** 31 - 1)
        hi = (n * m) >> 32
        hi += n
        q = (hi >> 2) + ((hi >> 31) & 1)                 # arithmetic shift, plus sign bit
        if q != int(n / 7):                               # C truncates toward zero
            bad += 1
    return bad

if __name__ == "__main__":
    random.seed(0)
    print("d,m_hex,shift,fixup")
    for d in (3, 5, 6, 7, 9, 10, 11, 13, 100):
        m, s, add = magic_unsigned(d)
        print(f"{d},0x{m:X},{s},{'yes' if add else 'no'}")
    exhaustive = "--exhaustive" in sys.argv
    m, s, add = magic_unsigned(7)
    bad = check_unsigned(7, m, s, add, exhaustive=exhaustive)
    print(f"unsigned_div7_mismatches,{bad},{'exhaustive' if exhaustive else 'random1e6'}")
    print(f"signed_div7_mismatches,{check_signed7()},random2e6")
