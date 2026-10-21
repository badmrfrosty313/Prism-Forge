# Release-candidate packaging

GitHub Actions builds the v1.0.0 release candidate on Ubuntu 22.04 x86-64 and Windows x86-64. The package job runs only after the Debug/Release Linux/Windows tests and Linux AddressSanitizer/UndefinedBehaviorSanitizer tests pass.

Each package contains the CLI, quick-start and release documentation, project and third-party license texts, and a small PPM/PNG sample. The Windows ZIP also contains `PrismForge-GUI.exe` at the package root and in `bin/`, plus `START_HERE.txt`; Windows users can double-click the root GUI executable. The package job runs CLI conversion checks and an automated GUI initialization/processing smoke test, then extracts the archive and exercises both executables. The GUI smoke test calls the production processing library and verifies the saved output; it does not automate mouse or keyboard interaction.

Download artifacts named `prismforge-v1.0.0-rc-ubuntu-22.04` (tar.gz) and `prismforge-v1.0.0-rc-windows-latest` (ZIP) from a successful Actions run. These are workflow artifacts, not a published GitHub Release. Linux packages use the Ubuntu 22.04 runtime baseline and require glibc 2.35 or newer. Windows builds use the static MSVC runtime.

No Python or development tool is needed to run the executable. Linux remains CLI-only. JPEG/PNG metadata is not preserved: EXIF orientation is not applied, ICC profiles are ignored, and pixels are treated as 8-bit samples without color management. PNG alpha is preserved through filters and PNG output; JPEG/PPM writing rejects non-opaque images to prevent silent alpha loss. The GUI asks before flattening transparency for JPEG or PPM output.
