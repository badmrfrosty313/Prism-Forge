# Release-candidate packaging

GitHub Actions builds the v1.0.0 release candidate on Ubuntu 22.04 x86-64 and Windows x86-64. The package job runs only after the Debug/Release Linux/Windows tests and Linux AddressSanitizer/UndefinedBehaviorSanitizer tests pass.

Each package contains the CLI, quick-start and release documentation, project and third-party license texts, and a small PPM/PNG sample. The package job first runs `--version`, converts the included PPM to PNG, reopens that PNG, and writes PPM. It then archives and extracts the package and repeats a conversion using the extracted executable.

Download artifacts named `prismforge-v1.0.0-rc-ubuntu-22.04` (tar.gz) and `prismforge-v1.0.0-rc-windows-latest` (ZIP) from a successful Actions run. These are workflow artifacts, not a published GitHub Release. Linux packages use the Ubuntu 22.04 runtime baseline and require glibc 2.35 or newer. Windows builds use the static MSVC runtime.

No Python or development tool is needed to run the executable. JPEG/PNG metadata is not preserved: EXIF orientation is not applied, ICC profiles are ignored, and pixels are treated as 8-bit samples without color management. PNG alpha is preserved through filters and PNG output; JPEG/PPM writing rejects non-opaque images to prevent silent alpha loss.
