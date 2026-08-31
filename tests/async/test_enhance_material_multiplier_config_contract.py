#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path
root=Path(__file__).resolve().parents[2]
s=(SRC / "enhance.c").read_text(); c=(root/'lib/enhance.cfg').read_text()
assert 'enhance_stat_material_quantity_multiplier' in s
assert '"enhance_stat.material.quantity.mult"' in s
assert 'enhance_stat.material.quantity.mult=1.0' in c
print('superior material multiplier config contract passed')
