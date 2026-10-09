# Benchmark Notes

## Project version

- Version: `v0.1.0`
- Status: Initial CPU renderer benchmark baseline
- Workload recorded below: 320 width, 10 samples per pixel, maximum depth 20

This version identifies the project state used for the measurements in this
document. Future renderer, scene, build-system, or optimization changes should
use a new version identifier and add a separate benchmark entry rather than
overwriting these results.

## CPU profiling snapshot

### Profiling configuration

- Project version: `v0.1.0`
- Tool: Linux `perf`
- Command:

  ```bash
  perf record -g ./build/benchmark 320 10 20 3
  ```

- Profile data: `perf.data`
- Samples: approximately 90,000 processed samples
- Lost samples: 4
- Event: `cpu/cycles/Pu`

The profile was collected from the single-threaded renderer. The small number
of lost samples should not change the overall hotspot ordering, but future
measurements should use a debug-symbol build with frame pointers enabled:

```bash
make clean
make benchmark-build \
  CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -g -fno-omit-frame-pointer"

perf record -e cycles:u -F 99 --call-graph fp \
  ./build/benchmark 320 10 20 1
```

### Observed hotspots

The profile shows that almost all execution time is spent in the rendering
path:

| Function or area | Approximate inclusive share |
|---|---:|
| `camera::render` | 99.98% |
| `camera::ray_color` | 99.54% |
| Recursive `ray_color` calls | 61.75% |
| `hittable_list::hit` | 37.19% |
| `sphere::hit` | 33.66% |
| `vec3::length_squared` | 2.0-3.5% |
| `dot` | 2.1-2.4% |
| `vec3` subtraction | 2.0-2.4% |

These are inclusive call-tree values; nested values must not be added
together. For example, `sphere::hit` is included within
`hittable_list::hit`.

### Profiling conclusion

The current dominant cost is not matrix arithmetic. It is recursive ray
tracing combined with linear traversal of every object in
`hittable_list::hit`. The next high-value single-thread optimization should be
an acceleration structure, most likely a BVH:

```text
linear hittable traversal
-> BVH
-> random number generation
-> ray/sphere intersection math
-> vec3/SIMD experiments
-> parallel rendering
```

The profiling snapshot is a `v0.1.0` baseline and should be repeated after
each major optimization.

## BVH profiling snapshot

The post-BVH profile was collected with `-O2 -g -fno-omit-frame-pointer`:

```bash
perf record -e cycles:u -F 99 --call-graph fp \
  ./build/benchmark 320 10 20 1
```

The dominant sampled function is now `aabb::hit`, at approximately 63% of
samples. Its callers are recursive `bvh_node::hit` calls, confirming that
the renderer is spending most of its traversal time testing bounding boxes
instead of testing every sphere. This is the expected cost shift after the
linear traversal bottleneck was removed.

The next optimization target should be BVH traversal itself: test the child
whose bounding box is entered first, then use the resulting hit distance to
prune the farther child. Only after that should lower-level AABB arithmetic
or SIMD changes be evaluated.

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

## BVH comparison

### Project version

- Version: `v0.2.0`
- Configuration: `-O2 -DNDEBUG`
- Workload: 320 width, 10 samples per pixel, maximum depth 20, 3 iterations
- Scene generation uses the default deterministic `std::rand()` sequence.
- Render timing excludes scene construction and BVH construction.

| Configuration | Iteration 1 (s) | Iteration 2 (s) | Iteration 3 (s) | Average (s) | Average rays/s |
|---|---:|---:|---:|---:|---:|
| Linear `hittable_list` | 7.71636 | 7.57228 | 7.62503 | 7.63789 | 75,418 |
| BVH | 1.32321 | 1.29561 | 1.29704 | 1.30529 | 441,324 |

BVH traversal is approximately **5.85x faster** than linear traversal for
this workload, with the same rendered scene and compiler configuration.
The comparison was run with:

```bash
make clean
make benchmark-build \
  CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG"
./build/benchmark 320 10 20 3
RAY_TRACING_SCENE=linear ./build/benchmark 320 10 20 3
```

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
