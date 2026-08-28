#!/usr/bin/env python3
"""Generate a TH08 score.dat with all progression flags enabled.

The format follows ScoreDat::WriteScore/OpenScore in this repository.  The
LZSS stream intentionally uses literal tokens only; this is slower to read
than a tightly compressed stream but is fully compatible with the decoder and
keeps this generator independent of the game binary.
"""

from __future__ import annotations

import argparse
import struct
from pathlib import Path


MAGIC = {
    "TH8K": 0x4B384854,
    "CATK": 0x4B544143,
    "CLRD": 0x44524C43,
    "FLSP": 0x50534C46,
    "HSCR": 0x52435348,
    "LSNM": 0x4D4E534C,
    "PLST": 0x54534C50,
    "PSCR": 0x52435350,
}

HEADER_SIZE = 0x1C
SHOT_ALL = 12
SPELL_COUNT = 222
LAST_WORD_START = 205
LAST_WORD_COUNT = 17
STAGES = 9
DIFFICULTIES = 5
EXTRA_FLAG = 1 << 14
SPELL_PRACTICE_FLAG = 1 << 15


def chapter_header(magic: int, size: int, version: int) -> bytes:
    return struct.pack("<IHHBBxx", magic, size, size, version, 0)


def make_clrd() -> bytes:
    # Clrd is 0x24 bytes: 0x0c header, two 5-entry u16 arrays, two bytes,
    # and two bytes of alignment padding.
    out = bytearray()
    for shot in range(SHOT_ALL + 1):
        part = bytearray(chapter_header(MAGIC["CLRD"], 0x24, 4))
        part += struct.pack("<5H", *([0xFFFF] * DIFFICULTIES))
        part += struct.pack("<5H", *([0xFFFF] * DIFFICULTIES))
        part += bytes((1, shot, 0, 0))
        assert len(part) == 0x24
        out += part
    return bytes(out)


def make_flsp() -> bytes:
    # The array stores the actual spell number, not merely a boolean.
    part = bytearray(chapter_header(MAGIC["FLSP"], 0x20, 1))
    part += bytes(range(LAST_WORD_START, LAST_WORD_START + LAST_WORD_COUNT))
    part += bytes(0x20 - len(part))
    assert len(part) == 0x20
    return bytes(part)


def make_catk() -> bytes:
    out = bytearray()
    for spell in range(SPELL_COUNT):
        part = bytearray(chapter_header(MAGIC["CATK"], 0x22C, 3))
        part += struct.pack("<HBB", spell, 0, 0)
        # Generic names keep every entry visibly available.  The game will
        # replace these with the original Japanese text after each encounter.
        name = f"UNLOCKED SPELL {spell + 1:03d}".encode("ascii")
        owner = b"TH08 IOS"
        part += name[:47] + bytes(48 - min(len(name), 47))
        part += owner + bytes(48 - len(owner))
        part += bytes(128)
        # Two CatkHistory values, each 13 maxBonus/attempts/captures fields.
        part += struct.pack("<13i", *([999999] * 13))
        part += struct.pack("<13I", *([9999] * 13))
        part += struct.pack("<13I", *([9999] * 13))
        part += struct.pack("<13i", *([999999] * 13))
        part += struct.pack("<13I", *([9999] * 13))
        part += struct.pack("<13I", *([9999] * 13))
        part += struct.pack("<i", 0)
        assert len(part) == 0x22C
        out += part
    return bytes(out)


def make_pscr() -> bytes:
    out = bytearray()
    for shot in range(SHOT_ALL):
        part = bytearray(chapter_header(MAGIC["PSCR"], 0x178, 2))
        part += struct.pack("<45i", *([999999999] * 45))
        part += struct.pack("<45i", *([999999999] * 45))
        part += bytes((shot, 1, 0, 0))
        assert len(part) == 0x178
        out += part
    return bytes(out)


def make_plst() -> bytes:
    part = bytearray(chapter_header(MAGIC["PLST"], 0x228, 2))
    part += struct.pack("<8I", *([999999] * 8))
    # Six difficulty counters plus the total counter.
    for _ in range(DIFFICULTIES + 2):
        part += struct.pack("<I", 999999)
        part += struct.pack("<12i", *([999999] * 12))
        part += bytes(4)
        part += struct.pack("<3i", 999999, 0, 999999)
    part += bytes([1] * 32)
    assert len(part) == 0x228
    return bytes(part)


def make_lsnm() -> bytes:
    part = bytearray(chapter_header(MAGIC["LSNM"], 0x18, 1))
    part += b"PLAYER\0\0\0"
    part += bytes(0x18 - len(part))
    assert len(part) == 0x18
    return bytes(part)


def lzss_literals(data: bytes) -> bytes:
    # Pack one flag bit followed by eight literal bits for each input byte.
    # A zero flag and a zero 13-bit offset terminate the stream.
    bits: list[int] = []
    for value in data:
        bits.append(1)
        bits.extend((value >> bit) & 1 for bit in range(7, -1, -1))
    bits.extend([0] * 14)
    out = bytearray((len(bits) + 7) // 8)
    for index, bit in enumerate(bits):
        if bit:
            out[index // 8] |= 0x80 >> (index % 8)
    return bytes(out)


def rotate8(value: int) -> int:
    return ((value & 0xE0) >> 5) | ((value & 0x1F) << 3)


def crypt(data: bytes, encrypt: bool) -> bytes:
    xor_value = 0x59
    increment = 0x79
    chunk_size = 0x100
    max_bytes = 0xC00
    size = len(data)
    unencrypted = (size % chunk_size if size % chunk_size < chunk_size // 4 else 0) + (size & 1)
    work = size - unencrypted
    out = bytearray(size)
    in_pos = out_pos = 0
    while work > 0 and max_bytes > 0:
        chunk = min(chunk_size, work)
        if encrypt:
            pos = in_pos + chunk - 1
            for _ in range((chunk + 1) // 2):
                out[out_pos] = data[pos] ^ xor_value
                out_pos += 1
                pos -= 2
                xor_value = (xor_value + increment) & 0xFF
            pos = in_pos + chunk - 2
            for _ in range(chunk // 2):
                out[out_pos] = data[pos] ^ xor_value
                out_pos += 1
                pos -= 2
                xor_value = (xor_value + increment) & 0xFF
        else:
            backup = out_pos
            pos = out_pos + chunk - 1
            for _ in range((chunk + 1) // 2):
                out[pos] = data[in_pos] ^ xor_value
                in_pos += 1
                pos -= 2
                xor_value = (xor_value + increment) & 0xFF
            pos = backup + chunk - 2
            for _ in range(chunk // 2):
                out[pos] = data[in_pos] ^ xor_value
                in_pos += 1
                pos -= 2
                xor_value = (xor_value + increment) & 0xFF
            out_pos += chunk
        if encrypt:
            in_pos += chunk
        work -= chunk
        max_bytes -= chunk
    remainder = work + unencrypted
    if remainder:
        out[out_pos : out_pos + remainder] = data[in_pos : in_pos + remainder]
    return bytes(out)


def build_score() -> bytes:
    payload = bytearray()
    payload += chapter_header(MAGIC["TH8K"], 0x0C, 1)
    payload += make_clrd()
    payload += make_flsp()
    payload += make_catk()
    payload += make_pscr()
    payload += make_plst()
    payload += make_lsnm()

    compressed = lzss_literals(payload)
    header = bytearray(HEADER_SIZE)
    header[0] = 0
    header[1] = 0x37
    struct.pack_into("<H", header, 4, 1)
    struct.pack_into("<I", header, 8, HEADER_SIZE)
    struct.pack_into("<I", header, 16, HEADER_SIZE + len(payload))
    struct.pack_into("<I", header, 20, len(payload))
    struct.pack_into("<I", header, 24, len(compressed))
    plain = header + compressed

    # ResultScreen::WriteScore's rolling byte transform.
    checksum = sum(plain[4:]) & 0xFFFF
    struct.pack_into("<H", plain, 2, checksum)
    value = plain[1]
    for index in range(1, len(plain) - 1):
        old = plain[index + 1]
        value = rotate8(value)
        plain[index + 1] ^= value
        value = (value + old) & 0xFF

    return crypt(bytes(plain), True)


def main() -> None:
    parser = argparse.ArgumentParser()
    parser.add_argument("output", nargs="?", type=Path, default=Path("score.dat"))
    args = parser.parse_args()
    result = build_score()
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_bytes(result)
    print(f"wrote {args.output} ({len(result)} bytes)")


if __name__ == "__main__":
    main()
