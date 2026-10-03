#!/usr/bin/env python3
"""Execute the real character link read leaf with actual directed link records."""
import argparse
from pathlib import Path
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--compiler", default="g++")
    args = parser.parse_args()
    with tempfile.TemporaryDirectory(prefix="character-link-lookup-") as private:
        for profile in ("sql-header", "flat-header"):
            binary = Path(private) / profile
            flags = (["-D__NO_MYSQL__", "-Isrc/no_mysql"]
                     if profile == "flat-header" else [])
            subprocess.run([args.compiler, "-std=c++20", "-Wall", "-Wextra",
                            "-Wpedantic", "-Werror", "-fsanitize=address,undefined",
                            "-fno-omit-frame-pointer", "-fno-pie", "-no-pie", *flags,
                            "-Isrc", "src/magic/character_link_lookup.c",
                            "tests/async/character_link_lookup_test.cpp", "-o", str(binary)],
                           cwd=ROOT, check=True, timeout=60)
            subprocess.run([str(binary)], cwd=ROOT, check=True, timeout=10)
            print(profile + " actual directed link controls passed")


if __name__ == "__main__":
    main()
