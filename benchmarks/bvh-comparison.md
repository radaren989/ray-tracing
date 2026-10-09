# BVH Before/After Comparison

## Scope

This note compares the renderer before and after adding a bounding volume
hierarchy (BVH). Both measurements use the same default scene, camera,
compiler configuration, and benchmark workload.

| Setting | Value |
|---|---|
| Compiler flags | `-std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG` |
| Width | 320 |
| Samples per pixel | 10 |
| Maximum depth | 20 |
| Iterations | 3 |
| Timed region | Rendering only; scene construction is excluded |

## What changed

Before BVH, every ray was tested against every object through linear
`hittable_list` traversal. The optimization introduced:

1. `aabb`, with slab-based ray/box intersection.
2. `bounding_box()` on the `hittable` interface.
3. Sphere and scene-list bounding boxes.
4. Recursive `bvh_node` construction using the longest axis and median split.
5. BVH traversal that skips nodes whose boxes miss and narrows the hit range
   after a closer intersection.
6. BVH scene integration in the renderer and benchmark.
7. A correctness test comparing linear and BVH hit distances for 221 rays.

The benchmark also gained `RAY_TRACING_SCENE=linear`, which makes the old
linear traversal available for an apples-to-apples comparison.

## Measurements

Commands used after a clean optimized build:

```bash
make clean
make benchmark-build \
  CXXFLAGS="-std=c++17 -Wall -Wextra -Wpedantic -O2 -DNDEBUG"
./build/benchmark 320 10 20 3
RAY_TRACING_SCENE=linear ./build/benchmark 320 10 20 3
```

| Traversal | Iteration 1 (s) | Iteration 2 (s) | Iteration 3 (s) | Average (s) | Average rays/s |
|---|---:|---:|---:|---:|---:|
| Before: linear `hittable_list` | 7.71636 | 7.57228 | 7.62503 | 7.63789 | 75,418 |
| After: BVH | 1.32321 | 1.29561 | 1.29704 | 1.30529 | 441,324 |

## Result

The BVH reduced average render time from **7.63789 s** to **1.30529 s**.
That is a **5.85x speedup** and an **82.9% reduction in render time**.
Throughput increased from **75,418 rays/s** to **441,324 rays/s**, also a
**5.85x improvement**.

The correctness test passed:

```text
BVH correctness passed for 221 rays
```

The post-BVH `perf` profile shifted the main sampled cost from linear object
traversal to `aabb::hit` at approximately 63% of samples. This confirms that
the BVH is being traversed and that the dominant cost has moved to bounding
box tests rather than sphere tests for every object.

## Reproducibility

Run the correctness check with:

```bash
make test
```

The current next optimization target is near-child-first traversal, followed
by reducing the cost of AABB intersection if profiling still identifies it
as the dominant bottleneck.
