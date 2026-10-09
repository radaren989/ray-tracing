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

## AABB hot-path optimization

The near-child-first experiment was measured but not retained. Although it
passed correctness validation, it increased the clean-build average from
1.30529 s to 1.50788 s because the extra child-ordering calculation was more
expensive than the pruning benefit for this scene.

The retained change simplified `aabb::hit()` by removing its axis loop and
processing the three stored intervals directly. The ray-parallel containment
check and interval narrowing semantics were preserved.

With `-std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG` and workload
`320 10 20 3`, the measured render time improved from 1.23661 s to 1.14928 s.
The three post-change iterations were 1.15445 s, 1.13971 s, and 1.15368 s,
for approximately 501,200 rays/s on average. `make test` continued to pass
for 221 deterministic rays.

## Thread parallelism

The render loop was parallelized after the BVH and AABB optimizations. Worker
threads claim scanlines through an atomic counter and write only to their own
rows in a pixel buffer. The main thread serializes the PPM output after all
workers join. Random sampling uses thread-local `std::mt19937` state instead
of global `std::rand()`.

With `-O2 -DNDEBUG` and workload `320 10 20 3`, the measured scaling was:

| Workers | Average seconds | Speedup | Efficiency | Cost | Overhead |
|---:|---:|---:|---:|---:|---:|
| 1 | 1.264233 | 1.00x | 100.0% | 1.264233 | 0.000000 |
| 2 | 0.660122 | 1.92x | 96.0% | 1.320244 | 0.056011 |
| 4 | 0.341575 | 3.70x | 92.5% | 1.366300 | 0.102067 |
| 6 | 0.233677 | 5.41x | 90.2% | 1.402062 | 0.137829 |
| 12 | 0.192235 | 6.58x | 54.8% | 2.306820 | 1.042587 |

The one-worker average is the baseline:

```text
Speedup(p)    = T1 / Tp
Efficiency(p) = Speedup(p) / p * 100
Cost(p)       = p * Tp
Overhead(p)   = Cost(p) - T1
```

For six workers:

```text
Speedup    = 1.264233 / 0.233677 = 5.41x
Efficiency = 5.41 / 6 * 100     = 90.2%
Cost       = 6 * 0.233677        = 1.402062
Overhead   = 1.402062 - 1.264233 = 0.137829
```

Six workers use the six physical cores efficiently. Twelve workers reduce
the total time further, but the second logical thread on each physical core
shares execution units and cache, so efficiency drops. Small values above
100% efficiency or negative overhead are measurement noise from CPU boost,
scheduling, cache state, temperature, or background activity. More
iterations and a heavier workload produce more stable estimates.

## Parallel perf profiling

The parallel renderer was profiled after adding configurable worker counts.
Both profiles used `-O2 -g -fno-omit-frame-pointer`, the BVH scene, workload
`320 10 20 3`, and user-space cycle sampling:

```bash
perf record -e cycles:u -F 99 --call-graph fp \
  ./build/benchmark 320 10 20 3 6
perf record -e cycles:u -F 99 --call-graph fp \
  ./build/benchmark 320 10 20 3 12
```

| Worker threads | Samples | `aabb::hit` samples | Main caller |
|---:|---:|---:|---|
| 6 | 572 | 62.92% | recursive `bvh_node::hit` |
| 12 | 751 | 60.84% | recursive `bvh_node::hit` |

The thread count changes throughput but does not change the dominant
algorithmic hotspot: AABB intersection remains responsible for roughly
61-63% of sampled cycles. The profile also shows repeated
`shared_ptr<hittable>::operator bool()` checks inside recursive BVH traversal,
but these are secondary to the AABB cost.

The next optimization should therefore target ray/AABB arithmetic, especially
reducing division work or caching per-ray inverse directions while preserving
the zero-direction containment check. BVH pointer layout and `shared_ptr`
indirection can be investigated after the AABB change is benchmarked.

### Inverse-direction experiment

Caching `1 / direction` in each `ray` and replacing AABB divisions with
multiplications was tested, but rejected. The cache added three divisions to
every ray construction, including recursive scattered rays. With six workers
and workload `320 10 20 3`, the clean-build average was approximately
0.296 s, compared with the previous approximately 0.234 s baseline. The
experiment therefore increased total render time by roughly 26%; the
inverse-direction fields were removed and the validated baseline was kept.

## SoA packet prototype

An 8-lane structure-of-arrays packet prototype was added without changing
the renderer or BVH traversal. `ray_packet` stores origin and direction
components in separate arrays:

```text
origin_x[8], origin_y[8], origin_z[8]
direction_x[8], direction_y[8], direction_z[8]
```

`aabb::hit_packet()` processes all eight lanes and returns one hit mask per
lane. The test compares every packet result with the scalar AABB
implementation, including parallel-direction edge cases:

```text
Packet AABB correctness passed for 8 lanes
BVH correctness passed for 221 rays
```

This is currently a data-layout and correctness prototype, not an
optimization: it still performs scalar work per lane and is not integrated
into BVH traversal.

### AVX2 packet prototype

`aabb::hit_packet_avx2()` now processes the eight lanes as two AVX2 vectors of
four doubles. It uses vector min/max/division operations and preserves the
scalar parallel-direction containment rule. Builds without `-mavx2` use the
validated scalar packet implementation as a fallback, while the explicit AVX2
path can be tested with:

```bash
make avx2-test
```

Both the default build and the `-mavx2` build currently pass the same
correctness checks:

```text
Packet AABB correctness passed for 8 lanes (scalar and AVX2)
BVH correctness passed for 221 rays
```

This is still only an isolated correctness experiment. It has not been
connected to BVH traversal. The isolated throughput benchmark is run with:

```bash
make clean
make packet-benchmark CXXFLAGS="-std=c++17 -O2 -Wall -Wextra -Wpedantic -mavx2"
```

It compares scalar eight-ray processing with the AVX2 path and verifies that
both checksums match. The result is intentionally not used as a renderer
speedup claim: BVH traversal, ray divergence, and recursive material paths
still need a packetized design before integration.

One run with `-O2 -mavx2`, 65,536 packets, and 100 repetitions produced:

| Implementation | Seconds | Packets/s | Hit checksum |
|---|---:|---:|---:|
| Scalar | 1.08082 | 6.06 million | 52,428,800 |
| AVX2 | 0.570082 | 11.50 million | 52,428,800 |

The isolated AABB packet operation was approximately 1.90x faster with AVX2
in this run. This does not predict whole-renderer speedup because the current
renderer still generates divergent recursive rays one at a time.

### Packet BVH traversal experiment

The packet benchmark was extended with an isolated BVH traversal prototype.
Internal BVH nodes use AVX2 AABB masks, while sphere leaves still use scalar
intersection. The packet result matched scalar traversal exactly:

```text
scalar BVH checksum: 1402300
packet BVH checksum: 1402300
```

However, the first implementation was slower for the test scene:

| Traversal | Seconds | Relative speed |
|---|---:|---:|
| Scalar BVH | 1.72775 | 1.00x |
| Packet AABB BVH | 6.71128 | 0.26x |

The slowdown is expected for this intermediate design. It performs
`dynamic_cast` checks during traversal, tests both child boxes for every
packet, and still falls back to one scalar sphere intersection per lane.
Therefore this version is retained as a correctness experiment, not as a
renderer optimization. A useful packet BVH needs explicit node types or
packet-aware child traversal, active-lane masks, and a strategy for handling
divergent rays before integration into the renderer.

### Active-lane mask refinement

The packet BVH was refined to carry an active lane mask through recursive
node visits. Lanes that miss a parent AABB are no longer tested against child
boxes or sphere leaves. A clean `-O2 -mavx2` run produced matching checksums
and improved the isolated BVH experiment:

| Traversal | Seconds | Relative speed |
|---|---:|---:|
| Scalar BVH | 0.725105 | 1.00x |
| Active-mask packet BVH | 0.606390 | 1.20x |

This is a positive isolated result, but it is not yet a renderer result.
The packet path still uses scalar sphere intersections and `dynamic_cast`
for recursive BVH nodes. Those costs should be removed before integrating it
with the production renderer.

### Virtual packet dispatch refinement

The packet interface now lives on `hittable`: leaf objects use a scalar
lane-by-lane fallback, while `bvh_node` overrides the active-mask traversal.
This removes `dynamic_cast` from packet recursion and keeps the traversal
polymorphic.

In a clean `-O2 -mavx2` run, checksums still matched:

```text
scalar BVH checksum: 1402300
packet BVH checksum: 1402300
```

Measured timings were:

| Traversal | Seconds | Relative speed |
|---|---:|---:|
| Scalar BVH | 0.723929 | 1.00x |
| Virtual-dispatch packet BVH | 0.674768 | 1.07x |

This is a smaller gain than the previous active-mask measurement, so the
experiment remains isolated. The renderer default is unchanged.

### Packet sphere intersection

Sphere leaves now override the packet hit interface with an AVX2 quadratic
intersection. The active lane mask is applied after the vector discriminant
and root tests, while the non-AVX2 build retains the scalar fallback.
Correctness is checked lane-by-lane against `sphere::hit()`.

With a clean `-O2 -mavx2` build, the packet BVH benchmark produced matching
checksums and the following timings:

| Traversal | Seconds | Relative speed |
|---|---:|---:|
| Scalar BVH | 0.738425 | 1.00x |
| Packet AABB + sphere AVX2 | 0.612914 | 1.20x |

This is an isolated traversal improvement, not a full renderer result.
Material scattering and recursive ray generation are still scalar and
divergent.

## Production renderer AVX2 comparison

The AVX2 packet code is not called by the normal renderer path. A clean
production benchmark was nevertheless compared with and without compiling
the whole program using `-mavx2`, using six workers and workload
`320 10 20 3 6`:

| Build | Iterations (s) | Average (s) |
|---|---|---:|
| `-O2 -DNDEBUG` | 0.238494, 0.229863, 0.231022 | 0.233126 |
| `-O2 -DNDEBUG -mavx2` | 0.267352, 0.260755, 0.254232 | 0.260780 |

The AVX2 build was approximately **11.9% slower** in this sample. This does
not mean the packet implementation is slowing the renderer: the scalar
renderer does not call `hit_packet_avx2()` or `sphere::hit_packet_active()`.
Compiling the whole program with `-mavx2` can change instruction selection
and can reduce CPU frequency on processors that apply an AVX operating-point
transition. The packet code should therefore remain in an explicitly
AVX2-targeted benchmark or function until it is integrated intentionally.

### Renderer integration experiment and rollback

An opt-in renderer path was tested that grouped primary rays into eight-lane
packets, used packet traversal as a hit precheck, and then kept scalar
recursive shading for hit lanes. This preserved the scalar shading path but
added a second traversal for every hit ray.

With six workers, workload `320 10 20 3`, and a clean `-O2 -mavx2` build:

| Renderer path | Average seconds |
|---|---:|
| Existing scalar renderer | approximately 0.263 |
| Packet hit precheck + scalar shading | approximately 0.637 |

The experimental path was approximately 2.43x slower, so it was removed from
the production camera. The AVX2 AABB, sphere, and isolated packet BVH
benchmarks remain available; a useful full renderer integration requires
packetized closest-hit records and recursive material/shading work rather
than a separate boolean precheck.

### Closest-hit packet integration and rollback

The boolean precheck was replaced with a closer experiment: packet BVH
traversal produced the nearest `hit_record` for every lane, and the renderer
used that record directly for primary-ray shading, avoiding a second primary
ray traversal. This was still slower in a clean `-O2 -mavx2` benchmark with
six workers and workload `320 10 20 3`:

| Renderer path | Average seconds |
|---|---:|
| Scalar renderer | approximately 0.229 |
| Closest-hit packet renderer | approximately 1.225 |

The packet renderer was approximately 5.35x slower. The production camera
and benchmark interface were therefore restored to the scalar implementation.
The packet closest-hit API remains an isolated experiment for future work;
it is not enabled by default.

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
