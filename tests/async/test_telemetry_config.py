#!/usr/bin/env python3
"""Focused #263 effective-config identity and publication tests."""

from __future__ import annotations

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]
SOURCE = ROOT / "src" / "telemetry" / "telemetry_config.c"
PRIVATE_HEADER = ROOT / "src" / "telemetry" / "telemetry_config_private.h"
HARNESS = ROOT / "tests" / "async" / "telemetry_config_harness.cc"
DOC = ROOT / "docs" / "telemetry" / "CONFIG_CONTEXT.md"
PROPERTIES = ROOT / "lib" / "duris.properties"
GOLDEN = "71daf2a6e0a9faa4f200f855b88426d6cac93b9b3cead7d66046ac0263e0daf7"


def check_source_boundary() -> None:
    source = SOURCE.read_text(encoding="utf-8")
    header = PRIVATE_HEADER.read_text(encoding="utf-8")
    harness = HARNESS.read_text(encoding="utf-8")
    document = DOC.read_text(encoding="utf-8")

    assert "telemetry_config_snapshot_copy(void)" in source
    assert "telemetry_config_validation telemetry_config_status(void)" in source
    assert "telemetry_capture_result telemetry_config_publish(" in source
    assert "SHA256(" in source
    assert "TELEMETRY_CONFIG_CANONICAL_BYTES = 70U" in header
    assert "TELEMETRY_CONFIG_PROPERTY_DIGEST_BYTES" in header
    assert "telemetry_config_property_catalog" in header
    assert "visibility_floor_revision" in header
    assert "canonical" in header
    assert "telemetry_config_property_role::maintained_but_unused" in source
    assert "property_catalog_unavailable" in source
    assert "revision_conflict" in source
    assert "1994669249U" in harness
    assert "0x3F00908CU" in harness
    assert "0x3F01C902U" in harness
    assert "payout.zone_alignment_context" in source
    assert "DURISWEB_SECRET" not in source
    assert "DB_PASSWD" not in source
    assert "std::vector" not in source
    assert "std::thread" not in source
    assert "malloc(" not in source
    assert "calloc(" not in source
    assert "realloc(" not in source
    assert "new telemetry" not in source
    assert "process-local" in document
    assert "epic.freqMod" in document
    assert "unknown" in document.lower()


def check_reviewed_property_values() -> None:
    values: dict[str, str] = {}
    for line in PROPERTIES.read_text(encoding="utf-8").splitlines():
        if "=" not in line or line.lstrip().startswith(("#", "[")):
            continue
        key, value = line.split("=", 1)
        values[key.strip()] = value.strip()
    assert values["exp.zoneTrophy.observe"] == "0"
    assert values["epic.touch.maxPayoutFactor"] == "10.000"
    assert values["epic.touch.PayoutFactor"] == "1.000"
    assert values["epic.zone.alignmentMod"] == "0.200"
    assert values["epic.alignment.minPercentage"] == "0.150"
    assert values["epic.freqMod.tick.waitSecs"] == "3600.000"
    assert values["epic.freqMod.tick.add"] == "0.002"
    assert values["epic.freqMod.touch.sub"] == "0.200"
    assert values["epic.freqMod.min"] == "0.050"
    assert values["epic.freqMod.max"] == "1.550"


def run_harness() -> None:
    with tempfile.TemporaryDirectory(prefix="telemetry-config-") as directory:
        for suffix, sanitizer in (("", ()), ("-sanitized", ("-fsanitize=address,undefined", "-fno-omit-frame-pointer"))):
            executable = str(Path(directory) / f"telemetry_config_harness{suffix}")
            command = [
                "g++",
                "-std=c++20",
                "-Wall",
                "-Wextra",
                "-Werror",
                *sanitizer,
                "-I",
                "src",
                str((HARNESS.parent / "telemetry_config_review.cc").relative_to(ROOT)),
                str(SOURCE.relative_to(ROOT)),
                "-lcrypto",
                "-o",
                executable,
            ]
            subprocess.run(command, cwd=ROOT, check=True)
            completed = subprocess.run(
                [executable], cwd=ROOT, check=False, text=True, capture_output=True
            )
            print(completed.stdout, end="")
            print(completed.stderr, end="")
            completed.check_returncode()
            assert GOLDEN in completed.stdout
            assert "telemetry config focused checks passed" in completed.stdout


def main() -> None:
    check_source_boundary()
    check_reviewed_property_values()
    run_harness()
    print("test_telemetry_config.py: passed")


if __name__ == "__main__":
    main()
