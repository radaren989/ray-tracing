# Benchmark Notes

## Project version

- Version: `v0.1.0`
- Status: Initial CPU renderer benchmark baseline
- Workload recorded below: 320 width, 10 samples per pixel, maximum depth 20

This version identifies the project state used for the measurements in this
document. Future renderer, scene, build-system, or optimization changes should
use a new version identifier and add a separate benchmark entry rather than
overwriting these results.

## Workload

| Parameter | Value |
|---|---:|
| Width | 320 |
| Samples per pixel | 10 |
| Maximum depth | 20 |
| Iterations | 3 |

Each row reports one render iteration. Higher `rays_per_second` and lower
`seconds` are better.

## Results

| Configuration | Iteration 1 (s) | Iteration 2 (s) | Iteration 3 (s) | Average (s) | Average rays/s |
|---|---:|---:|---:|---:|---:|
| `-O0 -DNDEBUG` | 33.6122 | 32.8804 | 33.1481 | 33.2136 | 17,343.7 |
| `-O2 -DNDEBUG` | 8.00035 | 7.69635 | 7.75518 | 7.8173 | 73,703.5 |
| `-O3 -march=native -DNDEBUG` | 8.28309 | 8.11163 | 8.13816 | 8.1776 | 70,442.2 |
| `-O3 -march=native -ffast-math -DNDEBUG` | 7.95976 | 7.66823 | 7.81686 | 7.8149 | 73,722.0 |

## Relative comparison

Compared with the `-O0` baseline:

| Configuration | Speedup |
|---|---:|
| `-O2 -DNDEBUG` | 4.25x |
| `-O3 -march=native -DNDEBUG` | 4.06x |
| `-O3 -march=native -ffast-math -DNDEBUG` | 4.25x |

For this workload, `-O2` was slightly faster than `-O3 -march=native`.
`-ffast-math` recovered the `-O2` performance, but it relaxes floating-point
semantics and must be validated for image correctness before becoming a
default configuration.

## Reproducibility note

The captured `g++` commands in the original terminal output show
`-std=c++17 -Wall -Wextra -Wpedantic`, but do not show the reported
optimization flags. This means the results above should be treated as recorded
measurements, not yet as a verified compiler-flag comparison.

For a valid comparison, clean the build before every configuration and confirm
the optimization flag appears in the compile command:

```bash
make clean
make CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O0 -DNDEBUG"
make benchmark BENCHMARK_ARGS="320 10 20 3"

make clean
make CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG"
make benchmark BENCHMARK_ARGS="320 10 20 3"

make clean
make CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O3 -march=native -DNDEBUG"
make benchmark BENCHMARK_ARGS="320 10 20 3"
```

Record the CPU model, compiler version, operating-system state, and exact
compiler flags with future benchmark runs.
