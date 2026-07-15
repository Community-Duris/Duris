#!/usr/bin/env python3
"""Contract for configurable NPC death essence drops."""
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
enhance = (ROOT / "src/enhance.c").read_text()
config = (ROOT / "lib/enhance.cfg").read_text()

for key in (
    "enhance.essence_drop.enabled=1",
    "enhance.essence_drop.primary_roll_max=3000",
    "enhance.essence_drop.max_roll_max=4000",
    "enhance.essence_drop.elite_level_multiplier=1",
):
    assert key in config, f"missing documented essence-drop setting: {key}"

assert "[essence_drop]" in config
assert "enhance_essence_drop_enabled" in enhance
assert "enhance_essence_primary_roll_max" in enhance
assert "enhance_essence_max_roll_max" in enhance
assert "enhance_essence_elite_level_multiplier" in enhance
assert '"essence_drop"' in enhance
assert '"enhance.essence_drop.enabled"' in enhance
assert '"enhance.essence_drop.primary_roll_max"' in enhance
assert '"enhance.essence_drop.max_roll_max"' in enhance
assert '"enhance.essence_drop.elite_level_multiplier"' in enhance

# The drop remains opt-in only through its own master switch and preserves the
# two-roll design. Defaults must retain current normal-NPC behavior.
assert "if (!enhance_essence_drop_enabled)" in enhance
assert "number(1, enhance_essence_primary_roll_max) < moblvl" in enhance
assert "number(1, enhance_essence_max_roll_max) < moblvl" in enhance
assert "moblvl *= enhance_essence_elite_level_multiplier;" in enhance

print("enhancement essence-drop config contract passed")
