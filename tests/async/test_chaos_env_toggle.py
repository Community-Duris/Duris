#!/usr/bin/env python3
"""Regression coverage for the runtime CHAOS_MUD environment toggle."""

from _paths import SRC
import os
import subprocess
import tempfile
from pathlib import Path

from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]
makefile = (SRC / "Makefile").read_text()
example = (ROOT / ".env.example").read_text()
config = (SRC / "chaos_config.c").read_text()
consumers = "\n".join(
    (SRC / name).read_text()
    for name in ("guild.c", "memorize.c", "mobconv.c", "nanny.c")
)

assert "-DCHAOS_MUD" not in makefile
assert "CHAOS_MUD=FALSE" in example
assert "CHAOS_MUD=1" not in example
assert "CHAOS_MUD=0" not in example
assert contains(config, 'getenv("CHAOS_MUD")')
assert contains(config, 'strcmp(value, "TRUE") == 0')
assert "defined(CHAOS_MUD)" not in consumers
assert "chaos_mud_enabled()" in consumers

HARNESS = r"""
#include "combat/chaos_config.h"

#include <stdio.h>

int main(void)
{
	printf("%s\n", chaos_mud_enabled() ? "TRUE" : "FALSE");
	return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="duris-chaos-toggle-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            f"-I{SRC}",
            str(harness),
            str(SRC / "chaos_config.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )

    def evaluate(value):
        env = os.environ.copy()
        if value is None:
            env.pop("CHAOS_MUD", None)
        else:
            env["CHAOS_MUD"] = value
        return subprocess.check_output([str(binary)], env=env, text=True).strip()

    assert evaluate("TRUE") == "TRUE"
    assert evaluate("FALSE") == "FALSE"
    assert evaluate(None) == "FALSE"
    assert evaluate("1") == "FALSE"
    assert evaluate("0") == "FALSE"
    assert evaluate("true") == "FALSE"

print("chaos environment toggle contracts passed")
