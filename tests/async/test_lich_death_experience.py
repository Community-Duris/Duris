#!/usr/bin/env python3
"""Keep lich death's proportional residual XP calculation fractional."""

from pathlib import Path
import subprocess
import tempfile

from _paths import extract_function, source


HELPER = extract_function("combat/fight.c", "static long lich_death_residual_experience(")
DIE = extract_function("combat/fight.c", "void die(P_char ch, P_char killer)")

# The death path must use the shared production helper and keep ordinary death
# policy in the non-lich branch.
assert "GET_EXP(ch) = lich_death_residual_experience(tmp, GET_LEVEL(ch));" in DIE
assert "else\n\t\t{\n\t\t\tloss = gain_exp(ch, NULL, 0, EXP_DEATH);" in DIE
assert "new_exp_table[GET_LEVEL(ch)] / new_exp_table[GET_LEVEL(ch) + 1]" not in DIE
assert "static_cast<double>(new_exp_table[level])" in HELPER
assert "static_cast<double>(new_exp_table[level + 1])" in HELPER

HARNESS = f"""
#include <cassert>

long new_exp_table[64] = {{}};

#define MAX(a, b) ((a) > (b) ? (a) : (b))
{HELPER}

int main()
{{
    // Increasing thresholds must retain the fractional ratio instead of
    // truncating it to zero before the residual is scaled.
    new_exp_table[50] = 20000000L;
    new_exp_table[51] = 40000000L;
    assert(lich_death_residual_experience(10000000L, 50) == 5000000L);

    // Equal thresholds are unchanged by the corrected division.
    new_exp_table[50] = 24000000L;
    new_exp_table[51] = 24000000L;
    assert(lich_death_residual_experience(1234567L, 50) == 1234567L);

    // Preserve the existing minimum-one policy for zero and sub-unit results.
    new_exp_table[50] = 1L;
    new_exp_table[51] = 2L;
    assert(lich_death_residual_experience(0L, 50) == 1L);
    assert(lich_death_residual_experience(1L, 50) == 1L);
    return 0;
}}
"""

with tempfile.TemporaryDirectory(prefix="duris-lich-death-") as temporary:
    root = Path(temporary)
    harness = root / "lich_death.cpp"
    binary = root / "lich_death"
    harness.write_text(HARNESS, encoding="utf-8")
    subprocess.run(
        ["g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
         str(harness), "-o", str(binary)],
        check=True,
    )
    subprocess.run([str(binary)], check=True)

print("lich death proportional residual experience regression passed")
