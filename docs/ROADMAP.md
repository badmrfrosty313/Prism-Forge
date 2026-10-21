# Roadmap

## Completed in foundation

- C++17 image storage, Gaussian blur, Sobel magnitude, and brightness/contrast/saturation.
- P3/P6 PPM I/O, command-line interface, algorithm tests, generated sample, benchmark, and Linux/Windows CI definition.

## Candidate next milestones

1. Add a separately reviewed PNG/JPEG I/O dependency using supported prebuilt packages and test color/alpha semantics.
2. Add benchmark output in machine-readable CSV/JSON and repeatable workload presets.
3. Profile representative workloads before considering SIMD or thread-level parallelism, preserving scalar reference tests.

GPU acceleration, graphical interfaces, and unrelated integrations are outside the current scope.
