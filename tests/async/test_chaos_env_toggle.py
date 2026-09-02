#!/usr/bin/env python3
"""Regression coverage for the runtime CHAOS_MUD environment toggle."""

from _paths import SRC
import os
import subprocess
import tempfile
from pathlib import Path

from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]
makefile = (SRC / "Makefile").read_text()
example = (ROOT / ".env.example").read_text()
config = (SRC / "chaos_config.c").read_text()
consumers = "\n".join(
    (SRC / name).read_text()
    for name in ("guild.c", "memorize.c", "mobconv.c", "nanny.c")
)

assert "-DCHAOS_MUD" not in makefile
assert "CHAOS_MUD=FALSE" in example
assert "CHAOS_MUD=1" not in example
assert "CHAOS_MUD=0" not in example
assert "CHAOS_TEST_COMMANDS=FALSE" in example
assert contains(config, 'getenv("CHAOS_MUD")')
assert "CHAOS_TEST_COMMANDS" in config
assert 'strcmp(environment, "local")' in config
assert contains(config, 'strcmp(value, "TRUE") == 0')
for switch in (
    "CHAOS_STARTER_BONUSES",
    "CHAOS_STARTER_FRIGATE",
    "CHAOS_STARTER_EPIC_SKILLS",
    "CHAOS_STARTER_EPIC_POINTS",
    "CHAOS_STARTER_BANK_PLATINUM",
    "CHAOS_STARTER_MATERIALS",
):
    assert switch in config
    assert switch in example
assert "defined(CHAOS_MUD)" not in consumers
assert "chaos_mud_enabled()" in consumers

HARNESS = r"""
#include "combat/chaos_config.h"

#include <stdio.h>

int main(void)
{
	printf("%s\n", chaos_mud_enabled() ? "TRUE" : "FALSE");
	return 0;
}
"""

STARTER_HARNESS = r"""
#include "combat/chaos_config.h"

#include <stdio.h>

int main(void)
{
	printf("%d%d%d%d%d%d\n", chaos_starter_bonuses_enabled(),
	       chaos_starter_frigate_enabled(), chaos_starter_epic_skills_enabled(),
	       chaos_starter_epic_points_enabled(), chaos_starter_bank_platinum_enabled(),
	       chaos_starter_materials_enabled());
	return 0;
}
"""

with tempfile.TemporaryDirectory(prefix="duris-chaos-toggle-") as temp_dir:
    temp = Path(temp_dir)
    harness = temp / "harness.cpp"
    binary = temp / "harness"
    harness.write_text(HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            f"-I{SRC}",
            str(harness),
            str(SRC / "chaos_config.c"),
            "-o",
            str(binary),
        ],
        check=True,
    )

    def evaluate(value):
        env = os.environ.copy()
        if value is None:
            env.pop("CHAOS_MUD", None)
        else:
            env["CHAOS_MUD"] = value
        return subprocess.check_output([str(binary)], env=env, text=True).strip()

    starter_harness = temp / "starter_harness.cpp"
    starter_binary = temp / "starter_harness"
    starter_harness.write_text(STARTER_HARNESS)
    subprocess.run(
        [
            "g++",
            "-std=c++20",
            f"-I{SRC}",
            str(starter_harness),
            str(SRC / "chaos_config.c"),
            "-o",
            str(starter_binary),
        ],
        check=True,
    )

    starter_switches = (
        "CHAOS_STARTER_BONUSES",
        "CHAOS_STARTER_FRIGATE",
        "CHAOS_STARTER_EPIC_SKILLS",
        "CHAOS_STARTER_EPIC_POINTS",
        "CHAOS_STARTER_BANK_PLATINUM",
        "CHAOS_STARTER_MATERIALS",
    )

    def evaluate_starters(chaos, **overrides):
        env = os.environ.copy()
        env["CHAOS_MUD"] = chaos
        for switch in starter_switches:
            env.pop(switch, None)
        env.update(overrides)
        return subprocess.check_output([str(starter_binary)], env=env, text=True).strip()

    assert evaluate("TRUE") == "TRUE"
    assert evaluate("FALSE") == "FALSE"
    assert evaluate(None) == "FALSE"
    assert evaluate("1") == "FALSE"
    assert evaluate("0") == "FALSE"
    assert evaluate("true") == "FALSE"

    assert evaluate_starters("TRUE") == "111111"
    assert evaluate_starters("TRUE", CHAOS_STARTER_BONUSES="FALSE") == "000000"
    assert evaluate_starters("TRUE", CHAOS_STARTER_FRIGATE="FALSE") == "101111"
    assert evaluate_starters("TRUE", CHAOS_STARTER_EPIC_SKILLS="FALSE") == "110111"
    assert evaluate_starters("TRUE", CHAOS_STARTER_EPIC_POINTS="FALSE") == "111011"
    assert evaluate_starters("TRUE", CHAOS_STARTER_BANK_PLATINUM="FALSE") == "111101"
    assert evaluate_starters("TRUE", CHAOS_STARTER_MATERIALS="FALSE") == "111110"
    assert evaluate_starters("TRUE", CHAOS_STARTER_MATERIALS="MAYBE") == "111110"
    assert evaluate_starters(
        "FALSE",
        CHAOS_STARTER_BONUSES="TRUE",
        CHAOS_STARTER_FRIGATE="TRUE",
        CHAOS_STARTER_EPIC_SKILLS="TRUE",
        CHAOS_STARTER_EPIC_POINTS="TRUE",
        CHAOS_STARTER_BANK_PLATINUM="TRUE",
        CHAOS_STARTER_MATERIALS="TRUE",
    ) == "000000"

print("chaos environment and starter-gate contracts passed")
