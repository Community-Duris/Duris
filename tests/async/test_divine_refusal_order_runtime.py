#!/usr/bin/env python3
"""Compile and execute the real divine-refusal helpers and do_order branches."""

from pathlib import Path
import subprocess
import sys
import tempfile

from _paths import ROOT, extract_function, source


FUNCTIONS = (
    ("static divine_refusal_config current_divine_refusal_config(",),
    ("static bool divine_refusal_eligible(",),
    ("static bool divine_refusal_order_candidate(",),
    ("static bool divine_refusal_command_blocked(",),
    ("static void show_new_divine_refusal(",),
    ("static bool divine_refusal_blocks_order(",),
    ("void do_order(",),
)


def main() -> int:
    template_path = Path(__file__).with_name(
        "divine_refusal_order_runtime_template.cpp"
    )
    template = template_path.read_text(encoding="utf-8")
    marker = "/*__DIVINE_REFUSAL_PRODUCTION_FUNCTIONS__*/"
    assert template.count(marker) == 1
    production = "\n\n".join(
        extract_function("actoff.c", signatures[0]) for signatures in FUNCTIONS
    )
    harness = template.replace(marker, production)

    with tempfile.TemporaryDirectory() as directory:
        generated = Path(directory) / "divine_refusal_order_runtime.cpp"
        binary = Path(directory) / "divine_refusal_order_runtime"
        generated.write_text(harness, encoding="utf-8")
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                "-g",
                "-fsanitize=address,undefined",
                "-I",
                str(ROOT / "src"),
                str(generated),
                str(source("divine_refusal_policy.c")),
                str(source("divine_refusal_content.c")),
                "-lcjson",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True)
    print("Divine refusal production order runtime checks passed.")
    return 0


if __name__ == "__main__":
    sys.exit(main())
