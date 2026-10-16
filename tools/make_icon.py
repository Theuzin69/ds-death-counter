"""Generates res/app.ico (a coiled sword in a bonfire) without external libraries.

Usage: python tools/make_icon.py res/app.ico
"""
import math
import struct
import sys
import zlib

SIZES = [16, 24, 32, 48, 64, 128, 256]
SS = 4  # supersampling per axis


def lerp(a, b, t):
    return a + (b - a) * t


def mix(c1, c2, t):
    return tuple(lerp(a, b, t) for a, b in zip(c1, c2))


def flame_width(y):
    # flame spans y in [0.40, 0.86]; widest near the bottom
    if y < 0.40 or y > 0.86:
        return 0.0
    t = (y - 0.40) / 0.46
    return 0.25 * math.sin(math.pi * min(1.0, t * 0.95)) ** 0.7 * (0.35 + 0.65 * t)


def sample(x, y):
    """Returns premultiplied-free RGBA (0..1) for a point in unit space."""
    col = (0, 0, 0, 0)

    def over(dst, src):
        sa = src[3]
        da = dst[3]
        oa = sa + da * (1 - sa)
        if oa <= 0:
            return (0, 0, 0, 0)
        rgb = tuple((s * sa + d * da * (1 - sa)) / oa for s, d in zip(src[:3], dst[:3]))
        return rgb + (oa,)

    # background disc with a warm glow at the bottom
    dx, dy = x - 0.5, y - 0.5
    r = math.hypot(dx, dy)
    if r <= 0.48:
        glow = max(0.0, 1.0 - math.hypot(x - 0.5, y - 0.78) / 0.55)
        base = mix((0.09, 0.075, 0.065), (0.42, 0.17, 0.05), glow ** 1.6)
        col = over(col, base + (1.0,))
    elif r <= 0.5:
        col = over(col, (0.55, 0.36, 0.18, 1.0))

    # ash mound
    if ((x - 0.5) / 0.30) ** 2 + ((y - 0.86) / 0.07) ** 2 <= 1.0:
        col = over(col, (0.23, 0.20, 0.18, 1.0))

    # sword (planted blade-down)
    steel = (0.86, 0.84, 0.80, 1.0)
    dark = (0.45, 0.40, 0.35, 1.0)
    if math.hypot(x - 0.5, y - 0.15) <= 0.038:
        col = over(col, dark)
    if 0.483 <= x <= 0.517 and 0.17 <= y <= 0.29:
        col = over(col, dark)
    if 0.35 <= x <= 0.65 and 0.285 <= y <= 0.325:
        col = over(col, steel)
    if 0.463 <= x <= 0.537 and 0.32 <= y <= 0.82:
        col = over(col, steel)

    # flames (partially over the blade)
    w = flame_width(y)
    if w > 0:
        wobble = 0.035 * math.sin(y * 34.0)
        d = abs(x - 0.5 - wobble * (y - 0.4))
        if d <= w:
            t = d / w
            inner = mix((1.0, 0.86, 0.45), (0.96, 0.38, 0.08), t)
            alpha = 0.92 * (1.0 - t ** 3)
            if 0.463 <= x <= 0.537:
                alpha *= 0.55  # let the blade show through
            col = over(col, inner + (alpha,))
    return col


def render(size):
    rows = []
    for py in range(size):
        row = bytearray()
        for px in range(size):
            acc = [0.0, 0.0, 0.0, 0.0]
            for sy in range(SS):
                for sx in range(SS):
                    c = sample((px + (sx + 0.5) / SS) / size, (py + (sy + 0.5) / SS) / size)
                    acc[0] += c[0] * c[3]
                    acc[1] += c[1] * c[3]
                    acc[2] += c[2] * c[3]
                    acc[3] += c[3]
            n = SS * SS
            a = acc[3] / n
            if a > 0:
                rgb = [min(255, int(round(acc[i] / acc[3] * 255))) for i in range(3)]
            else:
                rgb = [0, 0, 0]
            row += bytes(rgb + [int(round(a * 255))])
        rows.append(bytes(row))
    return rows


def png(size, rows):
    raw = b"".join(b"\x00" + r for r in rows)

    def chunk(kind, data):
        c = struct.pack(">I", len(data)) + kind + data
        return c + struct.pack(">I", zlib.crc32(kind + data) & 0xFFFFFFFF)

    ihdr = struct.pack(">IIBBBBB", size, size, 8, 6, 0, 0, 0)
    return b"\x89PNG\r\n\x1a\n" + chunk(b"IHDR", ihdr) + chunk(b"IDAT", zlib.compress(raw, 9)) + chunk(b"IEND", b"")


def main(out):
    images = [png(s, render(s)) for s in SIZES]
    header = struct.pack("<HHH", 0, 1, len(images))
    offset = 6 + 16 * len(images)
    entries = b""
    for s, data in zip(SIZES, images):
        dim = 0 if s >= 256 else s
        entries += struct.pack("<BBBBHHII", dim, dim, 0, 0, 1, 32, len(data), offset)
        offset += len(data)
    with open(out, "wb") as f:
        f.write(header + entries + b"".join(images))


if __name__ == "__main__":
    main(sys.argv[1] if len(sys.argv) > 1 else "app.ico")
