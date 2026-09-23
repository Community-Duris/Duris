#!/usr/bin/env python3
"""Prove negative-weight containers cannot inflate carry weight when nested."""
from pathlib import Path
import os
import subprocess
import tempfile

ROOT = Path(__file__).resolve().parents[2]
source = r'''
#include "item/encumbrance_policy.h"
#include <cassert>
#include <cstdio>

int main()
{
	constexpr int outer_bag = 10;
	constexpr int loaded_net = -2989;
	const int before = encumbrance_weight(outer_bag) + encumbrance_weight(loaded_net);
	const int after = encumbrance_weight(outer_bag + loaded_net);
	assert(before == 10);
	assert(after == 0);
	assert(after - before == -10);
	assert(encumbrance_weight(0) == 0);
	assert(encumbrance_weight(37) == 37);
	std::puts("negative nested container encumbrance remains clamped and conserved PASS");
}
'''

build = ROOT / "bin/tests"
build.mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="negative-container-weight-", dir=build) as tmp:
    path = Path(tmp)
    source_path = path / "test.cpp"
    binary = path / "test"
    source_path.write_text(source)
    subprocess.run([
        "g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
        "-fsanitize=address,undefined", "-fno-omit-frame-pointer", "-fno-pie", "-no-pie",
        "-I", str(ROOT / "src"), str(source_path), "-o", str(binary),
    ], check=True)
    subprocess.run([str(binary)], check=True,
                   env={**os.environ, "ASAN_OPTIONS": "detect_leaks=1:halt_on_error=1"})
