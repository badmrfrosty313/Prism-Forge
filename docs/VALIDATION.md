# Validation record

## Previous verified CI run

The baseline commit `618b84bf2ca01d6fe65aaebd02eb35efc5fe9193` passed the four-job Ubuntu/Windows × Debug/Release matrix in [GitHub Actions run 39644847413](https://github.com/badmrfrosty313/Prism-Forge/actions/runs/39644847413). Each job uploaded a run artifact; the original workflow uploaded only PPM files, which this hardening change corrects.

## Hardening changes: local validation

- Environment: Debian 13 x86_64 cloud environment, GCC 14.2.0, CMake 4.4.4.
- Debug configure/build passed with `-Wall -Wextra -Wpedantic -Wconversion -Wsign-conversion`; CTest passed 2/2 tests. `prismforge_tests` reported 36 checks passed.
- Release configure/build passed with the same strict warnings; CTest passed 2/2 tests. `prismforge_tests` reported 36 checks passed.
- Unit coverage includes P3 comments and samples; P6 roundtrip and CRLF headers with whitespace-valued first raster bytes; malformed/truncated headers and rasters; negative, zero, oversized dimensions; dimension budget before allocation; invalid maxval and out-of-range P3 samples; pixel bounds; filter results and parameters; and file failures.
- CLI integration invoked blur, Sobel, and color and verified each wrote a changed P6 result. It also verified exit status 2 for an unknown effect, unknown option, and missing input.
- PNG converter generated 96×64 RGB previews for the original and all three actual PPM filter results using only Python's standard library; `file` identified all four as valid PNG images.
- Release benchmark: GCC 14.2.0, 128×96 image, three repetitions. Blur (radius 3, sigma 1.5): 1.018 ms/filter run, checksum `16744905697689323037`; Sobel: 0.787012 ms/filter run, checksum `16639403792430811153`; color (brightness 8, contrast 1.1, saturation 1.2): 0.299192 ms/filter run, checksum `14169378915435519602`. Checksums include dimensions and output RGB bytes via FNV-1a and are consumed through a volatile sink after each timed call. Times exclude hashing and are environment-specific; CI has no timing threshold.
- Local artifact rehearsal verified nonempty JUnit XML, benchmark log, input and processed PPMs, and four PNG previews.

## Current remote verification

The hardening changes are being checked by the four-job GitHub Actions matrix. Update this section with the new run URL, all four job results, and artifact bundle names after the run completes. Do not infer remote results from local builds.

## Supported formats and limits

The C++ application reads P3/P6 PPM with max value 255 and writes P6 PPM. Input dimensions are limited to 16,384 per axis and 16 Mi pixels total. PNG files are documentation previews only; PNG/JPEG application I/O is not implemented.
