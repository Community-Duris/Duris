#!/usr/bin/env python3
"""Genuine Card/Hand score and legal ownership acceptance, without gambling admission.

Compile the complete production cardgames.c TU on Linux/WSL. Linker collection
excludes the unused table, shuffle, currency, authority and event paths. The
sibling driver uses only actual public operations and manual expectations.
Use --source-root for a frozen candidate, --build-dir or BIN_ROOT for fresh
retained output, and TMPDIR for task-owned compiler scratch.
"""

import argparse
import hashlib
import json
import os
from pathlib import Path
import shlex
import shutil
import subprocess
import time

ROOT = Path(__file__).resolve().parents[2]
CASES = ("empty",) + tuple(f"single_{value:02d}" for value in range(1, 53)) + (
    "ace_nine_ten", "ace_ten", "two_aces", "two_aces_nine", "two_aces_ten",
    "four_aces_nine", "bust", "five_cards", "soft_seventeen", "hard_ace",
    "order_ten_ace_nine", "order_nine_ten_ace", "four_suits_two", "three_faces",
)


def sha256(path):
    return hashlib.sha256(path.read_bytes()).hexdigest()


def run_logged(command, log_path, timeout, **kwargs):
    """Preserve nonzero diagnostics and timeout partial output in the same log."""
    with log_path.open("w") as log:
        try:
            return subprocess.run(command, stdout=log, stderr=subprocess.STDOUT,
                                  timeout=timeout, **kwargs)
        except subprocess.TimeoutExpired:
            log.write("\nTIMEOUT after " + str(timeout) + " seconds\n")
            return subprocess.CompletedProcess(command, 124)


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--source-root", type=Path, default=ROOT)
    parser.add_argument("--build-dir", type=Path)
    parser.add_argument("--optimization", choices=("O1", "Og"), default="O1")
    args = parser.parse_args()
    root = args.source_root.resolve()
    output = (args.build_dir or Path(os.environ.get("BIN_ROOT", ROOT / "bin")) /
              "tests" / "blackjack-hand-native-values" /
              (args.optimization + "-" + str(time.time_ns()))).resolve()
    output.mkdir(parents=True, exist_ok=False)
    harness_source = Path(__file__).with_name("blackjack_hand_native_values_harness.cpp")
    harness = output / "driver.cpp"
    harness.write_bytes(harness_source.read_bytes())

    def recorded(command, name, timeout, **kwargs):
        record = {"command": command, "timeout_seconds": timeout, "cwd": str(root)}
        metadata = output / (name + "-command.json")
        metadata.write_text(json.dumps(record, indent=2))
        start = time.monotonic()
        result = run_logged(command, output / (name + ".log"), timeout, cwd=root, **kwargs)
        record.update(returncode=result.returncode, seconds=time.monotonic() - start)
        metadata.write_text(json.dumps(record, indent=2))
        assert result.returncode == 0, str(output / (name + ".log"))

    compiler = shlex.split(os.environ.get("CXX", "g++"))
    recorded(compiler + ["--version"], "compiler-version", 20)
    compiler_path = Path(shutil.which(compiler[0])).resolve()
    tool_pins = {"compiler": {"path": str(compiler_path), "sha256": sha256(compiler_path)}}
    for name in ("cc1plus", "collect2", "as", "ld"):
        recorded(compiler + ["-print-prog-name=" + name], name + "-path", 20)
        value = (output / (name + "-path.log")).read_text().strip()
        path = Path(value if "/" in value else shutil.which(value)).resolve()
        tool_pins[name] = {"path": str(path), "sha256": sha256(path)}
    for name in ("nm", "ldd"):
        path = Path(shutil.which(name)).resolve()
        tool_pins[name] = {"path": str(path), "sha256": sha256(path)}
    (output / "tool-pins.json").write_text(json.dumps(tool_pins, indent=2))

    flags = ["-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
             "-" + args.optimization, "-g", "-fsanitize=address,undefined",
             "-fno-omit-frame-pointer", "-fno-pie", "-ffunction-sections",
             "-fdata-sections", "-D__NO_MYSQL__", "-Isrc", "-Isrc/no_mysql", "-I."]
    production = root / "src/economy/cardgames.c"
    header = root / "src/economy/cardgames.h"
    direct = (production, header, harness, harness_source, Path(__file__).resolve())
    before = {str(path): sha256(path) for path in direct}
    (output / "direct-input-pins.json").write_text(json.dumps(before, indent=2))
    for index, source in enumerate((production, harness)):
        command = compiler + flags + ["-MD", "-MF", str(output / (str(index) + ".d")),
                                      "-c", str(source), "-o", str(output / (str(index) + ".o"))]
        recorded(command, str(index) + "-compile", 900)
    dependencies = set(direct)
    for depfile in output.glob("*.d"):
        value = depfile.read_text().replace(chr(92) + chr(10), " ").split(":", 1)[1]
        dependencies.update((root / token).resolve() for token in shlex.split(value))
    pins = {str(path): sha256(path) for path in sorted(dependencies)}
    assert all(pins[path] == digest for path, digest in before.items())
    (output / "input-pins.json").write_text(json.dumps(pins, indent=2))

    executable = output / "blackjack-hand-native-values"
    link = compiler + flags + ["-no-pie", str(output / "0.o"), str(output / "1.o"),
                               "-Wl,--gc-sections", "-Wl,-Map=" + str(output / "link.map"),
                               "-o", str(executable)]
    recorded(link, "link", 120)
    recorded([tool_pins["nm"]["path"], "-C", "--defined-only", str(executable)],
             "defined-symbols", 20)
    symbols = (output / "defined-symbols.log").read_text()
    entries = [line.split(maxsplit=2) for line in symbols.splitlines()]
    functions = "\n".join(entry[2] for entry in entries
                          if len(entry) == 3 and entry[1] in ("t", "T", "w", "W", "i", "I"))
    assert "Hand::BlackjackValue()" in functions
    excluded = ("blackjack_table(", "event_dealersturn(", "cards_object(", "Deck::",
                "Hand::Display(", "Card::Display", "economic_gameplay_authority::",
                "currency_transaction_", "SUB_MONEY(", "add_event(", "number(", "get_property(")
    for symbol in excluded:
        assert symbol not in functions, symbol
    # ASan global registration can keep a discarded function's static data.
    # Disclose those data symbols; their names are not live function bodies.
    retained_data = [entry for entry in entries if len(entry) == 3
                     and entry[1] not in ("t", "T", "w", "W", "i", "I")
                     and any(symbol in entry[2] for symbol in excluded)]
    (output / "retained-provider-data.json").write_text(json.dumps(retained_data, indent=2))
    discarded = (output / "link.map").read_text().split("Memory Configuration", 1)[0]
    for section in (".text._Z15blackjack_tableP8obj_dataP9char_dataiPc",
                    ".text._ZN4Deck7ShuffleEi", ".text._Z17event_dealersturnP9char_dataS0_P8obj_dataPv",
                    ".text._Z12cards_objectP8obj_dataP9char_dataiPc"):
        assert section in discarded, section
    recorded([tool_pins["ldd"]["path"], str(executable)], "shared-libraries", 20)
    library_pins = {}
    for line in (output / "shared-libraries.log").read_text().splitlines():
        path = next((Path(field) for field in line.split() if field.startswith("/")), None)
        if path:
            library_pins[str(path.resolve())] = sha256(path)
    (output / "shared-library-pins.json").write_text(json.dumps(library_pins, indent=2))

    environment = dict(os.environ, ASAN_OPTIONS="detect_leaks=1:halt_on_error=1",
                       UBSAN_OPTIONS="halt_on_error=1:print_stacktrace=1")
    (output / "runtime-profile.json").write_text(json.dumps(
        {"optimization": args.optimization, "TMPDIR": environment.get("TMPDIR"),
         "ASAN_OPTIONS": environment["ASAN_OPTIONS"],
         "UBSAN_OPTIONS": environment["UBSAN_OPTIONS"],
         "gambling_native_publication_qualification": False}, indent=2))
    runs = []
    for case in CASES:
        command = [str(executable), case]
        recorded(command, case, 30, env=environment)
        expected = f"PASS {case} genuine_hand_score_ownership=1 gambling_native_publication=0\n"
        assert (output / (case + ".log")).read_text() == expected, case
        runs.append({"case": case, "command": command, "returncode": 0, "timeout_seconds": 30})
        (output / "runs.json").write_text(json.dumps(runs, indent=2))
    assert all(sha256(Path(path)) == digest for path, digest in pins.items())
    assert all(sha256(Path(pin["path"])) == pin["sha256"] for pin in tool_pins.values())
    assert all(sha256(Path(path)) == digest for path, digest in library_pins.items())
    artifacts = {path.name: sha256(path) for path in sorted(output.iterdir()) if path.is_file()}
    (output / "artifact-pins.json").write_text(json.dumps(artifacts, indent=2))
    print(json.dumps({"passed": len(runs), "optimization": args.optimization,
                      "output": str(output), "elf_sha256": sha256(executable),
                      "gambling_native_publication_qualification": False}, indent=2))


if __name__ == "__main__":
    main()
