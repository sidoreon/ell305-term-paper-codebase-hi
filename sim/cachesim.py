"""cachesim.py : a set-associative cache simulator with LRU replacement and
the compulsory / capacity / conflict miss classification of Hill (1987):
a miss is compulsory if the line was never seen, conflict if a fully
associative cache of the same size would have hit, otherwise capacity.

Trace format: one address per line (decimal or 0x hex).  Usage:
  python3 cachesim.py trace.txt --size=131072 --assoc=8 --line=64"""
import sys, collections

class Cache:
    def __init__(self, size, assoc, line):
        self.line = line; self.assoc = assoc
        self.sets = size // (assoc * line)
        self.tab = [collections.OrderedDict() for _ in range(self.sets)]
    def access(self, addr):
        tag = addr // self.line; s = tag % self.sets
        st = self.tab[s]
        if tag in st:
            st.move_to_end(tag); return True
        st[tag] = None
        if len(st) > self.assoc: st.popitem(last=False)
        return False

def classify(trace, size, assoc, line):
    real = Cache(size, assoc, line)
    full = Cache(size, size // line, line)       # fully associative, same capacity
    seen = set(); c = collections.Counter()
    for a in trace:
        tag = a // line
        hit = real.access(a); fhit = full.access(a)
        if hit: c['hit'] += 1
        elif tag not in seen: c['compulsory'] += 1
        elif fhit: c['conflict'] += 1
        else: c['capacity'] += 1
        seen.add(tag)
    return c

def read_trace(path):
    return [int(l, 0) for l in open(path) if l.strip() and not l.startswith('#')]

if __name__ == '__main__':
    size, assoc, line = 131072, 8, 64
    files = []
    for a in sys.argv[1:]:
        if a.startswith('--size='): size = int(a[7:])
        elif a.startswith('--assoc='): assoc = int(a[8:])
        elif a.startswith('--line='): line = int(a[7:])
        else: files.append(a)
    print('trace,size,assoc,line,accesses,hits,compulsory,capacity,conflict,miss_rate')
    for f in files:
        tr = read_trace(f); c = classify(tr, size, assoc, line); n = len(tr)
        m = c['compulsory'] + c['capacity'] + c['conflict']
        print(f"{f},{size},{assoc},{line},{n},{c['hit']},{c['compulsory']},{c['capacity']},{c['conflict']},{m / n:.4f}")
