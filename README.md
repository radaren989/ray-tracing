# Ray Tracing HPC

This project is a C++ ray tracer designed to study correctness, performance,
and scalability. It initially follows the CPU-based renderer approach from
*Ray Tracing in One Weekend*.

## Goal

The long-term goal is to study ray tracing from an HPC perspective:

- Optimize mathematical and geometric operations
- Parallelize the rendering workload
- Measure and compare SIMD, OpenMP, and related techniques
- Reduce ray-intersection costs with acceleration structures such as BVH
- Track the impact of every change with reproducible benchmarks

The project prioritizes measurement over assumptions. Every performance change
should be benchmarked with the same scene, resolution, sample count, and ray
depth.

## Project structure

```text
src/
├── app/        # Main renderer application
├── benchmark/  # Performance measurement application
├── math/       # Core types such as vec3, ray, and interval
├── renderer/   # Camera and color output
└── scene/      # Geometry, materials, and scene construction

benchmarks/     # Results, system information, and benchmark notes
build/          # Build artifacts
output/         # PPM render output
```

## Build and run

```bash
make
make run
make clean
```

The render output is written to `output/image.ppm`, and the executable is
written to `build/ray-tracing`.

## Benchmark

The benchmark produces CSV output:

```bash
make benchmark BENCHMARK_ARGS="320 10 20 3"
```

The argument order is:

```text
width samples_per_pixel max_depth iterations
```

When comparing optimization flags, perform a clean build before each
configuration:

```bash
make clean
make CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG"
make benchmark BENCHMARK_ARGS="320 10 20 3"
```

Details about the current system, compiler, and benchmark environment are
available in
[`benchmarks/system-info.md`](benchmarks/system-info.md).
Measurement results and compiler optimization experiments are recorded in
[`benchmarks/compiler-optimization-notes.md`](benchmarks/compiler-optimization-notes.md).