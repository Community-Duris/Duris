#!/usr/bin/env python3
"""Production codec: independent golden bytes, legacy ABI fixtures and rejection.

Every input is synthetic. Executables and disposable files stay under bin/tests.
"""
from pathlib import Path
import struct
import subprocess
import tempfile
import zlib

ROOT = Path(__file__).resolve().parents[2]


def text(value, width):
    return value.encode().ljust(width, b"\0")


def frame(kind, payload):
    return struct.pack("<HHI", kind, 1, len(payload)) + payload


def seal(value):
    value = bytearray(value)
    struct.pack_into("<Q", value, 24, len(value) - 64)
    struct.pack_into("<I", value, 32, zlib.crc32(value[:32] + value[36:]))
    return bytes(value)


def reference():
    """Independent specification encoder; no production source extraction."""
    maximum = (1 << 64) - 1
    desc = (struct.pack("<i", 9) + text("synthetic", 50) + text("127.0.0.1", 50) +
            text("fixture.invalid", 254) + struct.pack("<b5i", -128, 1, 2, -1, -1, -1) +
            text("fixture", 64) + text("terminal", 32) + struct.pack("<2i", 2, -1) +
            text("target", 50) + struct.pack("<I", 1) +
            struct.pack("<10i", *range(1000, 1010)) + struct.pack("<10i", *range(0, -10, -1)) +
            struct.pack("<10i", *range(100, 110)) + struct.pack("<BiQ", 1, 60, maximum))
    # Handoff has eleven u64 values after pid (no native padding).
    handoff = struct.pack("<4Qi11QI", 1, 2, 3, maximum, -1,
                          4, 5, 6, 7, 8, 9, 10, 11, 12, 13, maximum, 0xffffffff)
    telemetry = struct.pack("<i", 9) + text("synthetic", 50) + b"\1" + handoff
    mob = (struct.pack("<12i", 1001, -1, 200, -2, 0x7fffffff, -0x80000000,
                       200, -3, 300, 10, 1, -1) + text("synthetic", 50) +
           struct.pack("<I43iI2i", 1, *([-1] * 42 + [1000]), 1, -99, -1) +
           struct.pack("<4i", 200, 300, 2, -1) + text("synthetic", 50) + struct.pack("<i", -1))
    affect = struct.pack("<hbiIiBBH5Q", -0x8000, -1, -1, 0xffffffff, -0x80000000,
                         255, 255, 65535, maximum, 1 << 63, 3, 4, 5)
    carried = struct.pack("<i", 1000)
    generated = b"GNP1" + bytes(4)
    item = (struct.pack("<3QiiI8i6q", maximum, maximum, 0, 1000, 0, 1,
                        *range(0, -8, -1), -1, -(1 << 63), (1 << 63) - 1, 0, 0, 0) +
            text("x" * 512, 513) + text("a synthetic item", 513) +
            text("Synthetic fixture.", 1025) + text("fixture action", 1025) +
            struct.pack("<5I10i5Q4i4i", 0xffffffff, 0, 0, 0, 0,
                        -2, 3, 0, -32768, 0, 0, 0, 32767, 0, 0,
                        maximum, 0, 0, 0, 0, 127, 0, 0, 0, -128, 0, 0, 0))
    custody = struct.pack("<3QB4QiB", maximum, maximum, 0, 4, 77, 12, maximum, 31, 1000, 1)
    tree = struct.pack("<iI", 200, 1) + item
    obj = struct.pack("<I", len(tree)) + tree + struct.pack("<I", 1) + custody
    door = struct.pack("<3i", 200, 9, -1)
    payloads = [desc, telemetry, mob, affect, carried, generated, obj, door]
    assert list(map(len, payloads)) == [670, 183, 356, 59, 4, 8, 3402, 12]
    header = struct.pack("<4sIIIqQ5I3i", b"DCOF", 18, 64, 0x01020304, -123456789,
                         0, 0, 1, 1, 1, 1, -1, 7, 8)
    return seal(header + b"".join(frame(i, p) for i, p in enumerate(payloads, 1))), payloads


def legacy(version, payloads):
    """Known little-endian LP64 offsets, including arbitrary ignored ABI padding."""
    desc, telemetry, mob, affect, carried, generated, obj, door = payloads
    d = bytearray(b"\xa5" * 680)
    d[:359] = desc[:359]
    d[360:534] = desc[359:533]
    d[536:660] = desc[533:657]
    d[660] = desc[657]
    d[661:664] = bytes(3)
    d[664:668] = desc[658:662]
    d[672:680] = desc[662:670]
    m = bytearray(b"\xa5" * 360)
    m[:98] = mob[:98]
    m[100:354] = mob[98:352]
    m[356:360] = mob[352:356]
    a = bytearray(b"\xa5" * 64)
    a[:3] = affect[:3]
    a[4:20] = affect[3:19]
    a[24:64] = affect[19:59]
    t = bytearray(200)
    t[:55] = telemetry[:55]
    t[64:100] = telemetry[55:91]
    t[104:196] = telemetry[91:183]
    # Portable item has a four-byte gap before timers in the retired ABI.
    item = obj[12:12 + 3324]
    native_item = item[:68] + b"\xa5" * 4 + item[68:]
    c = bytearray(b"\xa5" * 72)
    portable_c = obj[-62:]
    c[:25] = portable_c[:25]
    c[32:69] = portable_c[25:62]
    native_obj = obj[4:12] + native_item + c
    header = struct.pack("<4siq6i3i", b"COPY", version, -123456789, 1, 1, 1, 1, 0, 0, -1, 7, 8)
    trailer = b"TLMY" + struct.pack("<II", 1, 1) + t if version >= 15 else b""
    return (header + d[:660 if version < 17 else 680] + trailer +
            m[:288 if version == 12 else 356 if version < 16 else 360] + a +
            struct.pack("<Q", 0xdeadbeefdeadbeef) + carried + b"\xa5" * 4 +
            (generated if version >= 14 else b"") + struct.pack("<I", len(native_obj)) + native_obj + door)


def main():
    build_root = ROOT / "bin/tests"
    build_root.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="portable-copyover-", dir=build_root) as temporary:
        directory = Path(temporary)
        binary = directory / "codec-test"
        subprocess.run(["g++", "-std=c++20", "-Wall", "-Wextra", "-Werror", "-g",
                        "-fsanitize=address,undefined", "-fno-omit-frame-pointer",
                        "-ffunction-sections", "-fdata-sections", "-Isrc",
                        "tests/async/copyover_codec_harness.cpp", "src/persistence/copyover_codec.c",
                        "src/world/world_recovery_codec.c", "src/world/generated_npc_state.c",
                        "src/player/pet_restore_state.c", "src/item/item_transfer_command.c",
                        "-Wl,--gc-sections", "-Wl,--wrap=fsync", "-o", str(binary)], cwd=ROOT, check=True)

        def run(action, *files):
            subprocess.run([str(binary), action, *map(str, files)], cwd=directory, check=True, timeout=60)

        expected, payloads = reference()
        golden = directory / "rich.dat"
        run("write", golden)
        assert golden.read_bytes() == expected, "portable bytes differ from independent specification"
        run("read", golden)
        run("high-fds")  # Socket numbers are not bounded by the connection count.
        run("children")  # Preserve records beyond the old 64/256 recovery caps.
        run("sweep", golden)  # Every prefix and a high-bit flip at every byte.
        run("failures")
        door = directory / "door.dat"
        run("door", door)
        assert door.read_bytes() == bytes.fromhex(
            (ROOT / "tests/async/fixtures/copyover/v18-door.hex").read_text())
        legacy_paths = []
        for version in range(12, 18):
            fixture = directory / f"legacy-{version}.dat"
            fixture.write_bytes(legacy(version, payloads))
            legacy_paths.append(fixture)
        run("legacy", *legacy_paths)

        bad_files = []

        def reject(name, data):
            path = directory / (name + ".dat")
            path.write_bytes(data)
            bad_files.append(path)

        def mutate(name, offset, fmt, value):
            data = bytearray(expected)
            struct.pack_into(fmt, data, offset, value)
            reject(name, seal(data))  # Valid CRC: tests field/framing validation itself.

        mutate("unknown-version", 4, "<I", 19)
        mutate("old-portable-version", 4, "<I", 17)
        mutate("wrong-order", 12, "<I", 0x04030201)
        mutate("bad-header-size", 8, "<I", 63)
        for offset in (36, 40, 44, 48):
            mutate(f"count-{offset}", offset, "<I", 0xffffffff)
        mutate("negative-listener", 52, "<i", -2)
        mutate("duplicate-listener", 56, "<i", 8)
        mutate("record-length", 68, "<I", 0xffffffff)
        mutate("short-record", 68, "<I", 669)
        mutate("record-version", 66, "<H", 2)
        mutate("record-kind", 64, "<H", 99)
        mutate("bad-fd", 72, "<i", -1)
        mutate("listener-fd-collision", 72, "<i", 7)
        mutate("bad-pet-count", 72 + 533, "<I", 11)
        mutate("bad-death-delay", 72 + 658, "<i", 3)
        mutate("bad-death-pending", 72 + 657, "<B", 2)
        mob_start = 64 + 678 + 191 + 8
        mutate("negative-affect-count", mob_start + 98, "<I", 0xffffffff)
        mutate("too-many-inventory", mob_start + 274, "<I", 32769)
        object_start = 64 + sum(8 + len(p) for p in payloads[:6]) + 8
        mutate("object-tree-length", object_start, "<I", 0xffffffff)
        mutate("object-item-count", object_start + 8, "<I", 513)
        mutate("custody-count", object_start + 4 + 3332, "<I", 0xffffffff)
        mutate("custody-owner-type", object_start + len(payloads[6]) - 62 + 24, "<B", 255)
        mutate("custody-uid", object_start + len(payloads[6]) - 62, "<Q", 1)
        mutate("door-direction", len(expected) - 8, "<i", 10)
        data = bytearray(expected)
        data[76:126] = b"x" * 50
        reject("unterminated-name", seal(data))
        reject("trailing-bytes", seal(expected + b"x"))
        data = bytearray(expected)
        struct.pack_into("<Q", data, 24, 1 << 63)
        struct.pack_into("<I", data, 32, zlib.crc32(data[:32] + data[36:]))
        reject("impossible-payload", data)
        for version in (11, 18, 0xffffffff):
            data = bytearray(legacy(17, payloads))
            struct.pack_into("<I", data, 4, version)
            reject(f"legacy-version-{version}", data)
        data = bytearray(legacy(17, payloads))
        struct.pack_into(">I", data, 4, 17)
        reject("legacy-big-endian", data)
        data = bytearray(legacy(17, payloads))
        struct.pack_into("<i", data, 16, -1)
        reject("legacy-negative-count", data)
        data = bytearray(legacy(17, payloads))
        struct.pack_into("<i", data, 32, 1)
        reject("legacy-unwritten-combat", data)
        for name, offset, value in (("affects", 100, -1), ("inventory", 276, 32769)):
            data = bytearray(legacy(17, payloads))
            struct.pack_into("<i", data, 52 + 680 + 12 + 200 + offset, value)
            reject("legacy-bad-" + name, data)
        for fixture in legacy_paths:
            reject(fixture.stem + "-truncated", fixture.read_bytes()[:-1])
        # ILP32 header layout cannot be mistaken for the known LP64 file.
        reject("legacy-ilp32", struct.pack("<4s9i", b"COPY", 17, 1, 0, 0, 0, 0, 0, 0, -1))
        oversized = directory / "oversized.dat"
        with oversized.open("wb") as stream:
            stream.truncate(128 * 1024 * 1024 + 1)
        bad_files.append(oversized)
        run("reject", *bad_files)
    print("portable copyover: golden bytes, all records/sentinels, v12-v17 ABI fixtures, "
          "truncation/bit-flip sweeps, counts/lengths/version rejection and sync/allocation failures passed")


if __name__ == "__main__":
    main()
