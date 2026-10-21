# Roadmap

## v1.0 release candidate scope

- [x] RGBA8 checked storage, P3/P6 PPM, PNG and JPEG input/output.
- [x] Gaussian blur, Sobel, brightness/contrast/saturation, grayscale, sharpen, invert, nearest and bilinear resize.
- [x] Ordered filter pipelines, compatible single-effect flags, and safe non-recursive batches.
- [x] Numerical, malformed-input, codec, bounds, and CLI integration coverage.
- [x] Observable-checksum CSV benchmarks and executable-generated visual samples.
- [x] Linux/Windows Debug and Release CI, sanitizer run, and downloadable smoke-tested release-candidate packages.
- [x] Usage, codec limitations, licensing, and validation documentation.

This is a release candidate; the published GitHub Release remains an owner decision. Future optimization should follow representative profiling and preserve scalar reference tests. GPU acceleration, graphical interfaces, metadata/color management, and additional codecs/effects are outside v1.0.
