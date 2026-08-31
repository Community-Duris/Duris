#!/usr/bin/env python3
"""Contract for configurable NPC death essence drops."""
from _paths import SRC
from pathlib import Path
from contract_text import contains

ROOT = Path(__file__).resolve().parents[2]
enhance = (SRC / "enhance.c").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

for key in (
    "enhance.essence_drop.enabled=1",
    "enhance.essence_drop.primary_roll_max=3000",
    "enhance.essence_drop.max_roll_max=4000",
    "enhance.essence_drop.elite_level_multiplier=1",
):
    assert key in config, f"missing documented essence-drop setting: {key}"

assert contains(config, "[essence_drop]")
assert contains(enhance, "enhance_essence_drop_enabled")
assert contains(enhance, "enhance_essence_primary_roll_max")
assert contains(enhance, "enhance_essence_max_roll_max")
assert contains(enhance, "enhance_essence_elite_level_multiplier")
assert contains(enhance, '"essence_drop"')
assert contains(enhance, '"enhance.essence_drop.enabled"')
assert contains(enhance, '"enhance.essence_drop.primary_roll_max"')
assert contains(enhance, '"enhance.essence_drop.max_roll_max"')
assert contains(enhance, '"enhance.essence_drop.elite_level_multiplier"')

# The drop remains opt-in only through its own master switch and preserves the
# two-roll design. Defaults must retain current normal-NPC behavior.
assert contains(enhance, "if (!enhance_essence_drop_enabled)")
assert contains(enhance, "int primary_roll_max = enhance_essence_primary_roll_max;")
assert contains(enhance, "int max_roll_max     = enhance_essence_max_roll_max;")
assert contains(enhance, "number(1, primary_roll_max) < moblvl")
assert contains(enhance, "number(1, max_roll_max) < moblvl")
assert contains(enhance, "int elite_mult       = enhance_essence_elite_level_multiplier;")
assert contains(enhance, "moblvl *= elite_mult;")

print("enhancement essence-drop config contract passed")
