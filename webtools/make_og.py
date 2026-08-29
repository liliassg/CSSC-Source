#!/usr/bin/env python3
"""
Build the social preview card, web/og.png.

This is the image Reddit, Mastodon, Discord, Slack and the rest show when
somebody posts the link. 1200x630 is the size they all crop to. It is drawn
with the same font and the same star as the site, so a shared link looks like
the page it leads to.

Everything lands on whole pixels: the font is used at a multiple of 8 and the
star at a multiple of 10, its own resolution.
"""

from pathlib import Path

from PIL import Image, ImageDraw, ImageFont

HERE = Path(__file__).resolve().parent
FONT = HERE / "CSSCPixel.ttf"
STAR = HERE.parent / "web" / "star.png"
OUT = HERE.parent / "web" / "og.png"

W, H = 1200, 630
BG = (0, 0, 0)
BRIGHT = (0xE6, 0xE2, 0xF0)
MUTED = (0x8A, 0x84, 0xA0)

WORDMARK = "CSSC"
WORDMARK_SIZE = 96          # 12 source pixels per pixel
EXPANSION = "Control Specified Source Compiling"
EXPANSION_SIZE = 24         # 3 source pixels per pixel
STAR_SIZE = 180             # the star is 10x10, so 18x

GAP_AFTER_WORDMARK = 40
GAP_AFTER_EXPANSION = 48


def main():
    wordmark_font = ImageFont.truetype(str(FONT), WORDMARK_SIZE)
    expansion_font = ImageFont.truetype(str(FONT), EXPANSION_SIZE)

    # The font is monospaced with a full-em advance, so the width of a line is
    # simply its length times the size. No need to ask the rasterizer.
    wordmark_w = len(WORDMARK) * WORDMARK_SIZE
    expansion_w = len(EXPANSION) * EXPANSION_SIZE

    block_h = (
        WORDMARK_SIZE + GAP_AFTER_WORDMARK
        + EXPANSION_SIZE + GAP_AFTER_EXPANSION
        + STAR_SIZE
    )
    top = (H - block_h) // 2

    im = Image.new("RGB", (W, H), BG)
    draw = ImageDraw.Draw(im)

    y = top
    draw.text(((W - wordmark_w) // 2, y), WORDMARK, font=wordmark_font, fill=BRIGHT)

    y += WORDMARK_SIZE + GAP_AFTER_WORDMARK
    draw.text(((W - expansion_w) // 2, y), EXPANSION, font=expansion_font, fill=MUTED)

    y += EXPANSION_SIZE + GAP_AFTER_EXPANSION
    star = Image.open(STAR).convert("RGBA")
    star = star.resize((STAR_SIZE, STAR_SIZE), Image.NEAREST)
    im.paste(star, ((W - STAR_SIZE) // 2, y), star)

    im.save(OUT, optimize=True)
    print(f"{OUT}  {W}x{H}  {OUT.stat().st_size / 1024:.1f} KB")


if __name__ == "__main__":
    main()
