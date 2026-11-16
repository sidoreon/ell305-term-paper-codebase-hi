"""tracegen.py : generates the synthetic traces used by the three simulators.
Writes into the directory given as the first argument (default: traces/)."""
import os, random, sys

def pipeline_traces(d):
    # (1) a dependent chain: every instruction uses the previous result
    with open(f'{d}/pipe_chain.txt', 'w') as f:
        for i in range(1000): f.write(f"alu r{(i % 8) + 1} r{((i - 1) % 8) + 1} -\n")
    # (2) an independent stream: no RAW hazards at all
    with open(f'{d}/pipe_indep.txt', 'w') as f:
        for i in range(1000): f.write(f"alu r{(i % 8) + 1} r9 r10\n")
    # (3) load-use: load then immediately consume, repeated
    with open(f'{d}/pipe_loaduse.txt', 'w') as f:
        for i in range(500): f.write(f"load r1 r9 -\nalu r2 r1 r3\n")
    # (4) the same load-use pairs, scheduled one instruction apart
    with open(f'{d}/pipe_loaduse_sched.txt', 'w') as f:
        for i in range(250): f.write(f"load r1 r9 -\nload r4 r10 -\nalu r2 r1 r3\nalu r5 r4 r3\n")
    # (5) an array-sum loop body: load, add, increment, compare, taken branch
    with open(f'{d}/pipe_loop.txt', 'w') as f:
        for i in range(200): f.write("load r1 r2 -\nalu r3 r3 r1\nalu r2 r2 -\nalu r4 r2 r5\nbrt - r4 -\n")
    # (6) the same loop unrolled by four: one branch per four loads
    with open(f'{d}/pipe_loop_unroll4.txt', 'w') as f:
        for i in range(50):
            f.write("load r1 r2 -\nload r6 r2 -\nload r7 r2 -\nload r8 r2 -\n")
            f.write("alu r3 r3 r1\nalu r3 r3 r6\nalu r3 r3 r7\nalu r3 r3 r8\nalu r2 r2 -\nalu r4 r2 r5\nbrt - r4 -\n")

def branch_traces(d):
    random.seed(1)
    # loop with 100 iterations of an inner loop of 8: back-edge taken 7 of 8 times
    with open(f'{d}/br_loop8.txt', 'w') as f:
        for i in range(2000):
            f.write(f"100 {1 if (i % 8) != 7 else 0} b 1\n")
    # random data-dependent branch, p = 0.5
    with open(f'{d}/br_random.txt', 'w') as f:
        for i in range(10000): f.write(f"200 {random.randint(0, 1)} f -\n")
    # alternating pattern T N T N (period 2)
    with open(f'{d}/br_alt.txt', 'w') as f:
        for i in range(10000): f.write(f"300 {i % 2} f -\n")
    # a periodic pattern of length 6
    pat = [1, 1, 0, 1, 0, 0]
    with open(f'{d}/br_period6.txt', 'w') as f:
        for i in range(12000): f.write(f"400 {pat[i % 6]} f -\n")
    # a mix as a small program would produce: loop back-edge + an error check
    # that is almost never taken (Ball-Larus: pointer/error-check heuristic says not taken)
    with open(f'{d}/br_mixed.txt', 'w') as f:
        for i in range(5000):
            f.write(f"500 {1 if random.random() < 0.02 else 0} f 0\n")   # guard, hint: not taken
            f.write(f"504 {1 if random.random() < 0.7 else 0} f 1\n")    # data branch, hint: taken
            f.write(f"508 {1 if (i % 16) != 15 else 0} b 1\n")          # loop back-edge

def cache_traces(d):
    N = 512; line = 64
    # row-major vs column-major sweep over a 512x512 array of 8-byte doubles
    with open(f'{d}/cache_rowmajor.txt', 'w') as f:
        for i in range(N):
            for j in range(N): f.write(f"{(i * N + j) * 8}\n")
    with open(f'{d}/cache_colmajor.txt', 'w') as f:
        for j in range(N):
            for i in range(N): f.write(f"{(i * N + j) * 8}\n")
    # AoS (one hot 4-byte field of a 64-byte struct) vs SoA (packed field)
    with open(f'{d}/cache_aos.txt', 'w') as f:
        for i in range(65536): f.write(f"{i * 64}\n")
    with open(f'{d}/cache_soa.txt', 'w') as f:
        for i in range(65536): f.write(f"{i * 4}\n")
    # power-of-two stride aliasing: 16 streams 16 KiB apart (same L1 set) vs padded by 64 B
    for name, pad in (('cache_alias_pow2', 0), ('cache_alias_padded', 64)):
        with open(f'{d}/{name}.txt', 'w') as f:
            for i in range(1024):
                for k in range(16): f.write(f"{k * (16384 + pad) + i * 8}\n")
    # matmul ijk vs ikj, n = 64, doubles, three arrays A B C at 0, 32K, 64K
    n = 64; A, B, C = 0, 32768, 65536
    with open(f'{d}/cache_mm_ijk.txt', 'w') as f:
        for i in range(n):
            for j in range(n):
                for k in range(n): f.write(f"{A + (i*n+k)*8}\n{B + (k*n+j)*8}\n{C + (i*n+j)*8}\n")
    with open(f'{d}/cache_mm_ikj.txt', 'w') as f:
        for i in range(n):
            for k in range(n):
                for j in range(n): f.write(f"{A + (i*n+k)*8}\n{B + (k*n+j)*8}\n{C + (i*n+j)*8}\n")

if __name__ == '__main__':
    d = sys.argv[1] if len(sys.argv) > 1 else 'traces'
    os.makedirs(d, exist_ok=True)
    pipeline_traces(d); branch_traces(d); cache_traces(d)
    print('traces written to', d)
