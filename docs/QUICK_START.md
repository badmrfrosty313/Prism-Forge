# Quick start

## Windows desktop workflow

Extract the Windows ZIP and double-click `PrismForge-GUI.exe` (also linked from `START_HERE.txt`). Choose **Open Image**, select a PNG, JPEG or PPM file, choose a filter, set its parameters, then press **Apply Filter**. Add additional filters to build an ordered list. Use **Move up**, **Move down**, **Remove**, **Undo last** or **Reset** to manage it. **Save As** lets you choose PNG, JPEG or PPM; JPEG quality defaults to 90. If a transparent image is saved as JPEG/PPM, the app asks before flattening transparency onto white.

The Windows GUI stays responsive by processing image I/O and filters in a background worker. Image preview is reduced to at most 1500×1100 pixels while the saved output retains its full dimensions. The GUI needs no terminal, Python, compiler or separate framework.

Linux is CLI-only. The v1.0 CLI remains available on Windows as `bin/prismforge.exe` and on Linux as `bin/prismforge`.

## Command-line workflow

Run `prismforge --help` for the complete syntax.

```sh
prismforge --input photo.png --output gray.png --filter grayscale
prismforge --input photo.jpg --output enhanced.png \
  --filter 'blur:radius=3,sigma=1.5' --filter 'sharpen:amount=0.8'
prismforge --input-dir photos --output-dir processed --filter grayscale --format png
```

Inputs are detected from their file contents (PNG, JPEG, P3/P6 PPM). Output type is selected from `.png`, `.jpg`/`.jpeg`, or `.ppm`; JPEG quality defaults to 90 and may be set with `--quality 1..100`. PPM output is binary P6. Existing output files are protected unless `--overwrite` is supplied. Batch mode scans one directory level and skips unsupported extensions with a summary.

The binary has no Python, OpenCV, GUI, or GPU runtime dependency. Windows and Linux release candidates are built and smoke-tested by GitHub Actions; see [`RELEASE.md`](RELEASE.md).
