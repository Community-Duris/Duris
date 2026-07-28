#!/usr/bin/env python3
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
salvage = (ROOT / "src/salvage.c").read_text()
actobj = (ROOT / "src/actobj.c").read_text()
makefile = (ROOT / "src/Makefile").read_text()

for symbol in ("is_salvageable", "salvage_examine_item", "do_salvage"):
    assert symbol in salvage, symbol
assert "void do_salvage(" not in actobj
assert "bool is_salvageable(" not in actobj
assert "salvage.o" in makefile
print("salvage module contract passed")
