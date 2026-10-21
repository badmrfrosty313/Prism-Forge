#!/usr/bin/env python3
"""Write a small deterministic, original P6 PPM scene without third-party packages."""
from pathlib import Path

width, height = 96, 64
pixels = bytearray()
for y in range(height):
    for x in range(width):
        # Gradient background with a bright disk and a dark rectangular feature.
        r = int(18 + 0.75 * x)
        g = int(28 + 1.2 * y)
        b = int(115 - 0.55 * x + 0.25 * y)
        if (x - 31) ** 2 + (y - 30) ** 2 <= 15 ** 2:
            r, g, b = 245, 184, 42
        if 58 <= x < 82 and 18 <= y < 48:
            r, g, b = 28, 205, 120
        pixels.extend((r, g, b))

destination = Path(__file__).resolve().parents[1].joinpath("examples/synthetic.ppm")
destination.parent.mkdir(parents=True, exist_ok=True)
destination.write_bytes(
    f"P6\n{width} {height}\n255\n".encode("ascii") + pixels
)
