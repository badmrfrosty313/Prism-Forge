# Portfolio and interview notes

These bullets describe this project accurately. Prism-Forge was developed with AI assistance; the implementation was built and tested in a Codex cloud workspace and independently exercised by the repository's automated test suites and GitHub-hosted CI. This is a personal portfolio project, not professional employment.

- Built a C++17 CPU image-processing CLI for PNG, JPEG, and PPM with checked RGBA storage, bounded decoding, alpha-aware filters, ordered pipelines, and safe non-recursive batch processing.
- Integrated pinned stb PNG/JPEG codecs and implemented Gaussian blur, Sobel, color transforms, grayscale, sharpening, inversion, and nearest/bilinear resize with numerical and end-to-end tests.
- Set up GitHub Actions Debug/Release validation across Ubuntu and Windows, Linux ASan/UBSan checks, checksum-observable CSV benchmarks, and extracted-package smoke tests for downloadable release candidates.
