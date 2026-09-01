#!/usr/bin/env python3
"""Regression coverage for the standard/enhanceable Chaos gear selector."""

from __future__ import annotations

import os
import subprocess
import tempfile
from pathlib import Path

from _paths import SRC

ROOT = Path(__file__).resolve().parents[2]
config_h = (SRC / "chaos_config.h").read_text(encoding="utf-8", errors="replace")
config_c = (SRC / "chaos_config.c").read_text(encoding="utf-8", errors="replace")
assert 'getenv("CHAOS_EQ_PROFILE")' in config_c
assert 'strcmp(value, "enhanceable") == 0' in config_c
assert "CHAOS_EQ_PROFILE=standard" in (ROOT / ".env.example").read_text(encoding="utf-8", errors="replace")
assert "CHAOS_EQ_PROFILE accepts standard (default) or enhanceable." in config_h

harness_source = r'''
#include "combat/chaos_config.h"
#include <stdio.h>
int main(void)
{
    printf("%s\n", chaos_eq_use_enhanceable_profile() ? "enhanceable" : "standard");
    return 0;
}
'''
with tempfile.TemporaryDirectory(prefix="duris-chaos-profile-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(harness_source, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", f"-I{SRC}", str(harness), str(SRC / "chaos_config.c"), "-o", str(binary)],
        check=True,
    )

    def evaluate(value: str | None) -> str:
        env = os.environ.copy()
        if value is None:
            env.pop("CHAOS_EQ_PROFILE", None)
        else:
            env["CHAOS_EQ_PROFILE"] = value
        return subprocess.check_output([str(binary)], env=env, text=True, stderr=subprocess.DEVNULL).strip()

    assert evaluate(None) == "standard"
    assert evaluate("standard") == "standard"
    assert evaluate("enhanceable") == "enhanceable"
    assert evaluate("ENHANCEABLE") == "standard"
    assert evaluate("bogus") == "standard"

print("chaos equipment profile contracts passed")
