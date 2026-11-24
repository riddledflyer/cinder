from pathlib import Path

from PIL import Image

HERE = Path(__file__).resolve().parent
ROOT = HERE.parent
VERSION_H = ROOT / "source" / "version.h"

GLYPH = {
    "0": [" ### ", "#   #", "#   #", "#   #", "#   #", "#   #", " ### "],
    "1": ["  #  ", " ##  ", "  #  ", "  #  ", "  #  ", "  #  ", " ### "],
    "2": [" ### ", "#   #", "    #", "   # ", "  #  ", " #   ", "#####"],
    "3": [" ### ", "#   #", "    #", " ### ", "    #", "#   #", " ### "],
    "4": ["#   #", "#   #", "#   #", "#####", "    #", "    #", "    #"],
    "5": ["#####", "#    ", "#    ", "#### ", "    #", "    #", "#### "],
    "6": [" ### ", "#    ", "#    ", "#### ", "#   #", "#   #", " ### "],
    "7": ["#####", "    #", "   # ", "  #  ", "  #  ", "  #  ", "  #  "],
    "8": [" ### ", "#   #", "#   #", " ### ", "#   #", "#   #", " ### "],
    "9": [" ### ", "#   #", "#   #", " ####", "    #", "    #", " ### "],
    ".": [" ", " ", " ", " ", " ", "#", "#"],
}

INK = (231, 233, 238)
CHIP = (12, 13, 15)


def version():
    for line in VERSION_H.read_text().splitlines():
        if line.startswith("#define CINDER_VERSION"):
            return line.split('"')[1]
    raise SystemExit("CINDER_VERSION missing")


def text_size(text, scale):
    gap = scale
    width = 0
    for i, ch in enumerate(text):
        glyph = GLYPH[ch]
        width += len(glyph[0]) * scale
        if i + 1 < len(text):
            width += gap
    return width, 7 * scale


def stamp(src, dest, scale, margin):
    im = Image.open(src).convert("RGB")
    text = version()
    for ch in text:
        if ch not in GLYPH:
            raise SystemExit(f"no glyph for {ch!r}")
    tw, th = text_size(text, scale)
    pad = scale
    bw, bh = tw + pad * 2, th + pad * 2
    x0 = im.width - margin - bw
    y0 = margin
    if x0 < 0 or y0 + bh > im.height:
        raise SystemExit(f"version chip does not fit {src.name}")
    px = im.load()
    for y in range(y0, y0 + bh):
        for x in range(x0, x0 + bw):
            px[x, y] = CHIP
    gap = scale
    cx = x0 + pad
    cy = y0 + pad
    for ch in text:
        glyph = GLYPH[ch]
        gw = len(glyph[0]) * scale
        for gy, row in enumerate(glyph):
            for gx, bit in enumerate(row):
                if bit != "#":
                    continue
                for dy in range(scale):
                    for dx in range(scale):
                        px[cx + gx * scale + dx, cy + gy * scale + dy] = INK
        cx += gw + gap
    dest.parent.mkdir(parents=True, exist_ok=True)
    im.save(dest)


def main():
    stamp(HERE / "icon-base.png", ROOT / "icon.png", 1, 1)
    stamp(HERE / "banner-base.png", HERE / "banner.png", 2, 4)
    web_base = HERE / "icon-web-base.png"
    web_out = ROOT.parents[1] / "public" / "cinder-icon.png"
    if web_base.exists() and web_out.parent.exists():
        stamp(web_base, web_out, 4, 8)
    print("stamped", version())


if __name__ == "__main__":
    main()
