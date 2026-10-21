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

Only PPM is supported: binary P6 and ASCII P3 input with max sample value 255; output is binary P6. PPM input is limited to 16,384 pixels per axis and 16 Mi pixels total before memory allocation. PNG, JPEG, alpha channels, color profiles, and metadata are not supported by the application. Processing uses 8-bit RGB pixels and is single-threaded. Radius is deliberately capped at 128 to bound work and allocation.

## Tests and examples

CTest runs algorithm and file-format checks plus end-to-end CLI checks for each effect and invalid arguments. PPM parsing enforces a 16 Mi pixel safety budget. The original sample is reproducible with `python3 tools/generate_sample.py`. GitHub Actions builds and tests Debug and Release on Linux and Windows, verifies expected artifact files, then uploads the input, filter PPM outputs, PNG previews, JUnit report, and benchmark log.

![Original synthetic sample](examples/previews/original.png)

| Gaussian blur | Sobel edges | Color adjustment |
| --- | --- | --- |
| ![Blur preview](examples/previews/blur.png) | ![Sobel preview](examples/previews/sobel.png) | ![Color preview](examples/previews/color.png) |

These browser-viewable PNG previews are generated from the C++ application's PPM outputs with a small Python standard-library converter. They are documentation assets only; the application still reads P3/P6 PPM and writes P6 PPM. The original and processed PPM files remain in [`examples`](examples/).

## Benchmark

Build `prismforge_bench` (enabled by default), then run `./build/prismforge_bench [width] [height] [repetitions]`. Defaults are 256x256 and five repetitions. It reports compiler, build type, dimensions, parameters, elapsed filter time, mean per-run time, and an FNV-1a checksum over dimensions and output RGB bytes. Each result is hashed after its timed interval and written to a volatile checksum sink, so every output remains observably consumed while checksum overhead stays outside the measured interval. Timings are informational, vary by machine, and are not a CI threshold.
