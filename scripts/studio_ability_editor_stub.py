#!/usr/bin/env python3
"""Offline private-editor handoff stub: validate, round-trip, emit .trg references.

This does not connect to an editor or server. The compiled server-model validator
is required so the stub cannot silently invent a second semantic validator.
"""
from __future__ import annotations
import argparse
import json
import os
from pathlib import Path
import subprocess
import tempfile


def save_atomic(path: Path, text: str) -> None:
    with tempfile.NamedTemporaryFile(mode="w", encoding="utf-8", dir=path.parent,
                                     prefix=".studio-ability-", delete=False) as stream:
        temporary = Path(stream.name)
        try:
            stream.write(text); stream.flush(); os.fsync(stream.fileno())
        except Exception:
            temporary.unlink(missing_ok=True)
            raise
    try:
        os.replace(temporary, path)
    finally:
        temporary.unlink(missing_ok=True)


def run(input_path: Path, output_path: Path, validator: Path, trg_path: Path | None) -> None:
    if trg_path and trg_path.resolve() == output_path.resolve():
        raise ValueError("catalog and trigger output paths must differ")
    subprocess.run([str(validator.resolve()), str(input_path.resolve())], check=True)
    data = json.loads(input_path.read_text(encoding="utf-8"))
    # Keys may reorder; stable IDs, resource identities and effect order may not.
    text = json.dumps(data, ensure_ascii=False, indent=2, sort_keys=True, allow_nan=False)+"\n"
    with tempfile.TemporaryDirectory(prefix="studio-editor-stub-") as directory:
        candidate = Path(directory) / "catalog.json"
        candidate.write_text(text, encoding="utf-8")
        subprocess.run([str(validator.resolve()), str(candidate)], check=True)
    records: dict[int, list[dict]] = {}
    for ability in data["abilities"]:
        records.setdefault(ability["vnum"], []).append(ability)
    lines = ["* Generated editor stub references. Review before installing in any world."]
    for vnum, abilities in sorted(records.items()):
        lines.append(f"#{vnum} O")
        for ability in sorted(abilities, key=lambda value: value["id"]):
            lines += ["T HIT" if ability["trigger"] == "hit" else "T CMD use",
                      f"itemability {ability['id']}", "~"]
        lines.append("S")
    lines.append("#~")
    save_atomic(output_path, text)
    if trg_path: save_atomic(trg_path, "\n".join(lines)+"\n")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("input", type=Path)
    parser.add_argument("output", type=Path)
    parser.add_argument("--validator", type=Path, required=True)
    parser.add_argument("--emit-trg", type=Path)
    args = parser.parse_args()
    run(args.input, args.output, args.validator, args.emit_trg)
