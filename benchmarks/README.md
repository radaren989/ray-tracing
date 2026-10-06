# Benchmarks

System and toolchain information is recorded in
[`system-info.md`](system-info.md) so that benchmark results remain
comparable. Compiler optimization experiments are recorded in
[`compiler-optimization-notes.md`](compiler-optimization-notes.md).

The benchmark executable reports CSV to standard output:

```text
width,samples_per_pixel,max_depth,iteration,seconds,rays_per_second
```

Run the default workload with:

```bash
make benchmark
```

Override width, samples per pixel, maximum ray depth, and iteration count:

```bash
make benchmark BENCHMARK_ARGS="1920 100 50 5"
```

For optimized measurements:

```bash
make clean
make CXXFLAGS="-std=c++17 -O3 -march=native"
make benchmark BENCHMARK_ARGS="1920 100 50 5"
```

Record the compiler, flags, CPU, workload, and measured rows in
`benchmarks/results.csv` for each tagged project version.
