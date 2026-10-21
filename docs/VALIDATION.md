# Validation record

This file records commands actually run for the current implementation. Update it with the environment and exact outcomes after each validation run; do not infer CI results from the workflow definition.

## Current task

- Environment: Debian 13, GCC 14.2.0, CMake 4.4.4, x86_64 cloud instance.
- Debug: CMake configure and build passed with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`; CTest passed 1/1 test executable, which reported all 22 checks passed.
- Release: CMake configure and build passed with the same warnings; CTest passed 1/1 test executable, which reported all 22 checks passed.
- Functional CLI checks: blur, Sobel, and color adjustment each processed the 96x64 generated P6 sample and wrote output files.
- Benchmark: Release build, 128x96, 3 repetitions. Gaussian blur radius 3 sigma 1.5: 1.05168 ms/run; Sobel: 0.842712 ms/run; color (brightness 8, contrast 1.1, saturation 1.2): 0.346174 ms/run. GCC 14.2.0; single local cloud process. These timings are environment-specific.
- GitHub Actions Linux and Windows: configured for Debug and Release; results pending remote run.
- Supported I/O: P3/P6 PPM with max value 255. PNG/JPEG remain unsupported.
