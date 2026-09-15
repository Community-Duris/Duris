#!/usr/bin/env python3
"""Bounded asynchronous collector listing detail pipeline regressions."""

from pathlib import Path
import subprocess
import tempfile


ROOT = Path(__file__).resolve().parents[2]


def main() -> None:
    pipeline_source = (ROOT / "src/economy/collector_listing_pipeline.c").read_text()
    selected_source = (ROOT / "src/economy/collector_catalog_source.c").read_text()
    comm_source = (ROOT / "src/net/comm.c").read_text()
    assert "std::condition_variable" in pipeline_source
    assert "COLLECTOR_LISTING_MAX_PENDING" in pipeline_source
    assert "collector_listing_source_load" in pipeline_source
    assert "collector_repository_read_listing" in selected_source
    assert "sql_pool_acquire" in selected_source
    assert "flatfile_collector_repository_read_bootstrap" in selected_source
    assert "flatfile_collector_repository_read_listing" in selected_source
    assert "flat-file collector authority root is unavailable" in selected_source
    assert "collector_listing_pipeline_init()" in comm_source
    assert "collector_listing_pipeline_shutdown()" in comm_source
    assert "P_char" not in (ROOT / "src/economy/collector_listing_pipeline.h").read_text()
    output = ROOT / "bin" / "tests"
    output.mkdir(parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix="collector-listing-pipeline-", dir=output) as directory:
        binary = Path(directory) / "collector-listing-pipeline"
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
                str(ROOT / "src/economy/collector_listing_pipeline.c"),
                str(ROOT / "tests/async/collector_listing_pipeline_harness.cpp"),
                "-pthread",
                "-o",
                str(binary),
            ],
            check=True,
        )
        subprocess.run([str(binary)], check=True, timeout=30)


if __name__ == "__main__":
    main()
