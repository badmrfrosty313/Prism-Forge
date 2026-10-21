# Architecture

`prismforge_core` provides checked RGBA8 image storage, decoding/encoding, filters, and pipeline composition independently of the applications. The portable CLI handles single-image and non-recursive batch workflows on Linux and Windows. A Windows-only Win32 GUI uses the same core library. The benchmark also uses those core operations.

## Image model and codecs

Pixels are packed RGBA8 with opaque alpha by default. `Image` enforces 16,384 pixels per axis and a 16 Mi-pixel allocation budget. Input files are limited to 64 MiB before reading. `load_image` detects P3/P6 PPM, PNG, or JPEG by content. `save_image` selects P6 PPM, PNG, or JPEG by extension. PPM token parsing consumes only the required P6 raster separator, treating CRLF as one separator so leading whitespace-valued binary samples remain valid.

PNG/JPEG use stb_image v2.30 and stb_image_write v1.16, pinned together to upstream commit `a7d4c2175630e1ea5b6f8dd00ab96fed0b97b96d`. Only PNG/JPEG codecs are compiled. The notices and pin are in `third_party/stb`. The codec is not independently audited. No metadata is retained or interpreted: EXIF orientation is ignored, ICC profiles are ignored, and no color conversion is attempted. PNG alpha is loaded and saved; PPM and JPEG output reject non-opaque pixels.

## Filters and pipeline

Filters allocate one output image at a time and leave their input unchanged. Gaussian blur builds a normalized one-dimensional kernel and performs horizontal then vertical convolution with replicated edge pixels. Sobel computes Rec. 601 luma, applies 3x3 gradients with replicated edges, and clamps Euclidean magnitude to grayscale. Color adjustment applies saturation around luma, contrast around 127.5, then additive brightness, with rounding and byte clamping. Grayscale uses Rec. 601 luma; sharpening uses center plus `amount * (4*center - north - south - east - west)` with replicated neighbors; inversion affects RGB. Resize uses pixel-center mapping with nearest-neighbor or bilinear interpolation, including alpha.

`FilterSpec` is a small typed representation parsed from names and named arguments. Applying the vector processes filters sequentially and retains only the current image and next output. An empty vector is identity.

## CLI, tests and benchmark

The executable offers legacy single-effect flags, repeatable `--filter` pipelines, and non-recursive directory batches. Batch mode refuses input/output directory overlap and existing files unless `--overwrite` is explicit; unsupported extensions are skipped and failures contribute to a nonzero result. It preflights output names across supported source files to reject cross-extension collisions before any image is processed, even with `--overwrite`.

CMake builds the CLI, unit checks, CLI integration, and benchmark. The unit executable reports its assertion count. CLI integration exercises input/output format conversion, pipelines, failure exit codes, batch behavior, and overwrite rules. The benchmark emits CSV; it hashes dimensions and RGBA output bytes with FNV-1a after timing, then feeds the checksum to a volatile sink. GitHub Actions stores JUnit XML, benchmark CSV and executable-generated example images for the Linux/Windows Debug/Release matrix, and packages extracted-smoke-tested release candidates.

## Windows desktop GUI

`prismforge_gui` is built only under `WIN32` as the Windows-subsystem executable `PrismForge-GUI.exe`, so launching it does not create a console window. Native buttons, combo boxes, edit fields and a list box implement Open, filter configuration, ordered history, remove/reorder, undo/reset, and Save As. Standard Windows common file dialogs retain UTF-16 paths. GDI+ draws a centered aspect-preserving preview; a worker thread handles decoding, full-resolution filter pipelines, preview downsampling and saving. Preview buffers are capped at 1500×1100 pixels, while source and output image dimensions use the core's existing safety bounds.

Edits are stored as a filter list and recomputed from the original on each change, so undo/remove/reorder does not keep multiple full-size snapshots. The GUI's `--smoke-test input output` mode initializes a hidden native window and controls, runs a grayscale/sharpen pipeline through `prismforge_core`, creates a preview, saves through the core codec, and reopens the output. This validates initialization and the model/core path, not human mouse/keyboard interaction or visual quality on a physical Windows desktop.
