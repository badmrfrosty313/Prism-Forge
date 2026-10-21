#!/usr/bin/env python3
"""Convert P6 PPM result files to PNG previews using only Python's standard library."""

import argparse
import binascii
from pathlib import Path
import struct
import zlib


def read_p6(path: Path) -> tuple[int, int, bytes]:
    data = path.read_bytes()
    position = 0

    def next_token() -> bytes:
        nonlocal position
        while position < len(data):
            if data[position] in b" \t\r\n\v\f":
                position += 1
            elif data[position] == ord("#"):
                newline = data.find(b"\n", position)
                position = len(data) if newline < 0 else newline + 1
            else:
                break
        start = position
        while position < len(data) and data[position] not in b" \t\r\n\v\f#":
            position += 1
        if start == position:
            raise ValueError(f"missing PPM token in {path}")
        return data[start:position]

    if next_token() != b"P6":
        raise ValueError(f"{path} is not binary P6 PPM")
    width, height, maximum = (int(next_token()) for _ in range(3))
    if width <= 0 or height <= 0 or maximum != 255 or width * height > 16 * 1024 * 1024:
        raise ValueError(f"unsupported or oversized PPM dimensions in {path}")
    if position >= len(data) or data[position] not in b" \t\r\n\v\f":
        raise ValueError(f"missing PPM raster separator in {path}")
    separator = data[position]
    position += 1
    if separator == 13 and position < len(data) and data[position] == 10:
        position += 1
    pixels = data[position:]
    if len(pixels) != width * height * 3:
        raise ValueError(f"unexpected PPM raster length in {path}")
    return width, height, pixels


def chunk(kind: bytes, payload: bytes) -> bytes:
    body = kind + payload
    return struct.pack(">I", len(payload)) + body + struct.pack(">I", binascii.crc32(body) & 0xFFFFFFFF)


def write_png(source: Path, destination: Path) -> None:
    width, height, pixels = read_p6(source)
    stride = width * 3
    scanlines = b"".join(b"\0" + pixels[row * stride:(row + 1) * stride] for row in range(height))
    ihdr = struct.pack(">IIBBBBB", width, height, 8, 2, 0, 0, 0)
    destination.parent.mkdir(parents=True, exist_ok=True)
    destination.write_bytes(b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(scanlines, 9)) + chunk(b"IEND", b""))


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("--output-dir", required=True, type=Path)
    parser.add_argument("--image", action="append", required=True, metavar="NAME=FILE")
    args = parser.parse_args()
    for item in args.image:
        name, separator, filename = item.partition("=")
        if not separator or not name or Path(name).name != name:
            parser.error(f"invalid --image value: {item}")
        destination = args.output_dir / f"{name}.png"
        write_png(Path(filename), destination)
        print(f"created {destination} from {filename}")


if __name__ == "__main__":
    main()
