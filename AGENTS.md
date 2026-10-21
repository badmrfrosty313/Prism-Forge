# Engineering rules

- Keep the core dependency-free C++17 unless a reviewed requirement justifies a dependency.
- Preserve the separation between image data, algorithms, PPM I/O, CLI, benchmark, and tests.
- Algorithms must remain directly testable in `prismforge_core`.
- Do not claim support for formats or platforms that have not been implemented and checked.
- Keep compiler warnings enabled and resolve new warnings.
- Run CMake configure/build and CTest after C++ changes; record actual outcomes in `docs/VALIDATION.md`.
- Keep benchmarks descriptive; never make timing a pass/fail threshold.
- Keep sample assets original and generated from `tools/generate_sample.py`.
- Do not add GPU, GUI, or unrelated product features to this milestone.
