#!/usr/bin/env python3
"""Player-death intake policy and retry-stable sidecar identity."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def function_body(text: str, signature: str) -> str:
    """Return one function from its signature through its closing brace."""
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start:position + 1]
    raise AssertionError(f"unterminated function: {signature}")


def main() -> None:
    movement = (ROOT / "src/item/item_movement_transaction.c").read_text(
        encoding="utf-8", errors="replace"
    )
    publish = function_body(
        movement,
        "void publish(std::unordered_map<std::string, pending_movement>::iterator found,",
    )
    assert "collector_death_enrollment_note_submitted" not in movement
    assert publish.index("item_ownership_runtime_apply(entry.payload, result)") < publish.index(
        "collector_death_enrollment_note_committed(corpse, entry.payload);"
    )

    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-death-enrollment-", dir=output) as directory:
        binary = Path(directory) / "collector-death-enrollment"
        subprocess.run(
            [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Wpedantic",
                "-Werror",
                "-O2",
                "-I",
                str(ROOT / "src"),
                str(ROOT / "src/persistence/critical_command.c"),
                str(ROOT / "src/item/item_transfer_command.c"),
                str(ROOT / "src/economy/collector_policy.c"),
                str(ROOT / "src/economy/collector_death_enrollment.c"),
                str(ROOT / "tests/async/collector_death_enrollment_harness.cpp"),
                "-lcrypto",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
