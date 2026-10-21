# Engineering rules

- Keep the core C++17 and retain the pinned, license-compatible stb PNG/JPEG codec unless an explicit review justifies a replacement.
- Preserve separation between image data, algorithms, codecs, pipeline, CLI, Windows-only GUI, benchmark, and tests.
- Algorithms must remain directly testable in `prismforge_core`.
- Do not claim support for formats or platforms that have not been implemented and checked.
- Keep compiler warnings enabled and resolve new warnings.
- Run CMake configure/build and CTest after C++ changes, including the CLI integration check; record actual outcomes in `docs/VALIDATION.md`.
- Keep all image reads within the documented encoded-size and 16 Mi pixel bounds; test malformed data before changing codecs/parsers.
- Keep benchmarks descriptive, consume each output through the checksum sink, and never make timing a pass/fail threshold.
- Generate gallery images with the Prism-Forge executable and verify the PNG files are valid.
- Keep sample assets original and generated from `tools/generate_sample.py`.
- Keep the Windows GUI native Win32/GDI+ and reuse `prismforge_core`; do not add Electron, Qt, GPU, or unrelated features.
- Keep Linux GUI-free; verify Windows GUI builds in Debug and Release and run its non-interactive `--smoke-test` mode after packaging.
