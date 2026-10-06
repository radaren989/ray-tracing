# Benchmark System Information

This file records the hardware and software environment used for the
benchmarks. Because machine-specific information can change, update this file
when using new hardware, an operating system, or a compiler, and record the
benchmark results as a separate configuration.

## Recording date

- Date: 2026-10-06
- Project architecture: `x86_64`

## CPU

| Property | Value |
|---|---|
| Model | AMD Ryzen 5 5600H with Radeon Graphics |
| Vendor | AuthenticAMD |
| CPU family / model / stepping | 25 / 80 / 0 |
| Socket | 1 |
| Physical cores | 6 |
| Logical CPUs / threads | 12 |
| Threads per core | 2 |
| NUMA nodes | 1 |
| Online CPU list | `0-11` |
| Frequency boost | Enabled |
| Reported maximum frequency | 3301.0000 MHz |
| Reported minimum frequency | 412.6250 MHz |
| L1 data cache | 192 KiB total, 6 instances |
| L1 instruction cache | 192 KiB total, 6 instances |
| L2 cache | 3 MiB total, 6 instances |
| L3 cache | 16 MiB, 1 instance |
| Virtualization | AMD-V |

### CPU instruction-set features

The main features relevant to benchmark compilation and SIMD experiments are:

```text
SSE, SSE2, SSE3, SSSE3, SSE4.1, SSE4.2, SSE4a
AVX, AVX2, FMA, F16C
BMI1, BMI2, AES, VAES
PCLMULQDQ, POPCNT, RDRAND, RDSEED
```

The complete CPU flag list was collected from `lscpu`. With `-march=native`,
the compiler can use instruction-set features supported by this machine; the
resulting binary may not be portable to other CPUs.

## Operating system and kernel

| Property | Value |
|---|---|
| Distribution | CachyOS Linux |
| Distribution base | Arch Linux (`ID_LIKE=arch`) |
| Release model | Rolling |
| Kernel family | Linux 7.2 |
| Kernel architecture | `x86_64` |

## Compiler and build tools

| Tool | Version |
|---|---|
| GNU C++ compiler (`g++`) | 16.2.1 |
| GCC build date | 2026-08-10 |
| GNU Make | 4.4.1 |
| Make target | `x86_64-pc-linux-gnu` |
| C++ standard | C++17 |

Compiler version:

```text
g++ (GCC) 16.2.1 20260810
```

Baseline warning flags:

```text
-Wall -Wextra -Wpedantic
```

Optimization configurations used for benchmark comparisons:

```text
-O0 -DNDEBUG
-O2 -DNDEBUG
-O3 -march=native -DNDEBUG
-O3 -march=native -ffast-math -DNDEBUG
```

Because `-ffast-math` relaxes floating-point semantics, its performance results
must be evaluated together with image correctness.

## Benchmark workload

The current results were collected with the following common workload:

| Parameter | Value |
|---|---:|
| Width | 320 |
| Samples per pixel | 10 |
| Maximum ray depth | 20 |
| Iterations | 3 |

The benchmark executable sends the rendered image to `/dev/null` and reports
the following CSV fields:

```text
width,samples_per_pixel,max_depth,iteration,seconds,rays_per_second
```

## Reproducibility

Because CPU boost and frequency scaling are enabled, CPU temperature,
background load, and the power profile can affect the results. For more
reliable comparisons:

1. Run `make clean` before each compiler configuration.
2. Use the same workload and iteration count.
3. Confirm the expected `-O` and `-march` flags in the build output.
4. Treat the first run as a warm-up measurement.
5. Record every iteration alongside the average.
6. Keep the CPU governor, temperature, and background load as consistent as possible.
