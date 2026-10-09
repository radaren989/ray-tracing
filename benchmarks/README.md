# Benchmarks

System and toolchain information is recorded in
[`system-info.md`](system-info.md) so that benchmark results remain
comparable. Compiler optimization experiments are recorded in
[`compiler-optimization-notes.md`](compiler-optimization-notes.md).
The BVH before/after measurement is recorded in
[`bvh-comparison.md`](bvh-comparison.md).

The benchmark executable reports CSV to standard output:

```text
width,samples_per_pixel,max_depth,threads,iteration,seconds,rays_per_second
```

The packet prototype has a separate benchmark target and is not enabled by
the production renderer:

```bash
make clean
make packet-benchmark \
  CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wpedantic -mavx2"
```

The current production result uses scalar recursive shading with BVH
traversal. Packet AABB and sphere experiments passed correctness tests and
showed isolated speedups, but both packet renderer integration attempts were
slower because recursive material scattering remains divergent. The latest
closest-hit integration was approximately 5.35x slower, so it remains
disabled while the scalar renderer is retained as the validated baseline.

Run the default workload with:

```bash
make benchmark BENCHMARK_ARGS="320 10 20 3 1"
```

The arguments are width, samples per pixel, maximum ray depth, iterations,
and render threads. For example, compare thread scaling with:

```bash
for threads in 1 2 4 6 12; do
  ./build/benchmark 320 10 20 3 "$threads"
done
```

Run the complete scaling measurement and calculate average time, speedup,
efficiency, cost, and overhead automatically:

```bash
make thread-scaling
```

The raw rows are saved to `build/thread-scaling.csv`. Override the workload
without editing the Makefile:

```bash
make thread-scaling THREAD_SCALING_WIDTH=640 \
  THREAD_SCALING_SAMPLES=20 THREAD_SCALING_DEPTH=30 \
  THREAD_SCALING_ITERATIONS=5
```

## Thread parallelism results

The renderer was changed to calculate pixels in worker threads. Each worker
claims the next available scanline through an atomic counter and writes only
to its own row in a pixel buffer. PPM serialization remains sequential after
all workers join, so worker threads never write to the same output stream.
The random generator was changed from global `std::rand()` to a
thread-local `std::mt19937`.

The benchmark uses the fifth argument as the worker-thread count:

```text
width samples_per_pixel max_depth iterations threads
```

The first scaling measurement used `320 10 20 3`, `-O2 -DNDEBUG`, and the
BVH scene:

| Worker threads | Average seconds | Average rays/s | Speedup | Efficiency | Cost | Overhead |
|---:|---:|---:|---:|---:|---:|---:|
| 1 | 1.264233 | 455,660 | 1.00x | 100.0% | 1.264233 | 0.000000 |
| 2 | 0.660122 | 873,525 | 1.92x | 96.0% | 1.320244 | 0.056011 |
| 4 | 0.341575 | 1,688,430 | 3.70x | 92.5% | 1.366300 | 0.102067 |
| 6 | 0.233677 | 2,465,507 | 5.41x | 90.2% | 1.402062 | 0.137829 |
| 12 | 0.192235 | 2,996,340 | 6.58x | 54.8% | 2.306820 | 1.042587 |

The calculations use the one-worker average as the baseline:

```text
Speedup(p)    = T1 / Tp
Efficiency(p) = Speedup(p) / p * 100
Cost(p)       = p * Tp
Overhead(p)   = Cost(p) - T1
```

For example, with six workers:

```text
Speedup    = 1.264233 / 0.233677 = 5.41x
Efficiency = 5.41 / 6 * 100     = 90.2%
Cost       = 6 * 0.233677        = 1.402062
Overhead   = 1.402062 - 1.264233 = 0.137829
```

Small efficiency values above 100% or negative overhead are measurement
noise, not real parallel efficiency. They occur when separate benchmark
runs differ slightly because of CPU boost, scheduling, cache state,
temperature, or background activity. Use more iterations and a heavier
workload for more stable results:

```bash
make thread-scaling THREAD_SCALING_ITERATIONS=10 \
  THREAD_SCALING_WIDTH=640 THREAD_SCALING_SAMPLES=20
```

Six workers map approximately to the six physical cores and provide the
best efficiency. Twelve workers use two logical threads per physical core;
they reduce total time further, but the second SMT thread shares execution
units and cache resources, so efficiency drops.

`btop` reports the total threads belonging to a process, including its main
thread. Therefore a benchmark started with two workers normally appears as
three process threads:

```text
1 main thread + 2 worker threads = 3 process threads
```

The system-wide thread count shown by `btop` includes threads from all
running applications and is unrelated to the CPU's 12 logical processors.

The benchmark uses BVH traversal by default. To measure the linear
`hittable_list` traversal for comparison:

```bash
RAY_TRACING_SCENE=linear make benchmark BENCHMARK_ARGS="320 10 20 3"
```

For optimized measurements:

```bash
make clean
make CXXFLAGS="-std=c++17 -O3 -march=native"
make benchmark BENCHMARK_ARGS="1920 100 50 5"
```

Record the compiler, flags, CPU, workload, and measured rows in
`benchmarks/results.csv` for each tagged project version.
