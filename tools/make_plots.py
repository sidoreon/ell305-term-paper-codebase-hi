"""make_plots.py : turn the CSV files in results into pgfplots figures
(plain text, data embedded) written to plots/*.tex.  Every number in the
paper's figures comes from these CSV files; nothing is typed by hand."""
import csv, os, math, sys
R = 'results'; P = 'plots'
os.makedirs(P, exist_ok=True)

def rows(name):
    with open(f'{R}/{name}') as f: return list(csv.DictReader(f))
def raw(name):
    with open(f'{R}/{name}') as f: return [l.strip().split(',') for l in f if l.strip()]
def w(name, s):
    open(f'{P}/{name}.tex', 'w').write(s)

# clock estimate: one dependent add per cycle
lat = {r['op']: float(r['ns_per_op']) for r in rows('cal_latency.csv')}
FREQ = 1.0 / lat['add']          # GHz
def cyc(ns): return ns * FREQ
open(f'{P}/freq.tex', 'w').write(f"\\newcommand{{\\measuredfreq}}{{{FREQ:.2f}}}\n")

# ---------- calibration: latency bars, measured vs book ----------
mem = {int(r['bytes']): float(r['ns_per_load']) for r in rows('cal_memory.csv')}
L1 = mem[65536]; L2 = mem[524288]; L2far = mem[4194304]; DRAM = mem[268435456] if 268435456 in mem else mem[max(mem)]
br = {r['p_taken']: float(r['ns_per_elem']) for r in rows('cal_branch.csv')}
penalty_ns = (br['0.50'] - br['sorted']) / 0.5
book = [('add', 1, 1), ('mul', 3, 4), ('sdiv64', 20, 30), ('fadd', 3, 4), ('fmul', 3, 5), ('fdiv', 13, 20),
        ('L1', 4, 5), ('L2', 12, 15), ('DRAM', 200, 300), ('mispredict', 15, 20)]
meas = dict(lat); meas.update({'L1': L1, 'L2': L2, 'DRAM': DRAM, 'mispredict': penalty_ns})
cal_rows = [(n, cyc(meas[n]), lo, hi) for n, lo, hi in book]
open(f'{P}/cal_table.tex', 'w').write('\n'.join(
    f"{n} & {meas[n]:.2f} & {c:.1f} & {lo}--{hi} \\\\" for (n, c, lo, hi) in cal_rows))
w('cal_bars', r"""\begin{tikzpicture}
\begin{axis}[ybar, ymode=log, log origin=infty, width=\textwidth, height=7cm, bar width=7pt,
  symbolic x coords={""" + ','.join(n for n, *_ in cal_rows) + r"""}, xtick=data,
  x tick label style={rotate=30, anchor=east, font=\small}, ylabel={latency (cycles)},
  legend style={at={(0.02,0.98)}, anchor=north west, font=\small}, ymin=0.5, ymax=600, enlarge x limits=0.08]
\addplot+[fill=blue!60, draw=blue!60!black] coordinates {""" + ' '.join(f"({n},{c:.2f})" for n, c, *_ in cal_rows) + r"""};
\addplot+[fill=orange!70, draw=orange!70!black] coordinates {""" + ' '.join(f"({n},{lo})" for n, _, lo, hi in cal_rows) + r"""};
\addplot+[fill=orange!30, draw=orange!70!black] coordinates {""" + ' '.join(f"({n},{hi})" for n, _, lo, hi in cal_rows) + r"""};
\legend{measured, book (low), book (high)}
\end{axis}
\end{tikzpicture}""")

# ---------- calibration: memory staircase ----------
w('cal_memory', r"""\begin{tikzpicture}
\begin{axis}[xmode=log, log basis x=2, width=\textwidth, height=7.5cm, xlabel={working set (bytes)}, ylabel={latency per load (ns)},
  grid=major, xtick={4096,65536,1048576,16777216,268435456}, xticklabels={4 KiB,64 KiB,1 MiB,16 MiB,256 MiB}, ymin=0,
  legend pos=north west, legend style={font=\small}]
\addplot+[mark=*, mark size=1.5pt, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" +
  ' '.join(f"({b},{v}) +- (0,{abs(float(r['q3'])-float(r['q1']))/2:.3f})" for r in rows('cal_memory.csv') for b, v in [(int(r['bytes']), float(r['ns_per_load']))]) + r"""};
\addlegendentry{pointer chase, random order}
\draw[dashed, red] (axis cs:131072,0) -- (axis cs:131072,90) node[pos=0.55, right, font=\small] {L1 128 KiB};
\draw[dashed, red] (axis cs:16777216,0) -- (axis cs:16777216,90) node[pos=0.45, right, font=\small] {L2 16 MiB};
\end{axis}
\end{tikzpicture}""")

# ---------- calibration: branch penalty ----------
brows = [(float(r['p_taken']), float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('cal_branch.csv') if r['p_taken'] != 'sorted']
w('cal_branch', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xlabel={fraction of elements taking the branch, $p$}, ylabel={time per element (ns)}, grid=major, ymin=0,
  legend pos=north west, legend style={font=\small}]
\addplot+[mark=*, mark size=1.5pt, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" +
  ' '.join(f"({p},{v}) +- (0,{(q3-q1)/2:.3f})" for p, v, q1, q3 in brows) + r"""};
\addlegendentry{measured, shuffled data}
\addplot[red, dashed, thick, domain=0:1, samples=101] {""" + f"{br['sorted']:.4f} + min(x,1-x)*{penalty_ns:.4f}" + r"""};
\addlegendentry{$t_{\mathrm{sorted}} + \min(p,1-p)\cdot t_{\mathrm{mispredict}}$}
\addplot[blue, dotted, thick, domain=0:1] {""" + f"{br['sorted']:.4f}" + r"""};
\addlegendentry{sorted data}
\end{axis}
\end{tikzpicture}""")
open(f'{P}/numbers.tex', 'a').write(
    f"\\newcommand{{\\penaltyns}}{{{penalty_ns:.1f}}}\n\\newcommand{{\\penaltycyc}}{{{cyc(penalty_ns):.0f}}}\n"
    f"\\newcommand{{\\Lonens}}{{{L1:.2f}}}\\newcommand{{\\Lonecyc}}{{{cyc(L1):.0f}}}\n"
    f"\\newcommand{{\\Ltwons}}{{{L2:.1f}}}\\newcommand{{\\Ltwocyc}}{{{cyc(L2):.0f}}}\\newcommand{{\\Ltwofarns}}{{{L2far:.1f}}}\\newcommand{{\\Ltwofarcyc}}{{{cyc(L2far):.0f}}}\n"
    f"\\newcommand{{\\DRAMns}}{{{DRAM:.0f}}}\\newcommand{{\\DRAMcyc}}{{{cyc(DRAM):.0f}}}\n"
    f"\\newcommand{{\\mulcyc}}{{{cyc(lat['mul']):.1f}}}\\newcommand{{\\sdivcyc}}{{{cyc(lat['sdiv64']):.1f}}}\n"
    f"\\newcommand{{\\faddcyc}}{{{cyc(lat['fadd']):.1f}}}\\newcommand{{\\fdivcyc}}{{{cyc(lat['fdiv']):.1f}}}\n"
    f"\\newcommand{{\\brsorted}}{{{br['sorted']:.2f}}}\\newcommand{{\\brhalf}}{{{br['0.50']:.2f}}}\n")

def bars(name, data, ylabel, xlabels=None, ymode=None, height='6.5cm', extra='', series=None, legend=None, rotate=25):
    """data: list of (label, value, q1, q3) or, with series, dict series->list."""
    if series is None: series = {'': data}
    series = {k: [(l.replace(',', ''), *rest) for l, *rest in d] for k, d in series.items()}   # commas break symbolic coords
    labels = [d[0] for d in next(iter(series.values()))]
    s = r"\begin{tikzpicture}" + "\n" + r"\begin{axis}[ybar, width=\textwidth, height=" + height + ", bar width=" + ('9pt' if len(series) > 1 else '14pt') + \
        (", ymode=log, log origin=infty" if ymode == 'log' else ", ymin=0") + \
        r", symbolic x coords={" + ','.join(labels) + r"}, xtick=data, x tick label style={rotate=" + str(rotate) + r", anchor=east, font=\small}, ylabel={" + ylabel + \
        r"}, enlarge x limits=" + ('0.15' if len(labels) < 4 else '0.08') + r", legend style={font=\small, at={(0.98,0.98)}, anchor=north east}, nodes near coords, nodes near coords style={font=\tiny}, every node near coord/.append style={/pgf/number format/.cd, fixed, precision=2}" + extra + "]\n"
    for k, d in series.items():
        s += r"\addplot+[error bars/.cd, y dir=both, y explicit] coordinates {" + ' '.join(f"({l},{v:.4f}) +- (0,{abs(q3-q1)/2:.4f})" for l, v, q1, q3 in d) + "};\n"
        if legend: s += r"\addlegendentry{" + k + "}\n"
    s += r"\end{axis}" + "\n" + r"\end{tikzpicture}"
    w(name, s)

def rd4(name, key, val='ns_per_elem', rename=lambda x: x):
    return [(rename(r[key]), float(r[val]), float(r['q1']), float(r['q3'])) for r in rows(name)]

# ---------- Part I ----------
d = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p1_div.csv')}
lab = {'const_signed': 'signed /7 (const)', 'runtime_signed': 'signed /d (sdiv)', 'const_unsigned': 'unsigned /7 (const)', 'runtime_unsigned': 'unsigned /d (udiv)',
       'pow2_signed': 'signed /8', 'pow2_unsigned': 'unsigned /8', 'mod7_signed': 'signed \\%7', 'divisible7': '\\%7==0'}
bars('p1_div_throughput', [(lab[k], *d[k]) for k in lab], 'ns per element (independent elements)')
bars('p1_div_chain', [('const /7 (mul-shift)', *d['chain_const']), ('runtime /d (sdiv)', *d['chain_runtime'])], 'ns per division (dependent chain)', rotate=0)
mc = rows('p1_mulconst.csv')
cst = [(int(r['k']), float(r['ns_per_op'])) for r in mc if r['variant'] == 'const']
rt = [(int(r['k']), float(r['ns_per_op'])) for r in mc if r['variant'] == 'runtime']
w('p1_mulconst', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xlabel={multiplier $k$}, ylabel={latency of $x \leftarrow kx + i$ (cycles)}, grid=major, ymin=0, ymax=6.5,
  xtick={3,5,6,7,9,10,11,12,13,15,17,25}, legend pos=south east, legend style={font=\small}]
\addplot+[only marks, mark=*, mark size=2.5pt, blue] coordinates {""" + ' '.join(f"({k},{cyc(v):.2f})" for k, v in cst) + r"""};
\addlegendentry{$k$ a compile-time constant (shift/add sequence)}
\addplot+[only marks, mark=square*, mark size=2.5pt, red] coordinates {""" + ' '.join(f"({k},{cyc(v):.2f})" for k, v in rt) + r"""};
\addlegendentry{$k$ a run-time value (\texttt{mul} + \texttt{add})}
\end{axis}
\end{tikzpicture}""")
fp = raw('p1_fp.csv')
fpd = {l[1]: (float(l[2]), float(l[3]), float(l[4])) for l in fp if l[0] == 'timing'}
bars('p1_fp', [('x / d (fdiv)', *fpd['div']), ('x * (1/d) (fmul)', *fpd['recip'])], 'ns per element', rotate=0)
idm = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p1_idiom_O2.csv')}
bars('p1_idiom', [('shift-and-count loop', *idm['popcount_loop']), ('Kernighan loop', *idm['popcount_kernighan']), ('builtin popcount', *idm['popcount_builtin']), ('manual byte swap', *idm['bswap_manual'])], 'ns per element', ymode='log')

# ---------- Part II ----------
b1 = [(float(r['p_taken']), float(r['ns_per_elem'])) for r in rows('cal_branch.csv') if r['p_taken'] != 'sorted']
c1 = [(float(r['p_taken']), float(r['ns_per_elem'])) for r in rows('p2_csel.csv') if r['p_taken'] != 'sorted']
# crossover: first p where branch curve exceeds csel curve
csel_level = sum(v for _, v in c1) / len(c1)
# the branch curve is t_sorted + p * penalty for small p; it meets the flat csel line at
cross = (csel_level - br['sorted']) / penalty_ns
open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\cselns}}{{{csel_level:.2f}}}\\newcommand{{\\crossp}}{{{cross:.2f}}}\n")
w('p2_crossover', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7.5cm, xlabel={fraction of elements satisfying the condition, $p$}, ylabel={time per element (ns)}, grid=major, ymin=0,
  legend pos=north west, legend style={font=\small}]
\addplot+[mark=*, mark size=1.5pt, thick, blue] coordinates {""" + ' '.join(f"({p},{v})" for p, v in b1) + r"""};
\addlegendentry{branch (\texttt{b.le}), \texttt{-fno-if-conversion}}
\addplot+[mark=square*, mark size=1.5pt, thick, red] coordinates {""" + ' '.join(f"({p},{v})" for p, v in c1) + r"""};
\addlegendentry{conditional select (\texttt{csel}), default \texttt{-O2}}
\draw[dashed] (axis cs:""" + f"{cross}" + r""",0) -- (axis cs:""" + f"{cross}" + r""",""" + f"{csel_level:.3f}" + r""") node[pos=0.5, right, font=\small, align=left] {crossover\\$p \approx """ + f"{cross:.2f}" + r"""$};
\draw[dashed] (axis cs:""" + f"{1-cross}" + r""",0) -- (axis cs:""" + f"{1-cross}" + r""",""" + f"{csel_level:.3f}" + r""") node[pos=0.5, left, font=\small, align=right] {crossover\\$p \approx """ + f"{1-cross:.2f}" + r"""$};
\end{axis}
\end{tikzpicture}""")
ilp = [(int(r['accumulators']), float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p2_ilp.csv')]
ilpn = [(int(r['accumulators']), float(r['ns_per_elem'])) for r in rows('p2_ilp_nosched.csv')]
w('p2_ilp', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xmode=log, log basis x=2, xlabel={number of independent accumulators}, ylabel={cycles per element}, grid=major, ymin=0,
  xtick={1,2,4,8,16}, xticklabels={1,2,4,8,16}, legend pos=north east, legend style={font=\small}]
\addplot+[mark=*, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" + ' '.join(f"({k},{cyc(v):.3f}) +- (0,{cyc(abs(q3-q1))/2:.3f})" for k, v, q1, q3 in ilp) + r"""};
\addlegendentry{default scheduling}
\addplot+[mark=square*, thick, dashed] coordinates {""" + ' '.join(f"({k},{cyc(v):.3f})" for k, v in ilpn) + r"""};
\addlegendentry{\texttt{-fno-schedule-insns2}}
\addplot[red, dotted, thick, domain=1:16] {""" + f"{cyc(ilp[0][1]):.2f}" + r"""/x};
\addlegendentry{perfect scaling: $t_1 / k$}
\end{axis}
\end{tikzpicture}""")
un = [(int(r['unroll']), float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p2_unroll.csv')]
w('p2_unroll', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=6.5cm, xmode=log, log basis x=2, xlabel={unroll factor (\texttt{\#pragma GCC unroll})}, ylabel={cycles per element}, grid=major, ymin=0,
  xtick={1,2,4,8,16,32}, xticklabels={1,2,4,8,16,32}]
\addplot+[mark=*, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" + ' '.join(f"({k},{cyc(v):.3f}) +- (0,{cyc(abs(q3-q1))/2:.3f})" for k, v, q1, q3 in un) + r"""};
\node[pin={[font=\small]45:{knee: overhead amortised}}] at (axis cs:8,""" + f"{cyc(un[3][1]):.3f}" + r""") {};
\end{axis}
\end{tikzpicture}""")
per = [(r['period'], float(r['ns_per_elem'])) for r in rows('p2_period.csv')]
pp = [(int(p), v) for p, v in per if p != 'random']; prand = [v for p, v in per if p == 'random'][0]
w('p2_period', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xmode=log, log basis x=2, xlabel={period of the branch outcome pattern}, ylabel={time per element (ns)}, grid=major, ymin=0, ymax=""" + f"{prand*1.15:.2f}" + r""",
  legend pos=north west, legend style={font=\small}]
\addplot+[mark=*, mark size=1.5pt, thick] coordinates {""" + ' '.join(f"({p},{v})" for p, v in pp) + r"""};
\addlegendentry{repeating random pattern of length $P$}
\addplot[red, dashed, thick, domain=1:65536] {""" + f"{prand:.4f}" + r"""};
\addlegendentry{fully random outcomes}
\end{axis}
\end{tikzpicture}""")
def pipe(name):
    return {os.path.basename(r['trace']).replace('pipe_', '').replace('.txt', ''): (int(r['instrs']), int(r['cycles']), int(r['data_stalls']), int(r['control_stalls'])) for r in rows(name)}
pf, pn = pipe('pipesim_fwd.csv'), pipe('pipesim_nofwd.csv')
order = ['indep', 'chain', 'loaduse', 'loaduse_sched', 'loop', 'loop_unroll4']
nice = {'indep': 'independent', 'chain': 'dependent chain', 'loaduse': 'load-use', 'loaduse_sched': 'load-use scheduled', 'loop': 'sum loop', 'loop_unroll4': 'sum loop unrolled x4'}
def stacked(name, data, title):
    s = r"""\begin{tikzpicture}
\begin{axis}[ybar stacked, width=\textwidth, height=6.5cm, bar width=16pt, ymin=0, ymax=3.4, ylabel={CPI}, title={""" + title + r"""},
  symbolic x coords={""" + ','.join(nice[k] for k in order) + r"""}, xtick=data, x tick label style={rotate=25, anchor=east, font=\small},
  legend style={font=\small, at={(0.02,0.98)}, anchor=north west}, enlarge x limits=0.1]
"""
    for idx, (lbl, col) in enumerate([('useful (1 per instruction)', 'blue!50'), ('data-hazard stalls', 'orange!70'), ('control-hazard stalls', 'red!60'), ('pipeline fill/drain', 'gray!50')]):
        s += r"\addplot+[fill=" + col + r", draw=black!50] coordinates {"
        for k in order:
            n, c, ds, cs = data[k]
            val = [1.0, ds / n, cs / n, (c - n - ds - cs) / n][idx]
            s += f"({nice[k]},{val:.3f}) "
        s += "};\n" + r"\addlegendentry{" + lbl + "}\n"
    s += r"\end{axis}" + "\n" + r"\end{tikzpicture}"
    w(name, s)
stacked('p2_pipesim_fwd', pf, 'with forwarding')
stacked('p2_pipesim_nofwd', pn, 'without forwarding')
open(f'{P}/pipesim_table.tex', 'w').write('\n'.join(f"{nice[k]} & {pf[k][0]} & {pn[k][1]} & {pn[k][1]/pn[k][0]:.2f} & {pf[k][1]} & {pf[k][1]/pf[k][0]:.2f} \\\\" for k in order))
bp = rows('bpsim.csv')
traces = ['loop8', 'alt', 'period6', 'random', 'mixed']
tn = {'loop8': 'loop (8 iters)', 'alt': 'alternating', 'period6': 'period 6', 'random': 'random', 'mixed': 'mixed program'}
preds = ['always-taken', 'BTFNT', 'heuristic', '1-bit', '2-bit', 'gshare-8']
acc = {(os.path.basename(r['trace']).replace('br_', '').replace('.txt', ''), r['predictor']): float(r['accuracy']) for r in bp}
s = r"""\begin{tikzpicture}
\begin{axis}[ybar, width=\textwidth, height=7cm, bar width=5.5pt, ymin=0, ymax=1.19, ylabel={prediction accuracy},
  symbolic x coords={""" + ','.join(tn[t] for t in traces) + r"""}, xtick=data, x tick label style={font=\small},
  legend style={font=\scriptsize, at={(0.5,0.98)}, anchor=north, legend columns=3}, enlarge x limits=0.12]
"""
for p in preds:
    s += r"\addplot coordinates {" + ' '.join(f"({tn[t]},{acc[(t, p)]:.4f})" for t in traces) + "};\n" + r"\addlegendentry{" + p + "}\n"
s += r"\end{axis}" + "\n" + r"\end{tikzpicture}"
w('p2_bpsim', s)
open(f'{P}/bpsim_table.tex', 'w').write('\n'.join(f"{tn[t]} & " + ' & '.join(f"{100*acc[(t,p)]:.1f}" for p in ['always-taken','always-not-taken','BTFNT','heuristic','1-bit','2-bit','gshare-4','gshare-8','gshare-12']) + r" \\" for t in traces))
inl = {r['variant']: (float(r['ns_per_call']), float(r['q1']), float(r['q3'])) for r in rows('p2_inline.csv')}
bars('p2_inline', [('out-of-line call', *inl['noinline']), ('inlined', *inl['inline']), ('call through pointer', *inl['pointer'])], 'ns per element', rotate=0)

# ---------- Part III ----------
ic = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p3_interchange.csv')}
cs = {os.path.basename(r['trace']).replace('cache_', '').replace('.txt', ''): float(r['miss_rate']) for r in rows('cachesim_L1.csv')}
bars('p3_interchange', [('row-major walk', *ic['rows']), ('column-major walk', *ic['cols'])], 'ns per element', rotate=0)
bars('p3_aos_soa', [('AoS, one hot field', *ic['aos_hot']), ('SoA, one hot field', *ic['soa_hot']), ('AoS, 6-field update', *ic['aos_update']), ('SoA, 6-field update', *ic['soa_update'])], 'ns per element')
open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\missrow}}{{{100*cs['rowmajor']:.1f}}}\\newcommand{{\\misscol}}{{{100*cs['colmajor']:.1f}}}\\newcommand{{\\missaos}}{{{100*cs['aos']:.1f}}}\\newcommand{{\\misssoa}}{{{100*cs['soa']:.1f}}}\n"
    f"\\newcommand{{\\rowsns}}{{{ic['rows'][0]:.2f}}}\\newcommand{{\\colsns}}{{{ic['cols'][0]:.2f}}}\\newcommand{{\\aoshot}}{{{ic['aos_hot'][0]:.2f}}}\\newcommand{{\\soahot}}{{{ic['soa_hot'][0]:.2f}}}\n")
o2 = {r['order']: (float(r['ms']), float(r['q1']), float(r['q3'])) for r in rows('p3_mm_orders_O2.csv')}
o3 = {r['order']: (float(r['ms']), float(r['q1']), float(r['q3'])) for r in rows('p3_mm_orders_O3.csv')}
ords = ['ijk', 'jik', 'ikj', 'kij', 'jki', 'kji']
bars('p3_mm_orders', None, 'time for $n=512$ (ms)', series={'\\texttt{-O2 -fno-tree-vectorize}': [(o, *o2[o]) for o in ords], '\\texttt{-O3}': [(o, *o3[o]) for o in ords]}, legend=True, ymode='log', rotate=0)
mm32 = {os.path.basename(r['trace']).replace('cache_mm_', '').replace('.txt', ''): float(r['miss_rate']) for r in rows('cachesim_mm32k.csv')}
open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\mmijk}}{{{o2['ijk'][0]:.0f}}}\\newcommand{{\\mmikj}}{{{o2['ikj'][0]:.0f}}}\\newcommand{{\\mmjki}}{{{o2['jki'][0]:.0f}}}\\newcommand{{\\mmikjOthree}}{{{o3['ikj'][0]:.0f}}}\n"
    f"\\newcommand{{\\missijk}}{{{100*mm32['ijk']:.2f}}}\\newcommand{{\\missikj}}{{{100*mm32['ikj']:.2f}}}\n")
def heat(name, csvname, title):
    t = rows(csvname)
    sizes = sorted({int(r['ti']) for r in t})
    s = r"""\begin{tikzpicture}
\begin{axis}[width=0.78\linewidth, height=0.72\linewidth, view={0}{90}, colorbar, colorbar style={width=0.12cm, ytick={0,2,4,6,8}, ylabel={}}, colormap/viridis, xlabel={$T_j$}, ylabel={$T_i$}, title={""" + title + r"""},
  xtick={0,...,""" + str(len(sizes)-1) + r"""}, xticklabels={""" + ','.join(map(str, sizes)) + r"""}, ytick={0,...,""" + str(len(sizes)-1) + r"""}, yticklabels={""" + ','.join(map(str, sizes)) + r"""},
  enlargelimits=false, point meta min=0, nodes near coords={\pgfmathprintnumber[precision=1]{\pgfplotspointmeta}}, nodes near coords style={font=\tiny, white}]
\addplot[matrix plot*, point meta=explicit, mesh/cols=""" + str(len(sizes)) + r"""] coordinates {
"""
    for r in t:
        s += f"({sizes.index(int(r['tj']))},{sizes.index(int(r['ti']))}) [{float(r['gflops']):.2f}]\n"
    s += "};\n" + r"\end{axis}" + "\n" + r"\end{tikzpicture}"
    w(name, s)
heat('p3_tiles_512', 'p3_mm_tiles_O2.csv', '$n=512$: all three matrices fit in L2')
if os.path.exists(f'{R}/rest_done.txt'): heat('p3_tiles_2048', 'p3_mm_tiles_big_O2.csv', '$n=2048$: 96 MiB of matrices, six times the L2')
best512 = max(rows('p3_mm_tiles_O2.csv'), key=lambda r: float(r['gflops']))
open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\besttile}}{{{best512['ti']}$\\times${best512['tj']}}}\\newcommand{{\\besttilems}}{{{float(best512['ms']):.1f}}}\n")
if os.path.exists(f'{R}/p3_assoc.csv'):
    asr = rows('p3_assoc.csv')
    s = r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xlabel={number of lines $k$ accessed round-robin}, ylabel={ns per load}, grid=major, ymin=0, legend pos=north west, legend style={font=\small}]
"""
    for st, lbl, mk in [('16384', 'stride 16 KiB (same L1 set)', '*'), ('16512', 'stride 16 KiB + 128 B (padded)', 'square*'), ('4096', 'stride 4 KiB', 'triangle*')]:
        s += r"\addplot+[mark=" + mk + r", mark size=1.5pt, thick] coordinates {" + ' '.join(f"({r['k']},{float(r['ns_per_load']):.3f})" for r in asr if r['stride'] == st) + "};\n" + r"\addlegendentry{" + lbl + "}\n"
    s += r"\draw[dashed, red] (axis cs:8.5,0) -- (axis cs:8.5,3.5) node[pos=0.95, right, font=\small] {8 ways};" + "\n"
    s += r"\end{axis}" + "\n" + r"\end{tikzpicture}"
    w('p3_assoc', s)
if os.path.exists(f'{R}/p3_prefetch.csv'):
    pfr = [(int(r['distance']), float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p3_prefetch.csv')]
    best = min(pfr, key=lambda x: x[1])
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\pfnone}}{{{pfr[0][1]:.1f}}}\\newcommand{{\\pfbestd}}{{{best[0]}}}\\newcommand{{\\pfbestns}}{{{best[1]:.1f}}}\n")
    w('p3_prefetch', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xlabel={prefetch distance $d$ (iterations ahead)}, ylabel={ns per element}, grid=major, ymin=0, xtick={0,8,16,24,32,48,64}]
\addplot+[mark=*, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" + ' '.join(f"({d},{v:.3f}) +- (0,{abs(q3-q1)/2:.3f})" for d, v, q1, q3 in pfr) + r"""};
\node[pin={[font=\small]0:{$d=1$: """ + f"{pfr[1][1]:.1f}" + r""" ns per node}}] at (axis cs:1,""" + f"{pfr[1][1]:.3f}" + r""") {};
\end{axis}
\end{tikzpicture}""")
if os.path.exists(f'{R}/p3_stride.csv'):
    st = raw('p3_stride.csv')[1:]
    sts = [(int(l[0]), float(l[2])) for l in st if l[0].isdigit()]
    other = {l[0]: (float(l[2]), float(l[3]), float(l[4])) for l in st if not l[0].isdigit()}
    w('p3_stride', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xmode=log, log basis x=2, xlabel={stride between consecutive loads (bytes)}, ylabel={ns per load}, grid=major, ymin=0,
  xtick={8,32,128,512,2048,8192}, xticklabels={8,32,128,512,2 Ki,8 Ki}]
\addplot+[mark=*, thick] coordinates {""" + ' '.join(f"({s},{v:.3f})" for s, v in sts) + r"""};
\draw[dashed, red] (axis cs:128,0) -- (axis cs:128,""" + f"{max(v for _, v in sts)*1.05:.2f}" + r""") node[pos=0.9, right, font=\small] {line = 128 B};
\end{axis}
\end{tikzpicture}""")
    bars('p3_list', [('array sequential', *other['array']), ('linked list sequential nodes', *other['list_sequential']), ('linked list shuffled nodes', *other['list_random'])], 'ns per element', ymode='log', rotate=15, extra=', nodes near coords={}')
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\listrand}}{{{other['list_random'][0]:.1f}}}\\newcommand{{\\listseq}}{{{other['list_sequential'][0]:.2f}}}\\newcommand{{\\arrayns}}{{{other['array'][0]:.2f}}}\n")
if os.path.exists(f'{R}/p3_mm_waterfall_O3.csv'):
    w2 = {r['stage']: float(r['ms']) for r in rows('p3_mm_waterfall_O2.csv')}
    w3 = {r['stage']: float(r['ms']) for r in rows('p3_mm_waterfall_O3.csv')}
    stages = [('naive ijk', w2['ijk']), ('interchange (ikj)', w2['ikj']), ('ikj + tiling 64', w2['tiled64']), ('ikj vectorised (-O3)', w3['ikj']), ('tiled vectorised (-O3)', w3['tiled64'])]
    base = stages[0][1]
    s = r"""\begin{tikzpicture}
\begin{axis}[ybar, width=\textwidth, height=7cm, bar width=22pt, ymin=0, ylabel={time for $n=2048$ (s)}, symbolic x coords={""" + ','.join(n for n, _ in stages) + r"""}, xtick=data,
  x tick label style={rotate=20, anchor=east, font=\small}, nodes near coords={\pgfmathprintnumber[precision=1]{\pgfplotspointmeta}$\times$}, point meta=explicit, nodes near coords style={font=\small}, enlarge x limits=0.15]
\addplot+[fill=blue!50] coordinates {""" + ' '.join(f"({n},{v/1000:.3f}) [{base/v:.1f}]" for n, v in stages) + r"""};
\end{axis}
\end{tikzpicture}"""
    w('p3_waterfall', s)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\wfijk}}{{{w2['ijk']/1000:.1f}}}\\newcommand{{\\wfikj}}{{{w2['ikj']/1000:.2f}}}\\newcommand{{\\wftiled}}{{{w2['tiled64']/1000:.2f}}}\\newcommand{{\\wfvec}}{{{w3['ikj']/1000:.2f}}}\\newcommand{{\\wftiledvec}}{{{w3['tiled64']/1000:.2f}}}\\newcommand{{\\wftotal}}{{{w2['ijk']/min(w3['ikj'],w3['tiled64']):.0f}}}\n")

# ---------- Part IV ----------
if os.path.exists(f'{R}/p4_vec_simd.csv'):
    vs = {r['type']: float(r['ns_per_elem']) for r in rows('p4_vec_scalar.csv')}
    vv = {r['type']: float(r['ns_per_elem']) for r in rows('p4_vec_simd.csv')}
    types = ['int8', 'int16', 'int32', 'int64', 'float', 'double']
    lanes = {'int8': 16, 'int16': 8, 'int32': 4, 'int64': 2, 'float': 4, 'double': 2}
    w('p4_vec', r"""\begin{tikzpicture}
\begin{axis}[ybar, width=\textwidth, height=7cm, bar width=11pt, ymin=0, ylabel={speedup of \texttt{-O3} over \texttt{-fno-tree-vectorize}}, symbolic x coords={""" + ','.join(types) + r"""}, xtick=data,
  legend style={font=\small, at={(0.98,0.98)}, anchor=north east}, enlarge x limits=0.12, nodes near coords, nodes near coords style={font=\tiny}, every node near coord/.append style={/pgf/number format/.cd, fixed, precision=1}]
\addplot+[fill=blue!60] coordinates {""" + ' '.join(f"({t},{vs[t]/vv[t]:.3f})" for t in types) + r"""};
\addlegendentry{measured speedup}
\addplot+[fill=orange!60] coordinates {""" + ' '.join(f"({t},{lanes[t]})" for t in types) + r"""};
\addlegendentry{lanes per 128-bit register}
\end{axis}
\end{tikzpicture}""")
    open(f'{P}/vec_table.tex', 'w').write('\n'.join(f"\\texttt{{{t}}} & {lanes[t]} & {vs[t]:.3f} & {vv[t]:.3f} & {vs[t]/vv[t]:.1f} \\\\" for t in types))
if os.path.exists(f'{R}/p4_ladder_fast.csv'):
    ls = {r['rung']: float(r['ns_per_elem']) for r in rows('p4_ladder_scalar.csv')}
    lv = {r['rung']: float(r['ns_per_elem']) for r in rows('p4_ladder_simd.csv')}
    lf = {r['rung']: float(r['ns_per_elem']) for r in rows('p4_ladder_fast.csv')}
    rungs = ['rung0_none', 'rung1_true_d1', 'rung2_true_d8', 'rung3_anti_d1', 'rung4_reduction', 'rung5_indirect']
    rn = {'rung0_none': '0 none', 'rung1_true_d1': '1 true d=1', 'rung2_true_d8': '2 true d=8', 'rung3_anti_d1': '3 anti d=1', 'rung4_reduction': '4 reduction', 'rung5_indirect': '5 indirect'}
    s = r"""\begin{tikzpicture}
\begin{axis}[ybar, width=\textwidth, height=7cm, bar width=8pt, ymin=0, ylabel={ns per element}, symbolic x coords={""" + ','.join(rn[r] for r in rungs) + r"""}, xtick=data,
  x tick label style={rotate=20, anchor=east, font=\small}, legend style={font=\small, at={(0.02,0.98)}, anchor=north west}, enlarge x limits=0.1]
"""
    for lbl, dd in [('\\texttt{-fno-tree-vectorize}', ls), ('\\texttt{-O2}', lv), ('\\texttt{-O2 -ffast-math}', lf)]:
        s += r"\addplot coordinates {" + ' '.join(f"({rn[r]},{dd[r]:.4f})" for r in rungs) + "};\n" + r"\addlegendentry{" + lbl + "}\n"
    s += r"\end{axis}" + "\n" + r"\end{tikzpicture}"
    w('p4_ladder', s)
    open(f'{P}/ladder_table.tex', 'w').write('\n'.join(f"{rn[r]} & {ls[r]:.3f} & {lv[r]:.3f} & {lf[r]:.3f} \\\\" for r in rungs))
if os.path.exists(f'{R}/p4_false.csv'):
    fs = [(l[0], float(l[1]), float(l[2]), float(l[3])) for l in raw('p4_false.csv')[1:]]
    pads = [(int(p), v, q1, q3) for p, v, q1, q3 in fs if p.isdigit()]
    atom = [v for p, v, *_ in fs if p == 'atomic_shared'][0]
    w('p4_false', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=7cm, xlabel={padding between the four counters (bytes)}, ylabel={ns per increment (per thread)}, grid=major, ymin=0, xtick={0,24,56,88,120,136,184,256}, x tick label style={font=\small},
  legend pos=north east, legend style={font=\small}]
\addplot+[mark=*, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" + ' '.join(f"({p},{v:.3f}) +- (0,{abs(q3-q1)/2:.3f})" for p, v, q1, q3 in pads) + r"""};
\addlegendentry{private counters, 4 threads}
\draw[dashed, red] (axis cs:56,0) -- (axis cs:56,""" + f"{max(v for _, v, *_ in pads)*1.05:.2f}" + r""") node[pos=0.55, right, font=\small, align=left] {$8+56=64$ B:\\one counter per unit};
\end{axis}
\end{tikzpicture}""")
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\fspadzero}}{{{pads[0][1]:.1f}}}\\newcommand{{\\fspadfull}}{{{pads[-1][1]:.2f}}}\\newcommand{{\\fsatomic}}{{{atom:.1f}}}\n")
if os.path.exists(f'{R}/p4_alias.csv'):
    al = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p4_alias.csv')}
    bars('p4_alias', [('plain pointers', *al['plain']), ('restrict', *al['restrict']), ('pragma omp simd', *al['omp_simd'])], 'ns per element', rotate=0)
if os.path.exists(f'{R}/p4_reduce_fast.csv'):
    rs = raw('p4_reduce_strict.csv'); rf = raw('p4_reduce_fast.csv')
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\redstrictns}}{{{float(rs[0][1]):.3f}}}\\newcommand{{\\redfastns}}{{{float(rf[0][1]):.3f}}}\\newcommand{{\\redstrictsum}}{{{rs[1][1]}}}\\newcommand{{\\redfastsum}}{{{rf[1][1]}}}\\newcommand{{\\redstrictbits}}{{{rs[1][3]}}}\\newcommand{{\\redfastbits}}{{{rf[1][3]}}}\\newcommand{{\\redexact}}{{{rs[1][5]}}}\n")

# ---------- synthesis: predicted (book) vs measured scatter ----------
w('syn_scatter', r"""\begin{tikzpicture}
\begin{axis}[width=0.8\textwidth, height=0.7\textwidth, xmode=log, ymode=log, xlabel={textbook / Fog estimate (cycles, midpoint of range)}, ylabel={measured on this machine (cycles)}, grid=major,
  xmin=0.5, xmax=600, ymin=0.5, ymax=600, legend pos=north west, legend style={font=\small}]
\addplot[gray, dashed, domain=0.5:600] {x};
\addlegendentry{$y = x$}
\addplot+[only marks, mark=*, mark size=2.5pt, nodes near coords, point meta=explicit symbolic, nodes near coords style={font=\tiny, anchor=west, xshift=2pt}] coordinates {""" +
  ' '.join(f"({(lo+hi)/2},{c:.2f}) [{n}]" for n, c, lo, hi in cal_rows) + r"""};
\addlegendentry{calibration quantities}
\end{axis}
\end{tikzpicture}""")
print('plots written; freq =', round(FREQ, 3), 'GHz; penalty', round(penalty_ns, 2), 'ns =', round(cyc(penalty_ns), 1), 'cycles')

# ---------- additions: throughput, size/speed, struct layout, fusion ----------
if os.path.exists(f'{R}/p1_throughput.csv'):
    tp = {(r['type'], r['chains']): (float(r['ns_per_op']), float(r['q1']), float(r['q3'])) for r in rows('p1_throughput.csv')}
    bars('p1_throughput', None, 'ns per multiply-add', series={'one chain (latency)': [(t, *tp[(t, '1')]) for t in ('int', 'float', 'double')],
         'eight chains (throughput)': [(t, *tp[(t, '8')]) for t in ('int', 'float', 'double')]}, legend=True, rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\tpintlat}}{{{tp[('int','1')][0]:.2f}}}\\newcommand{{\\tpinttp}}{{{tp[('int','8')][0]:.3f}}}\\newcommand{{\\tpfltlat}}{{{tp[('float','1')][0]:.2f}}}\\newcommand{{\\tpflttp}}{{{tp[('float','8')][0]:.3f}}}\n")
if os.path.exists(f'{R}/p1_sizespeed.csv'):
    ss = rows('p1_sizespeed.csv')
    levels = ['O0', 'O1', 'Os', 'O2', 'O3']
    size = {r['level']: int(r['text_bytes']) for r in ss}
    tot = {L: sum(float(r['ns_per_elem']) for r in ss if r['level'] == L) for L in levels}
    w('p1_sizespeed', r"""\begin{tikzpicture}
\begin{axis}[width=0.85\textwidth, height=7cm, xlabel={size of the three kernels' code (bytes of \texttt{\_\_text})}, ylabel={total ns per element, three kernels}, grid=major, xmin=0, ymin=0,
  nodes near coords, point meta=explicit symbolic, nodes near coords style={font=\small, anchor=south west}]
\addplot+[only marks, mark=*, mark size=3pt] coordinates {""" + ' '.join(f"({size[L]},{tot[L]:.3f}) [-{L}]" for L in levels) + r"""};
\end{axis}
\end{tikzpicture}""")
    open(f'{P}/sizespeed_table.tex', 'w').write('\n'.join(f"\\code{{-{L}}} & {size[L]} & " + ' & '.join(f"{float(r['ns_per_elem']):.2f}" for r in ss if r['level'] == L) + f" & {tot[L]:.2f} \\\\" for L in levels))
if os.path.exists(f'{R}/p3_struct.csv'):
    st = {r['layout']: (int(r['sizeof']), float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p3_struct.csv')}
    bars('p3_struct', [(f"{k} ({v[0]} B)", *v[1:]) for k, v in st.items()], 'ns per record (hot double field)', rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\stwaste}}{{{st['wasteful'][1]:.2f}}}\\newcommand{{\\streord}}{{{st['reordered'][1]:.2f}}}\\newcommand{{\\stpacked}}{{{st['packed'][1]:.2f}}}\n")
if os.path.exists(f'{R}/p3_fusion.csv'):
    fu = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p3_fusion.csv')}
    bars('p3_fusion', [('two loops', *fu['two_loops']), ('fused loop', *fu['fused']), ('eight streams', *fu['eight_streams']), ('four plus four', *fu['four_plus_four'])], 'ns per element', rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\futwo}}{{{fu['two_loops'][0]:.2f}}}\\newcommand{{\\fufused}}{{{fu['fused'][0]:.2f}}}\\newcommand{{\\fueight}}{{{fu['eight_streams'][0]:.2f}}}\\newcommand{{\\fufour}}{{{fu['four_plus_four'][0]:.2f}}}\n")

# ---------- second round of additions ----------
if os.path.exists(f'{R}/p2_chains.csv'):
    ch = [(int(r['chains']), float(r['ns_per_op']), float(r['q1']), float(r['q3'])) for r in rows('p2_chains.csv')]
    w('p2_chains', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=6.5cm, xlabel={number of independent chains $k$ (same total work)}, ylabel={cycles per multiply-add}, grid=major, ymin=0, xtick={1,2,3,4,6,8,12}, legend pos=north east, legend style={font=\small}]
\addplot+[mark=*, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" + ' '.join(f"({k},{cyc(v):.3f}) +- (0,{cyc(abs(q3-q1))/2:.3f})" for k, v, q1, q3 in ch) + r"""};
\addlegendentry{measured}
\addplot[red, dotted, thick, domain=1:12] {""" + f"{cyc(ch[0][1]):.2f}" + r"""/x};
\addlegendentry{$t_1/k$}
\addplot[blue, dashed, thick, domain=1:12] {0.5};
\addlegendentry{two multipliers: 0.5 cycles}
\end{axis}
\end{tikzpicture}""")
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\chone}}{{{cyc(ch[0][1]):.1f}}}\\newcommand{{\\chtwelve}}{{{cyc(ch[-1][1]):.2f}}}\n")
if os.path.exists(f'{R}/p2_indirect.csv'):
    ind = {(r['stream'], r['dispatch']): (float(r['ns_per_op']), float(r['q1']), float(r['q3'])) for r in rows('p2_indirect.csv')}
    bars('p2_indirect', None, 'ns per opcode', series={'switch (compare tree)': [('pattern of period 8', *ind[('pattern', 'switch')]), ('random opcodes', *ind[('random', 'switch')])],
         'computed goto (indirect jump)': [('pattern of period 8', *ind[('pattern', 'goto')]), ('random opcodes', *ind[('random', 'goto')])]}, legend=True, rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\indpsw}}{{{ind[('pattern','switch')][0]:.2f}}}\\newcommand{{\\indpgo}}{{{ind[('pattern','goto')][0]:.2f}}}\\newcommand{{\\indrsw}}{{{ind[('random','switch')][0]:.2f}}}\\newcommand{{\\indrgo}}{{{ind[('random','goto')][0]:.2f}}}\n")
if os.path.exists(f'{R}/pipesim_penalty.csv'):
    pen = [l.split(',') for l in open(f'{R}/pipesim_penalty.csv') if l.startswith('traces')]
    loop = [(int(p[2]), float(p[5])) for p in pen if 'unroll' not in p[0]]; unr = [(int(p[2]), float(p[5])) for p in pen if 'unroll' in p[0]]
    w('p2_penalty', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=6.5cm, xlabel={branch penalty (cycles, a proxy for pipeline depth)}, ylabel={simulated CPI}, grid=major, ymin=0, legend pos=north west, legend style={font=\small}]
\addplot+[mark=*, thick] coordinates {""" + ' '.join(f"({p},{c})" for p, c in loop) + r"""};
\addlegendentry{sum loop, one branch per element}
\addplot+[mark=square*, thick] coordinates {""" + ' '.join(f"({p},{c})" for p, c in unr) + r"""};
\addlegendentry{same loop unrolled by four}
\end{axis}
\end{tikzpicture}""")
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\penloopmax}}{{{loop[-1][1]:.2f}}}\\newcommand{{\\penunrmax}}{{{unr[-1][1]:.2f}}}\n")
if os.path.exists(f'{R}/p3_tlb.csv'):
    tl = [(int(r['pages']), float(r['ns_per_load']), float(r['q1']), float(r['q3'])) for r in rows('p3_tlb.csv')]
    w('p3_tlb', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=6.5cm, xmode=log, log basis x=2, xlabel={number of 16 KiB pages touched (one line each)}, ylabel={ns per dependent load}, grid=major, ymin=0, xtick={1,4,16,64,256,1024,4096}, xticklabels={1,4,16,64,256,1024,4096}]
\addplot+[mark=*, thick, error bars/.cd, y dir=both, y explicit] coordinates {""" + ' '.join(f"({p},{v:.3f}) +- (0,{abs(q3-q1)/2:.3f})" for p, v, q1, q3 in tl) + r"""};
\draw[dashed, red] (axis cs:192,0) -- (axis cs:192,12) node[pos=0.85, right, font=\small] {L1 TLB};
\draw[dashed, red] (axis cs:1400,0) -- (axis cs:1400,12) node[pos=0.85, right, font=\small] {L2 TLB};
\end{axis}
\end{tikzpicture}""")
if os.path.exists(f'{R}/p3_transpose.csv'):
    tr = [(r['variant'] + ('' if r['block'] == '0' else ' ' + r['block']), float(r['ms']), float(r['q1']), float(r['q3'])) for r in rows('p3_transpose.csv')]
    bars('p3_transpose', [(n.replace('blocked', 'block'), v, q1, q3) for n, v, q1, q3 in tr], 'time for $n = 4096$ (ms)', rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\trnaive}}{{{tr[0][1]:.0f}}}\\newcommand{{\\trbest}}{{{min(v for _, v, *_ in tr[1:]):.0f}}}\n")
if os.path.exists(f'{R}/p3_bandwidth.csv'):
    bw = {r['variant']: (float(r['ms']), float(r['GBps'])) for r in rows('p3_bandwidth.csv')}
    copies = ['copy_bytes', 'copy_words', 'copy_vector', 'copy_memcpy']
    w('p3_bandwidth', r"""\begin{tikzpicture}
\begin{axis}[ybar, width=\textwidth, height=6cm, bar width=18pt, ymin=0, ylabel={GB/s (read + write)}, symbolic x coords={byte loop,8-byte loop,vector loop,memcpy}, xtick=data, nodes near coords, nodes near coords style={font=\small}, enlarge x limits=0.15]
\addplot+[fill=blue!50] coordinates {""" + ' '.join(f"({n},{bw[c][1]:.1f})" for n, c in zip(['byte loop', '8-byte loop', 'vector loop', 'memcpy'], copies)) + r"""};
\end{axis}
\end{tikzpicture}""")
    nl = 2 * 1024 * 1024
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\bwmemcpy}}{{{bw['copy_memcpy'][1]:.0f}}}\\newcommand{{\\bwbytes}}{{{bw['copy_bytes'][1]:.1f}}}\\newcommand{{\\bwwords}}{{{bw['copy_words'][1]:.0f}}}\\newcommand{{\\rdseq}}{{{bw['read_sequential'][0]*1e6/nl:.1f}}}\\newcommand{{\\rdstride}}{{{bw['read_stride1K'][0]*1e6/(nl/8):.1f}}}\\newcommand{{\\rdrandom}}{{{bw['read_random_lines'][0]*1e6/nl:.1f}}}\n")
if os.path.exists(f'{R}/p4_aos_simd.csv'):
    ao = {r['layout']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p4_aos_simd.csv')}
    bars('p4_aos_simd', [('AoS (x 32 bytes apart)', *ao['AoS']), ('SoA (x contiguous)', *ao['SoA'])], 'ns per particle', rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\aosimd}}{{{ao['AoS'][0]:.2f}}}\\newcommand{{\\soasimd}}{{{ao['SoA'][0]:.2f}}}\n")
if os.path.exists(f'{R}/p4_threads.csv'):
    th = {(int(r['threads']), r['mode']): float(r['ms']) for r in rows('p4_threads.csv')}
    w('p4_threads', r"""\begin{tikzpicture}
\begin{axis}[width=\textwidth, height=6.5cm, ymode=log, xmode=log, log basis x=2, xlabel={threads}, ylabel={time to sum 64 M integers (ms)}, grid=major, xtick={1,2,4,8}, xticklabels={1,2,4,8}, legend pos=north west, legend style={font=\small}]
\addplot+[mark=*, thick] coordinates {""" + ' '.join(f"({t},{th[(t,'shared_atomic')]:.2f})" for t in (1, 2, 4, 8)) + r"""};
\addlegendentry{one shared atomic counter}
\addplot+[mark=square*, thick] coordinates {""" + ' '.join(f"({t},{th[(t,'private_partials')]:.2f})" for t in (1, 2, 4, 8)) + r"""};
\addlegendentry{private partial sums}
\end{axis}
\end{tikzpicture}""")
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\thpone}}{{{th[(1,'private_partials')]:.1f}}}\\newcommand{{\\thpeight}}{{{th[(8,'private_partials')]:.1f}}}\\newcommand{{\\thsone}}{{{th[(1,'shared_atomic')]:.0f}}}\\newcommand{{\\thseight}}{{{th[(8,'shared_atomic')]:.0f}}}\n")
if os.path.exists(f'{R}/p4_intrin_fast.csv'):
    ist = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p4_intrin_strict.csv')}
    ifa = {r['variant']: (float(r['ns_per_elem']), float(r['q1']), float(r['q3'])) for r in rows('p4_intrin_fast.csv')}
    bars('p4_intrin', [('compiler strict', *ist['compiler']), ('compiler fast-math', *ifa['compiler']), ('NEON intrinsics', *ifa['intrinsics'])], 'ns per element', rotate=0)
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\instrict}}{{{ist['compiler'][0]:.2f}}}\\newcommand{{\\infast}}{{{ifa['compiler'][0]:.2f}}}\\newcommand{{\\inneon}}{{{ifa['intrinsics'][0]:.3f}}}\n")
if os.path.exists(f'{R}/p4_align.csv'):
    alr = rows('p4_align.csv')
    open(f'{P}/align_table.tex', 'w').write('\n'.join(f"{i+1} & {r['offset_bytes']} & {float(r['ns_per_elem']):.3f} & {float(r['q1']):.3f}--{float(r['q3']):.3f} \\\\" for i, r in enumerate(alr)))
if os.path.exists(f'{R}/p1_fma_compare.csv'):
    fc = raw('p1_fma_compare.csv')[0]; fo = raw('p1_fma_off.csv'); ff = raw('p1_fma_fast.csv')
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\fmadiffer}}{{{int(fc[1]):,}}}\\newcommand{{\\fmaof}}{{{int(fc[3]):,}}}\\newcommand{{\\fmaerroff}}{{{float(fo[1][1]):.2e}}}\\newcommand{{\\fmaerrfast}}{{{float(ff[1][1]):.2e}}}\\newcommand{{\\fmansoff}}{{{float(fo[0][1]):.3f}}}\\newcommand{{\\fmansfast}}{{{float(ff[0][1]):.3f}}}\n")
if os.path.exists(f'{R}/pipesim_block.csv'):
    pb = rows('pipesim_block.csv')
    open(f'{P}/numbers.tex', 'a').write(f"\\newcommand{{\\blkcyc}}{{{pb[0]['cycles']}}}\\newcommand{{\\blkstall}}{{{pb[0]['data_stalls']}}}\\newcommand{{\\blkscyc}}{{{pb[1]['cycles']}}}\\newcommand{{\\blksstall}}{{{pb[1]['data_stalls']}}}\n")
