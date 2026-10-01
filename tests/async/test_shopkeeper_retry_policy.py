#!/usr/bin/env python3
"""Compile and exercise the production dirty-shopkeeper retry policy."""

from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source_text = (ROOT / "src/economy/shopkeeper_save_policy.h").read_text()
structs_text = (ROOT / "src/core/structs.h").read_text()
assert "dirty_save_retry" in structs_text
assert "SHOPKEEPER_SAVE_RETRY_MAX_SECONDS 900U" in source_text

harness = r'''
#include "economy/shopkeeper_save_policy.h"
#include <cassert>
#include <cstdio>

int main()
{
    shopkeeper_save_retry_state retry = {};
    bool dirty = true;
    unsigned int attempts = 0;
    const time_t start = 1000;
    assert(shopkeeper_save_retry_due(&retry, start, false));

    ++attempts;
    shopkeeper_save_retry_record_failure(&retry, start);
    assert(dirty && attempts == 1);
    assert(!shopkeeper_save_retry_due(&retry, start + 59, false));
    assert(shopkeeper_save_retry_due(&retry, start + 60, false));

    ++attempts;
    dirty = false;
    shopkeeper_save_retry_reset(&retry);
    assert(!dirty && attempts == 2);

    dirty = true;
    shopkeeper_save_retry_record_failure(&retry, start);
    assert(retry.failure_count == 1);
    assert(retry.next_retry_at == start + 60);
    assert(!shopkeeper_save_retry_due(&retry, start + 59, false));
    assert(shopkeeper_save_retry_due(&retry, start + 60, false));

    for (unsigned int i = 0; i < 16; ++i)
        shopkeeper_save_retry_record_failure(&retry, start + 60);
    assert(shopkeeper_save_retry_delay_seconds(retry.failure_count) == 900);
    assert(!shopkeeper_save_retry_due(&retry, start + 61, false));
    assert(shopkeeper_save_retry_due(&retry, start + 61, true));

    shopkeeper_save_retry_reset(&retry);
    assert(retry.failure_count == 0 && retry.next_retry_at == 0);
    assert(shopkeeper_save_retry_due(&retry, start, false));
    std::puts("shopkeeper retry policy runtime contract passed");
}
'''

build_root = ROOT / "bin/tests"
build_root.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="shopkeeper-retry-", dir=build_root) as tmp:
    tmp_path = Path(tmp)
    source = tmp_path / "harness.cpp"
    binary = tmp_path / "harness"
    source.write_text(harness)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-I",
            str(ROOT / "src"),
            str(source),
            "-o",
            str(binary),
        ],
        check=True,
    )
    subprocess.run([str(binary)], check=True, env=os.environ.copy())
