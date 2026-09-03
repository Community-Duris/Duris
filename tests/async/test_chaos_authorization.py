#!/usr/bin/env python3
"""Regression contracts for the trusted-only Chaos command gate."""

from __future__ import annotations

import subprocess
from pathlib import Path

from _paths import SRC


ROOT = Path(__file__).resolve().parents[2]
CHAOS = SRC / "combat/chaos.c"
EXPECTED_GATE = "if (!IS_TRUSTED(ch) || !chaos_mud_enabled())"


def extract_function(text: str, signature: str) -> str:
    start = text.index(signature)
    brace = text.index("{", start)
    depth = 0
    for position in range(brace, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated function: {signature}")


def preprocessed_chaos(*defines: str) -> str:
    command = [
        "g++",
        "-std=c++20",
        "-D__NO_TESTS__",
        *(f"-D{name}" for name in defines),
        "-I.",
        "-I../tests/async",
        "-E",
        "-P",
        str(CHAOS),
    ]
    return subprocess.run(
        command,
        cwd=SRC,
        check=True,
        text=True,
        capture_output=True,
    ).stdout


def assert_gate(source: str, variant: str) -> None:
    # Preprocessed output also contains the header prototype, so select the
    # final occurrence: the real function definition.
    body = extract_function(source[source.rindex("void do_chaos(") :], "void do_chaos(")
    if variant == "source":
        gate_index = body.index(EXPECTED_GATE)
    else:
        chaos_gate_lines = [
            line for line in body.splitlines() if "chaos_mud_enabled()" in line
        ]
        assert chaos_gate_lines, f"Chaos-mode gate missing in {variant} build"
        assert "||" in chaos_gate_lines[0], (
            f"trusted check is not unconditional in {variant} build"
        )
        gate_index = body.index(chaos_gate_lines[0])
    assert gate_index < body.index("one_argument"), (
        f"authorization gate occurs after argument parsing in {variant} build"
    )
    assert "chaos_test_commands_enabled()" in body
    assert body.index("chaos_test_commands_enabled()") > gate_index
    assert "#ifndef TEST_MUD" not in body


source = CHAOS.read_text(encoding="utf-8")
assert_gate(source, "source")
assert_gate(preprocessed_chaos(), "non-TEST_MUD")
assert_gate(preprocessed_chaos("TEST_MUD"), "TEST_MUD")

# The command registration is a position gate, not the authorization boundary.
interp = (SRC / "cmd/interp.c").read_text(encoding="utf-8")
assert "CMD_Y(CMD_CHAOS, STAT_DEAD + POS_PRONE, do_chaos, 0, FALSE);" in interp

print("Chaos trusted-only authorization contracts passed")
