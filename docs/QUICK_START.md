# Quick start

The Prism-Forge v1.0 release candidate is a standalone command-line program. Run `prismforge --help` for the complete syntax.

```sh
prismforge --input photo.png --output gray.png --filter grayscale
prismforge --input photo.jpg --output enhanced.png \
  --filter 'blur:radius=3,sigma=1.5' --filter 'sharpen:amount=0.8'
prismforge --input-dir photos --output-dir processed --filter grayscale --format png
```

Inputs are detected from their file contents (PNG, JPEG, P3/P6 PPM). Output type is selected from `.png`, `.jpg`/`.jpeg`, or `.ppm`; JPEG quality defaults to 90 and may be set with `--quality 1..100`. PPM output is binary P6. Existing output files are protected unless `--overwrite` is supplied. Batch mode scans one directory level and skips unsupported extensions with a summary.

The binary has no Python, OpenCV, GUI, or GPU runtime dependency. Windows and Linux release candidates are built and smoke-tested by GitHub Actions; see [`RELEASE.md`](RELEASE.md).
