#!/usr/bin/env python3
"""Compile and execute the production extractlink command with controlled saves."""

from pathlib import Path
import subprocess
import sys
import tempfile

from _paths import ROOT, extract_function


FUNCTIONS = (
    ("actwiz.c", "static bool extractlink_attempt(P_char ch"),
    ("actwiz.c", "void do_extractlink(P_char ch"),
)


def main() -> int:
    template_path = Path(__file__).with_name(
        "extractlink_feedback_runtime_template.cpp"
    )
    template = template_path.read_text(encoding="utf-8")
    marker = "/*__EXTRACTLINK_PRODUCTION_FUNCTIONS__*/"
    assert template.count(marker) == 1
    production = "\n\n".join(
        extract_function(filename, signature) for filename, signature in FUNCTIONS
    )
    harness = template.replace(marker, production)

    with tempfile.TemporaryDirectory(prefix="duris-extractlink-") as directory:
        generated = Path(directory) / "extractlink_feedback_runtime.cpp"
        binary = Path(directory) / "extractlink_feedback_runtime"
        generated.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-g",
                "-fsanitize=address,undefined",
                "-fno-omit-frame-pointer",
                str(generated),
                "-o",
                str(binary),
            ],
            check=True,
            cwd=ROOT,
        )
        subprocess.run([str(binary)], check=True, cwd=ROOT)

    print("extractlink feedback runtime checks passed")
    return 0


if __name__ == "__main__":
    sys.exit(main())
