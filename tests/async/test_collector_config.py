#!/usr/bin/env python3
"""Collector feature settings, safe defaults, and reload regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    properties = (ROOT / "lib/duris.properties").read_text()
    expected = {
        "collector.enabled=0",
        "collector.collection.delay.seconds=43200",
        "collector.sale.delay.seconds=86400",
        "collector.holding.duration.seconds=604800",
        "collector.price.percent=200",
        "collector.minimum.value.copper=100",
        "collector.maintenance.interval.seconds=60",
        "collector.maintenance.lease.seconds=120",
        "collector.maintenance.batch.limit=32",
    }
    for setting in expected:
        assert setting in properties

    property_source = (ROOT / "src/world/properties.c").read_text()
    assert "collector_config_reload();" in property_source
    presence_source = (ROOT / "src/economy/collector_presence.c").read_text()
    assert "collector_config_enabled()" in presence_source
    assert "collector_config_revision()" in presence_source

    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-config-", dir=output) as directory:
        binary = Path(directory) / "collector-config"
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
                str(ROOT / "src/economy/collector_policy.c"),
                str(ROOT / "src/economy/collector_config.c"),
                str(ROOT / "tests/async/collector_config_harness.cpp"),
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
