# Roadmap

## v1.0 release candidate scope

- [x] RGBA8 checked storage, P3/P6 PPM, PNG and JPEG input/output.
- [x] Gaussian blur, Sobel, brightness/contrast/saturation, grayscale, sharpen, invert, nearest and bilinear resize.
- [x] Ordered filter pipelines, compatible single-effect flags, and safe non-recursive batches.
- [x] Numerical, malformed-input, codec, bounds, and CLI integration coverage.
- [x] Observable-checksum CSV benchmarks and executable-generated visual samples.
- [x] Linux/Windows Debug and Release CI, sanitizer run, and downloadable smoke-tested release-candidate packages.
- [x] Usage, codec limitations, licensing, and validation documentation.
- [ ] Windows native GUI milestone: Win32 controls and GDI+ preview, backed by the existing processing library, with CI Debug/Release and extracted-package smoke checks. Linux remains CLI-only.

This is a release candidate; the published GitHub Release remains an owner decision. The Windows GUI is a separate executable and does not change the CLI/core contracts. Future optimization should follow representative profiling and preserve scalar reference tests. GPU acceleration, metadata/color management, and additional codecs/effects are outside this milestone.
