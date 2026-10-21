# Architecture

The project uses a small CMake library (`prismforge_core`) so image operations can be called independently of the command line. `Image` owns checked, row-major RGB8 storage. Filters return new images and do not mutate their inputs. The CLI parses options, loads a PPM, invokes one operation, and writes a PPM. A separate benchmark calls the same library functions on a deterministic synthetic image.

Gaussian blur builds a normalized one-dimensional Gaussian kernel and performs horizontal then vertical convolution. Sampling beyond the image uses the nearest edge pixel. Sobel computes horizontal and vertical 3x3 gradients over Rec. 601 luma, takes their Euclidean magnitude, and clamps it to grayscale byte range; its border uses the same replicated-pixel rule. Color adjustment applies saturation around luma, contrast around mid-gray 127.5, then an additive brightness offset, with final rounding and clamping.

Image I/O is deliberately limited to P3/P6 PPM at max value 255. This avoids external libraries in the foundation; PNG/JPEG and OpenCV remain possible follow-up work. CMake creates the CLI, CTest executable, and optional benchmark. The CI matrix covers Debug and Release on Linux and Windows.
