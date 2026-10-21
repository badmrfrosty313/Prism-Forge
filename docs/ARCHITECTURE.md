# Architecture

`prismforge_core` provides checked RGBA8 image storage, decoding/encoding, filters, and pipeline composition independently of the CLI. The CLI handles single-image and non-recursive batch workflows. The benchmark uses the same core operations.

## Image model and codecs

Pixels are packed RGBA8 with opaque alpha by default. `Image` enforces 16,384 pixels per axis and a 16 Mi-pixel allocation budget. Input files are limited to 64 MiB before reading. `load_image` detects P3/P6 PPM, PNG, or JPEG by content. `save_image` selects P6 PPM, PNG, or JPEG by extension. PPM token parsing consumes only the required P6 raster separator, treating CRLF as one separator so leading whitespace-valued binary samples remain valid.

PNG/JPEG use stb_image v2.30 and stb_image_write v1.16, pinned together to upstream commit `a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d`. Only PNG/JPEG codecs are compiled. The notices and pin are in `third_party/stb`. The codec is not independently audited. No metadata is retained or interpreted: EXIF orientation is ignored, ICC profiles are ignored, and no color conversion is attempted. PNG alpha is loaded and saved; PPM and JPEG output reject non-opaque pixels.

## Filters and pipeline

Filters allocate one output image at a time and leave their input unchanged. Gaussian blur builds a normalized one-dimensional kernel and performs horizontal then vertical convolution with replicated edge pixels. Sobel computes Rec. 601 luma, applies 3x3 gradients with replicated edges, and clamps Euclidean magnitude to grayscale. Color adjustment applies saturation around luma, contrast around 127.5, then additive brightness, with rounding and byte clamping. Grayscale uses Rec. 601 luma; sharpening uses center plus `amount * (4*center - north - south - east - west)` with replicated neighbors; inversion affects RGB. Resize uses pixel-center mapping with nearest-neighbor or bilinear interpolation, including alpha.

`FilterSpec` is a small typed representation parsed from names and named arguments. Applying the vector processes filters sequentially and retains only the current image and next output. An empty vector is identity.

## CLI, tests and benchmark

The executable offers legacy single-effect flags, repeatable `--filter` pipelines, and non-recursive directory batches. Batch mode refuses input/output directory overlap and existing files unless `--overwrite` is explicit; unsupported extensions are skipped and failures contribute to a nonzero result.

CMake builds the CLI, unit checks, CLI integration, and benchmark. The unit executable reports its assertion count. CLI integration exercises input/output format conversion, pipelines, failure exit codes, batch behavior, and overwrite rules. The benchmark emits CSV; it hashes dimensions and RGBA output bytes with FNV-1a after timing, then feeds the checksum to a volatile sink. GitHub Actions stores JUnit XML, benchmark CSV and executable-generated example images for the Linux/Windows Debug/Release matrix, and packages extracted-smoke-tested release candidates.
