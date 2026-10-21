# Prism-Forge

Prism-Forge is a small C++17 CPU image-processing lab. This first milestone provides a testable RGB image type, separable Gaussian blur, Sobel edge magnitude, color adjustment, a command-line tool, PPM file I/O, deterministic sample images, tests, and a lightweight benchmark.

## Build

Requirements: CMake 3.16+ and a C++17 compiler (GCC, Clang, or MSVC). No third-party C++ dependencies are required.

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Use `--config Release` with Visual Studio or another multi-configuration generator. Switch `Release` to `Debug` to build the debug configuration.

## Command line

```sh
./build/prismforge --input examples/synthetic.ppm --output blurred.ppm \
  --effect blur --radius 3 --sigma 1.5
./build/prismforge --input examples/synthetic.ppm --output edges.ppm --effect sobel
./build/prismforge --input examples/synthetic.ppm --output adjusted.ppm \
  --effect color --brightness 12 --contrast 1.1 --saturation 1.25
```

Effects are `blur`, `sobel`, and `color`. Blur radius is 0–128 and sigma must be finite and positive. Color brightness is an additive 8-bit intensity offset; contrast is a multiplier around 127.5; saturation is a multiplier around Rec. 601 luma. Contrast and saturation must be nonnegative. Outputs are rounded and clamped to 0–255. Gaussian blur and Sobel use replicated edge pixels. Sobel uses a 3x3 kernel, Rec. 601 luma, Euclidean gradient magnitude, and clamps magnitude to 8-bit grayscale.

## Formats and limits

Only PPM is supported: binary P6 and ASCII P3 input with max sample value 255; output is binary P6. PNG, JPEG, alpha channels, color profiles, and metadata are not supported. Processing uses 8-bit RGB pixels and is single-threaded. Radius is deliberately capped at 128 to bound work and allocation.

## Tests and examples

CTest runs algorithm-level checks for kernel normalization, known patterns, identities, clamping, deterministic output, dimensions, invalid parameters, border behavior, and file errors. The original sample is reproducible with `python3 tools/generate_sample.py`. GitHub Actions builds and tests Debug and Release on Linux and Windows and uploads filter outputs as artifacts.

![Original synthetic sample](examples/synthetic.ppm)

Generated PPM outputs are available in [`examples/outputs`](examples/outputs/): [blur](examples/outputs/blur.ppm), [Sobel](examples/outputs/sobel.ppm), and [color adjustment](examples/outputs/color.ppm). CI also uploads the images and JUnit test reports as run artifacts.

## Benchmark

Build `prismforge_bench` (enabled by default), then run `./build/prismforge_bench [width] [height] [repetitions]`. Defaults are 256x256 and five repetitions. It reports compiler, build type, dimensions, parameters, total duration, and mean duration for each actual filter run. Timings are informational and vary by machine; they are not a CI threshold.
