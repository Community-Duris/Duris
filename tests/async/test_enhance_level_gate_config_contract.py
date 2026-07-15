#!/usr/bin/env python3
"""Legacy enhancement level gate is a documented live configuration control."""
from pathlib import Path
src = (Path(__file__).resolve().parents[2] / "src/enhance.c").read_text()
cfg = (Path(__file__).resolve().parents[2] / "lib/enhance.cfg").read_text()
assert "enhance_level_gate_multiplier" in src
assert '"enhance.level.gate.multiplier"' in src
assert "GET_LEVEL(ch) * enhance_level_gate_multiplier" in src
assert "enhance.level.gate.multiplier=3" in cfg
for obsolete in ("enhance_level_gate_a", "enhance_level_gate_b", "enhance_level_gate_c", '"enhance.level.gate.a"', '"enhance.level.gate.b"', '"enhance.level.gate.c"'):
    assert obsolete not in src, obsolete
print("enhancement level gate config contract passed")
