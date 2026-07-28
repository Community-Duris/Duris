#!/usr/bin/env python3
"""Legacy essence modifier stacking has one live pre-pwipe cap setting."""
from pathlib import Path
root = Path(__file__).resolve().parents[2]
src = (root / "src/enhance.c").read_text()
cfg = (root / "lib/enhance.cfg").read_text()
assert "enhance_mod_max_steps" in src
assert '"enhance.mod.max.steps"' in src
assert "modifier / mod < enhance_mod_max_steps" in src
assert "enhance.mod.max.steps=3" in cfg
print("enhancement modifier-step config contract passed")
