#!/usr/bin/env python3
"""number() counts its span in 64 bits.

number(0, 2147483647), which the account confirmation code and the name generator call,
computed `to - from + 1` in an int: the span overflowed, and half the results were negative.
The real function runs here under UBSan over the widest spans.
"""
from pathlib import Path
import subprocess
import tempfile

from _paths import ROOT, rel

HARNESS = r'''
#include <cassert>
#include <climits>
#include <cstdint>
#include <cstdio>
void randomize(uint64_t seed);
int number(int from, int to);
int main()
{
    randomize(42);
    bool low = false, high = false;
    for (int roll = 0; roll < 100000; ++roll)
    {
        assert(number(0, INT_MAX) >= 0);
        assert(number(INT_MAX, 0) >= 0);
        const int any = number(INT_MIN, INT_MAX);
        low = low || any < 0;
        high = high || any > 0;
        assert(number(INT_MAX, INT_MIN) <= INT_MAX);
        const int die = number(1, 6);
        assert(die >= 1 && die <= 6);
        const int back = number(6, 1);
        assert(back >= 1 && back <= 6);
        assert(number(-3, -3) == -3);
    }
    assert(low && high);
    std::puts("number() keeps its range over the widest spans");
}
'''

(ROOT / "bin/tests").mkdir(parents=True, exist_ok=True)
with tempfile.TemporaryDirectory(prefix="number-span-", dir=ROOT / "bin/tests") as tmp:
    source, binary = Path(tmp) / "test.cpp", Path(tmp) / "test"
    source.write_text(HARNESS)
    subprocess.run(["g++", "-std=c++20", "-g", "-Wall", "-Wextra", "-Werror",
                    "-fsanitize=undefined", "-fno-sanitize-recover=undefined",
                    str(source), rel("random.c"), "-o", str(binary)], cwd=ROOT, check=True)
    subprocess.run([str(binary)], check=True)
