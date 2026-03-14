"""
Generate a Windows RGNDATA file (.rgn) from a skin image.
Uses the top-left pixel as the transparent color key.

Usage: python gen_rgn.py pic.jpg pic.rgn
       python gen_rgn.py pic.jpg pic.rgn --tolerance 30
"""

import struct
import sys
from PIL import Image


def color_distance(c1, c2):
    return sum((a - b) ** 2 for a, b in zip(c1, c2)) ** 0.5


def generate_rgn(image_path, output_path, tolerance=30):
    img = Image.open(image_path).convert("RGB")
    width, height = img.size
    pixels = img.load()

    # Use top-left pixel as transparent color
    transparent = pixels[0, 0]
    print(f"Image size: {width}x{height}")
    print(f"Transparent color (top-left pixel): RGB{transparent}")
    print(f"Tolerance: {tolerance}")

    # Build rectangles from horizontal spans of non-transparent pixels
    rects = []
    for y in range(height):
        x = 0
        while x < width:
            # Skip transparent pixels
            if color_distance(pixels[x, y], transparent) <= tolerance:
                x += 1
                continue
            # Found start of non-transparent span
            x_start = x
            while x < width and color_distance(pixels[x, y], transparent) > tolerance:
                x += 1
            # Add rect for this span (one pixel tall)
            rects.append((x_start, y, x, y + 1))

    print(f"Generated {len(rects)} rectangles")

    # Calculate bounding rectangle
    if rects:
        bound_left = min(r[0] for r in rects)
        bound_top = min(r[1] for r in rects)
        bound_right = max(r[2] for r in rects)
        bound_bottom = max(r[3] for r in rects)
    else:
        bound_left = bound_top = bound_right = bound_bottom = 0

    print(f"Bounding rect: ({bound_left}, {bound_top}) - ({bound_right}, {bound_bottom})")

    # Write RGNDATA structure
    # RGNDATAHEADER: dwSize(4) + iType(4) + nCount(4) + nRgnSize(4) + rcBound(16) = 32 bytes
    # Each RECT: left(4) + top(4) + right(4) + bottom(4) = 16 bytes
    RDH_RECTANGLES = 1
    header_size = 32
    rect_buf_size = len(rects) * 16

    with open(output_path, "wb") as f:
        # RGNDATAHEADER
        f.write(struct.pack("<I", header_size))        # dwSize
        f.write(struct.pack("<I", RDH_RECTANGLES))     # iType
        f.write(struct.pack("<I", len(rects)))          # nCount
        f.write(struct.pack("<I", rect_buf_size))       # nRgnSize
        f.write(struct.pack("<l", bound_left))          # rcBound.left
        f.write(struct.pack("<l", bound_top))           # rcBound.top
        f.write(struct.pack("<l", bound_right))         # rcBound.right
        f.write(struct.pack("<l", bound_bottom))        # rcBound.bottom

        # RECT array
        for left, top, right, bottom in rects:
            f.write(struct.pack("<llll", left, top, right, bottom))

    total_size = header_size + rect_buf_size
    print(f"Written {output_path} ({total_size} bytes)")


if __name__ == "__main__":
    if len(sys.argv) < 3:
        print(f"Usage: {sys.argv[0]} <image> <output.rgn> [--tolerance N]")
        sys.exit(1)

    image_path = sys.argv[1]
    output_path = sys.argv[2]
    tolerance = 30

    if "--tolerance" in sys.argv:
        idx = sys.argv.index("--tolerance")
        tolerance = int(sys.argv[idx + 1])

    generate_rgn(image_path, output_path, tolerance)
