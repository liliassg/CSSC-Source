#!/usr/bin/env python3
"""
Trim the dead space off the IDE screenshots.

The raw captures are full 2560x1440 frames with a lot of empty editor below the
code. Shrinking them to what actually matters means the page ships a quarter of
the pixels and the reader sees the point instead of a field of black.

Nothing is scaled or recolored, this only cuts rows and columns whose pixels
are all one of the editor's background tones. Originals are left alone; the
results go to web/img/.
"""

from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
SHOTS = HERE / "screenshots"
OUT = HERE.parent / "web" / "img"

# Every background tone in the IDE stays below 40 on its brightest channel
# (#0E1016, #141019, #1A1424), while the dimmest thing worth keeping, the
# purple selection band #572663, reaches 99. 45 splits them cleanly.
INK = 45

# A row or column only counts as content if it carries at least this many lit
# pixels. Without it the 2px pane border, which runs the full width and height,
# would make every frame look full of content and nothing would ever be cut.
MIN_LIT = 8

# name -> (source, how to cut it)
#
# An int means autocrop with that much padding, which is right for the frames
# meant to show the whole IDE. A 4-tuple is an explicit box, used where the
# point is one detail: the page can only give an image about 1600px, and a full
# 2500px capture squeezed into that turns the code into grey mush. Cutting to
# the part that carries the point keeps it near 1:1 and readable.
JOBS = {
    "syntax": ("SyntaxStyle.png", 8),
    "analyzer_full": ("lsp_sema_ownership.png", 8),
    "autoclean_full": ("ide_autoclean.png", 8),
    "debug_trace_full": ("ide_debug_trace.png", 8),

    "workspace": ("IDE.png", (0, 0, 2556, 560)),
    "analyzer": ("lsp_sema_ownership.png", (335, 75, 2020, 488)),
    "ownership_freed": ("ide_ownership_freed.png", (0, 0, 1010, 990)),
    "ownership_leak": ("ide_ownership_leak.png", (0, 0, 1010, 960)),
    "autoclean": ("ide_autoclean.png", (300, 950, 1500, 1085)),
    "autoclean_report": ("ide_autoclean.png", (10, 1195, 900, 1345)),
    "debug_ip": ("ide_debug_trace.png", (330, 5, 1420, 395)),
    "debug_track": ("ide_debug_trace.png", (0, 1095, 900, 1280)),
    "comments": ("SyntaxStyle.png", (280, 505, 1250, 1015)),
    "hover": ("lsp_hover_slot.png", 0),
    "completions": ("lsp_completitions.png", 0),
    "arguments": ("lsp_arguments.png", 0),
}


def content_box(im, pad):
    """Bounding box of the rows and columns that actually carry ink."""
    width, height = im.size
    lit = im.point(lambda v: 255 if v >= INK else 0).convert("L")
    px = lit.load()

    row_counts = [0] * height
    col_counts = [0] * width
    for y in range(height):
        row = row_counts
        for x in range(width):
            if px[x, y]:
                row[y] += 1
                col_counts[x] += 1

    rows = [y for y in range(height) if row_counts[y] >= MIN_LIT]
    cols = [x for x in range(width) if col_counts[x] >= MIN_LIT]

    if not rows or not cols:
        return (0, 0, width, height)

    return (
        max(cols[0] - pad, 0),
        max(rows[0] - pad, 0),
        min(cols[-1] + 1 + pad, width),
        min(rows[-1] + 1 + pad, height),
    )


def main():
    OUT.mkdir(parents=True, exist_ok=True)

    for name, (source, how) in JOBS.items():
        path = SHOTS / source
        if not path.exists():
            print(f"skip   {source} (not found)")
            continue

        im = Image.open(path).convert("RGB")
        if isinstance(how, tuple):
            box = (
                max(how[0], 0), max(how[1], 0),
                min(how[2], im.size[0]), min(how[3], im.size[1]),
            )
        else:
            box = content_box(im, how)
        cropped = im.crop(box)

        target = OUT / f"{name}.png"
        cropped.save(target, optimize=True)

        before = path.stat().st_size
        after = target.stat().st_size
        print(
            f"{name:18} {im.size[0]}x{im.size[1]} -> "
            f"{cropped.size[0]}x{cropped.size[1]}   "
            f"{before / 1024:6.1f} KB -> {after / 1024:6.1f} KB"
        )


if __name__ == "__main__":
    main()
