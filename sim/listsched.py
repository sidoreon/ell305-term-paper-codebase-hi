"""listsched.py : a toy list scheduler for one basic block, after Gibbons and
Muchnick (1986).  Builds the dependence DAG of a block, assigns each node
the length of the longest path to a leaf (its priority), then repeatedly
picks the ready instruction with the highest priority, preferring one that
does not stall the instruction just scheduled (a load followed by its use).
The unscheduled and scheduled blocks are written as traces for pipesim.py."""
import sys

LAT = {'load': 2, 'alu': 1, 'store': 1}      # latency in cycles for the priority computation

def deps(block):
    """RAW, WAR and WAW dependences between instructions of one block."""
    edges = {i: set() for i in range(len(block))}
    for j, (op, dst, s1, s2) in enumerate(block):
        for i in range(j):
            pop, pdst, ps1, ps2 = block[i]
            if pdst != '-' and pdst in (s1, s2): edges[j].add(i)               # RAW
            if dst != '-' and dst in (ps1, ps2): edges[j].add(i)               # WAR
            if dst != '-' and dst == pdst: edges[j].add(i)                     # WAW
            if op == 'store' and pop == 'store': edges[j].add(i)               # memory order
            if (op == 'load' and pop == 'store') or (op == 'store' and pop == 'load'): edges[j].add(i)
    return edges

def schedule(block):
    e = deps(block); n = len(block)
    succ = {i: [j for j in range(n) if i in e[j]] for i in range(n)}
    prio = [0] * n
    for i in reversed(range(n)):                                               # longest path to a leaf
        prio[i] = LAT[block[i][0]] + max((prio[j] for j in succ[i]), default=0)
    done = set(); order = []; last = None
    while len(order) < n:
        ready = [i for i in range(n) if i not in done and e[i] <= done]
        def key(i):
            stalls = last is not None and block[last][0] == 'load' and block[last][1] in block[i][2:]
            return (-int(not stalls), -prio[i], i)
        pick = min(ready, key=key)
        order.append(pick); done.add(pick); last = pick
    return [block[i] for i in order], prio

def read_block(path):
    return [tuple(l.split()) for l in open(path) if len(l.split()) == 4 and not l.startswith('#')]

if __name__ == '__main__':
    block = read_block(sys.argv[1]); out = sys.argv[2]
    sched, prio = schedule(block)
    with open(out, 'w') as f:
        for ins in sched: f.write(' '.join(ins) + '\n')
    for i, ins in enumerate(block): print(f"{i:2d} prio={prio[i]:2d}  {' '.join(ins)}")
    print('scheduled order:', ' '.join(str(block.index(s)) for s in sched))
