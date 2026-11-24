#!/usr/bin/env python3
"""Set the libctru boot heaps for an Old 3DS.

An Old 3DS application gets 64MB total, and only a few megabytes of that
can be physically contiguous. The devkitARM crt0 asks for 24MB plus a
32MB linear heap. Reserving the linear half crashes the HOME Menu.

Cinder's framebuffers are 24-bit BGR8 in this heap (about 1.6MB
double buffered). 4MB covers the title list. 4MB linear holds the
screens with room to spare and is still a small contiguous request.
"""
import struct
import sys
from pathlib import Path

HEAP = 4 * 1024 * 1024
LINEAR = 4 * 1024 * 1024

def main() -> None:
    path = Path(sys.argv[1])
    blob = bytearray(path.read_bytes())
    magic = blob.find(b"_prm")
    if magic < 0 or blob[magic - 4 : magic] != b"\x06\x00\x00\xea":
        raise SystemExit("crt0 header not found")
    struct.pack_into("<II", blob, magic + 12, HEAP, LINEAR)
    path.write_bytes(blob)
    print(f"heap {HEAP:#x} linear {LINEAR:#x}")

if __name__ == "__main__":
    main()
