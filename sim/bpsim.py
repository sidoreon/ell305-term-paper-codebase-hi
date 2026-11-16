"""bpsim.py : branch-predictor simulator.  Static predictors (always taken,
always not taken, backward-taken/forward-not-taken, and two Ball-Larus style
heuristics supplied as trace hints) and dynamic predictors (1-bit, 2-bit
saturating counter per Smith 1981, and gshare with a global history register).

Trace format: one branch per line  "<pc> <taken 0/1> <direction b/f> <hint 0/1>"
  direction: b = backward branch (loop back-edge), f = forward
  hint: the outcome a Ball-Larus heuristic would guess (1 = taken), or - if none."""
import sys

def static_taken(tr):      return sum(t for _, t, _, _ in tr)
def static_not_taken(tr):  return sum(1 - t for _, t, _, _ in tr)
def static_btfnt(tr):      return sum((t == (d == 'b')) for _, t, d, _ in tr)
def static_heuristic(tr):  return sum((t == h) if h != '-' else (t == (d == 'b')) for _, t, d, h in tr)

def one_bit(tr, entries=1024):
    tab = [1] * entries; ok = 0
    for pc, t, _, _ in tr:
        i = pc % entries
        ok += (tab[i] == t); tab[i] = t
    return ok

def two_bit(tr, entries=1024):
    tab = [2] * entries; ok = 0                # 0,1 predict not taken; 2,3 predict taken
    for pc, t, _, _ in tr:
        i = pc % entries
        ok += ((tab[i] >= 2) == t)
        tab[i] = min(3, tab[i] + 1) if t else max(0, tab[i] - 1)
    return ok

def gshare(tr, hist_bits=8, entries=1024):
    tab = [2] * entries; h = 0; ok = 0; mask = (1 << hist_bits) - 1
    for pc, t, _, _ in tr:
        i = (pc ^ h) % entries
        ok += ((tab[i] >= 2) == t)
        tab[i] = min(3, tab[i] + 1) if t else max(0, tab[i] - 1)
        h = ((h << 1) | t) & mask
    return ok

def read_trace(path):
    out = []
    for line in open(path):
        p = line.split()
        if len(p) == 4 and not line.startswith('#'):
            out.append((int(p[0]), int(p[1]), p[2], (int(p[3]) if p[3] != '-' else '-')))
    return out

PREDICTORS = [('always-taken', static_taken), ('always-not-taken', static_not_taken),
              ('BTFNT', static_btfnt), ('heuristic', static_heuristic),
              ('1-bit', one_bit), ('2-bit', two_bit),
              ('gshare-4', lambda tr: gshare(tr, 4)), ('gshare-8', lambda tr: gshare(tr, 8)),
              ('gshare-12', lambda tr: gshare(tr, 12))]

if __name__ == '__main__':
    print('trace,predictor,branches,correct,accuracy')
    for f in sys.argv[1:]:
        tr = read_trace(f)
        for name, fn in PREDICTORS:
            ok = fn(tr)
            print(f"{f},{name},{len(tr)},{ok},{ok / len(tr):.4f}")
