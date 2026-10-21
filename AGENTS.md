# Engineering rules

- Keep the core dependency-free C++17 unless a reviewed requirement justifies a dependency.
- Preserve the separation between image data, algorithms, PPM I/O, CLI, benchmark, and tests.
- Algorithms must remain directly testable in `prismforge_core`.
- Do not claim support for formats or platforms that have not been implemented and checked.
- Keep compiler warnings enabled and resolve new warnings.
- Run CMake configure/build and CTest after C++ changes, including the CLI integration check; record actual outcomes in `docs/VALIDATION.md`.
- Keep PPM reads within the documented 16 Mi pixel bound and test malformed headers before changing the parser.
- Keep benchmarks descriptive, consume each output through the checksum sink, and never make timing a pass/fail threshold.
- Keep documentation PNG previews generated from real PPM results; they do not extend application codec support.
- Keep sample assets original and generated from `tools/generate_sample.py`.
- Do not add GPU, GUI, or unrelated product features to this milestone.
