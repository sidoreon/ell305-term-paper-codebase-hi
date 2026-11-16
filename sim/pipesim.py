"""pipesim.py : a five-stage in-order pipeline simulator (IF ID EX MEM WB)
with optional forwarding, a one-cycle load-use stall when forwarding is on,
and a fixed control-hazard penalty for taken branches.

Trace format: one instruction per line,  "<op> <dst> <src1> <src2>"
  op in {alu, load, store, br, brt}   (brt = taken branch, br = not-taken)
  registers are names like r1; use "-" when a field is absent.
Output: cycles, CPI, and the number of stall cycles by cause."""
import sys, collections

def simulate(trace, forwarding=True, branch_penalty=2):
    """Return dict(cycles, instrs, data_stalls, control_stalls).
    The model: one instruction issues per cycle unless a hazard stalls it.
    Without forwarding, a RAW hazard on the instruction 1 or 2 back costs
    2 or 1 stall cycles (the value is written back in WB, read in ID with
    split-phase register file).  With forwarding, only a load followed
    immediately by a consumer costs 1 cycle.  A taken branch costs
    branch_penalty cycles (instructions fetched after it are squashed)."""
    cycles = 0; data = 0; control = 0
    hist = collections.deque(maxlen=2)          # (dst, was_load) of last two issued
    for op, dst, s1, s2 in trace:
        stall = 0
        srcs = [s for s in (s1, s2) if s != '-']
        for dist, (pdst, pload) in enumerate(reversed(hist), start=1):
            if pdst != '-' and pdst in srcs:
                if forwarding:
                    if pload and dist == 1: stall = max(stall, 1)
                else:
                    stall = max(stall, 3 - dist)     # dist 1 -> 2 stalls, dist 2 -> 1 stall
        cycles += stall; data += stall
        for _ in range(stall): hist.append(('-', False))   # bubbles occupy history slots
        cycles += 1
        hist.append((dst if op in ('alu', 'load') else '-', op == 'load'))
        if op == 'brt':
            cycles += branch_penalty; control += branch_penalty
            for _ in range(branch_penalty): hist.append(('-', False))
    cycles += 4                                     # drain the pipeline
    return dict(cycles=cycles, instrs=len(trace), data_stalls=data, control_stalls=control,
                cpi=cycles / max(1, len(trace)))

def read_trace(path):
    out = []
    for line in open(path):
        p = line.split()
        if len(p) == 4 and not line.startswith('#'): out.append(tuple(p))
    return out

if __name__ == '__main__':
    fwd = '--no-forwarding' not in sys.argv
    pen = 2
    for a in sys.argv[1:]:
        if a.startswith('--penalty='): pen = int(a.split('=')[1])
    files = [a for a in sys.argv[1:] if not a.startswith('--')]
    print('trace,forwarding,penalty,instrs,cycles,cpi,data_stalls,control_stalls')
    for f in files:
        r = simulate(read_trace(f), fwd, pen)
        print(f"{f},{int(fwd)},{pen},{r['instrs']},{r['cycles']},{r['cpi']:.3f},{r['data_stalls']},{r['control_stalls']}")
