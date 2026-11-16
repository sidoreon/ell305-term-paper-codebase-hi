# Architectural Foundations of Compiler Optimisations: companion code

Companion repository for the ELL305 (Computer Architecture) term paper
**Architectural Foundations of Compiler Optimisations**, Department of
Electrical Engineering, IIT Delhi, 17 November 2026.

**Authors:** Sidharth Valsan, Tanish Arya, Nihar V Rao, Bhargav Lad.


This repository holds every benchmark, simulator and script used in the ELL305
term paper *Architectural Foundations of Compiler Optimisations*. The paper
examines, for each compiler optimisation, the hardware cost it tries to reduce,
predicts its effect from an in-order cost model, and measures whether the
optimisation still pays off on one modern out-of-order core (an Apple M5 performance core). The code here is the
single source of truth: the paper carries only short before-and-after assembly
fragments, and each of those cites the file in this tree that produced it.

All headline numbers in the paper were measured on one machine (Apple M5, macOS
26.5, GCC 15.2, Clang 17, Python 3.11). Timings on any other machine will
differ, and the clock frequency is estimated rather than read from a counter, so
the absolute cycle counts are machine-specific by construction. Directions and
mechanisms reproduce; the second significant figure does not.

## Layout

```
bench/      bench.h          timing harness shared by every C benchmark
src/
  calibration/   cal_latency.c cal_memory.c cal_branch.c   machine calibration
  part1_isa/     instruction selection and arithmetic
  part2_pipeline/pipeline, branches, ILP, inlining, dispatch
  part3_memory/  caches, loop orders, prefetch, layouts
  part4_simd/    auto-vectorisation, legality, reductions, threads
sim/        pipesim.py bpsim.py cachesim.py tracegen.py listsched.py
tools/      magic_verify.py extract_asm.py make_plots.py readability.py
results/    result CSVs from the paper's runs 
asm/        assembly listings .txt (from extract_asm.py)
plots/      pgfplots .tex (from make_plots.py)
build/      compiled binaries (created when you compile)
traces/     simulator traces (created by tracegen.py)
```

## Prerequisites

- GCC 15.2 or any GCC 12 or later (the main compiler; `gcc-15` below).
- Clang (for the x86-64 cross-listings): `clang --target=x86_64-unknown-linux-gnu`.
- Python 3.11 (simulators and plotting). `make_plots.py` uses `matplotlib`.
- A POSIX shell. The benchmarks use `clock_gettime(CLOCK_MONOTONIC_RAW)` and, on
  Apple silicon, `pthread_set_qos_class_self_np` to ask for a performance core.

## How to run

Run everything from this folder. Each benchmark is a single C file that prints
CSV to standard output, so you compile it, run it, and save what it prints:

```sh
mkdir -p build results traces
gcc -O2 -Ibench -o build/p1_div src/part1_isa/p1_div.c
./build/p1_div > results/p1_div.csv
```

Some experiments need extra compiler flags so that the compiler does not undo
what is being measured:

| Flags | Used for |
|---|---|
| `-fno-tree-vectorize` | Part II (`p2_ilp`, `p2_inline`, `p2_unroll`, `p2_chains`, `p2_indirect`), Part III loop experiments (`p3_interchange`, `p3_matmul`, `p3_fusion`, `p3_transpose`) and the scalar run of `p4_vec` / `p4_ladder` |
| `-fno-if-conversion -fno-if-conversion2 -fno-tree-loop-if-convert -fno-tree-vectorize` | `cal_branch.c`, `p2_period.c` (keeps the branches as branches) |
| `-O3` | vectorised runs of `p4_vec` and `p3_matmul` |
| `-ffast-math` | the `fast` runs of `p4_reduce`, `p4_ladder` |
| `-fopenmp-simd` | `p4_alias.c` |
| `-lpthread` | `p4_false.c`, `p4_threads.c` |
| `-fopt-info-vec-all` | the `*_optinfo.txt` vectoriser reports for `p4_ladder`, `p4_costmodel` |

`p3_matmul` takes a mode argument (`orders`, `tiles` or `waterfall`) and the
matrix size with `-Dn=2048`.

Simulators (pure Python, no packages needed):

```sh
python3 sim/tracegen.py traces
python3 sim/pipesim.py traces/pipe_*.txt > results/pipesim_fwd.csv
python3 sim/pipesim.py --no-forwarding traces/pipe_*.txt > results/pipesim_nofwd.csv
python3 sim/bpsim.py traces/br_*.txt > results/bpsim.csv
python3 sim/cachesim.py --line=128 traces/cache_*.txt > results/cachesim_L1.csv
python3 sim/listsched.py
```

Other tools:

- `tools/make_plots.py` reads `results/` and writes the figures to `plots/`.
- `tools/extract_asm.py <file>.s <function>` pulls one function out of `-S`
  output; this is how the listings in `asm/` and in the paper were made.
- `tools/magic_verify.py` checks the division magic constants
  (`--exhaustive` tests all 2^32 inputs).
- `tools/readability.py` computes the reading indices in Appendix B.

## What reproduces and what does not

- **Reproduces anywhere:** the *direction* of every effect, the assembly the
  compiler emits, the simulator miss and stall counts (they depend only on the
  trace, not the machine).
- **Machine-specific:** every nanosecond and every cycle count. Cycles are
  nanoseconds times the estimated clock (`cal_latency.c`), so a different clock
  rescales them all. On a machine without a hardware prefetcher or an
  out-of-order core (for example the textbook's in-order pipeline) several
  "reduced" verdicts in the paper would become "confirmed", because the cost the
  optimisation removes is actually present there.

## Map: paper figure/table to source to result

| Paper figure / table | Source file | Result CSV |
|---|---|---|
| Calibration table, latency bars, clock | `src/calibration/cal_latency.c` | `cal_latency.csv` |
| Pointer-chase staircase | `src/calibration/cal_memory.c` | `cal_memory.csv` |
| Misprediction penalty curve | `src/calibration/cal_branch.c` | `cal_branch.csv` |
| Stride / line-size sweep | `src/part3_memory/p3_stride.c` | `p3_stride.csv` |
| TLB sweep | `src/part3_memory/p3_tlb.c` | `p3_tlb.csv` |
| Bandwidth ceilings | `src/part3_memory/p3_bandwidth.c` | `p3_bandwidth.csv` |
| Latency vs throughput (int/float/double) | `src/part1_isa/p1_throughput.c` | `p1_throughput.csv` |
| Division by a constant | `src/part1_isa/p1_div.c` | `p1_div.csv` |
| Magic-constant table | `tools/magic_verify.py` | `magic.csv` |
| Multiplication by a constant | `src/part1_isa/p1_mulconst.c` | `p1_mulconst.csv` |
| Floating point, reciprocal | `src/part1_isa/p1_fp.c` | `p1_fp.csv` |
| FMA contraction | `src/part1_isa/p1_fma.c` | `p1_fma.csv` |
| Idiom recognition | `src/part1_isa/p1_idiom.c` | `p1_idiom_O2.csv` |
| Size vs speed (-O0..-O3) | `src/part1_isa/p1_sizespeed.c` | `p1_sizespeed.csv` |
| Signed overflow (appendix) | `src/part1_isa/p1_overflow.c` | — |
| Part I assembly snippets | `src/part1_isa/p1_snippets.c` | (via `extract_asm.py`) |
| Accumulators and ILP | `src/part2_pipeline/p2_ilp.c` | `p2_ilp.csv`, `p2_ilp_nosched.csv` |
| Chain length | `src/part2_pipeline/p2_chains.c` | `p2_chains.csv` |
| Loop unrolling | `src/part2_pipeline/p2_unroll.c` | `p2_unroll.csv` |
| Inlining, `__builtin_expect` | `src/part2_pipeline/p2_inline.c` | `p2_inline.csv` |
| Indirect branches | `src/part2_pipeline/p2_indirect.c` | `p2_indirect.csv` |
| Probing the predictor (period) | `src/part2_pipeline/p2_period.c` | `p2_period.csv` |
| Branch vs select (crossover) | `src/calibration/cal_branch.c` | `p2_csel.csv` |
| Pipeline simulator (Appendix D) | `sim/pipesim.py`, `sim/tracegen.py` | `pipesim_fwd.csv`, `pipesim_nofwd.csv` |
| Branch-predictor simulator (Appendix D) | `sim/bpsim.py` | `bpsim.csv` |
| List scheduler (Appendix D) | `sim/listsched.py` | (prints schedule) |
| Loop interchange | `src/part3_memory/p3_interchange.c` | `p3_interchange.csv` |
| Matrix multiply, six orders, tiling, waterfall | `src/part3_memory/p3_matmul.c` | `p3_mm_orders_*.csv`, `p3_mm_tiles*.csv`, `p3_mm_waterfall_*.csv` |
| Software prefetch | `src/part3_memory/p3_prefetch.c` | `p3_prefetch.csv` |
| Loop fusion / fission | `src/part3_memory/p3_fusion.c` | `p3_fusion.csv` |
| Associativity / padding (appendix) | `src/part3_memory/p3_assoc.c` | `p3_assoc.csv` |
| Blocked transpose (appendix) | `src/part3_memory/p3_transpose.c` | `p3_transpose.csv` |
| Struct layout (appendix) | `src/part3_memory/p3_struct.c` | `p3_struct.csv` |
| Cache simulator (Appendix D) | `sim/cachesim.py` | `cachesim_*.csv` |
| Auto-vectorisation, lane count | `src/part4_simd/p4_vec.c` | `p4_vec_scalar.csv`, `p4_vec_simd.csv` |
| Legality ladder | `src/part4_simd/p4_ladder.c` | `p4_ladder_*.csv` |
| Reductions and `-ffast-math` | `src/part4_simd/p4_reduce.c` | `p4_reduce_*.csv` |
| Aliasing and `restrict` | `src/part4_simd/p4_alias.c` | `p4_alias.csv` |
| Cost model by trip count | `src/part4_simd/p4_costmodel.c` | `p4_costmodel_optinfo.txt` |
| SLP, library idioms | `src/part4_simd/p4_snippets.c` | (via `extract_asm.py`) |
| False sharing (appendix) | `src/part4_simd/p4_false.c` | `p4_false.csv` |
| AoS/SoA for SIMD (appendix) | `src/part4_simd/p4_aos_simd.c` | `p4_aos_simd.csv` |
| Thread scaling (appendix) | `src/part4_simd/p4_threads.c` | `p4_threads.csv` |
| Intrinsics vs auto-vectoriser (appendix) | `src/part4_simd/p4_intrin.c` | `p4_intrin.csv` |
