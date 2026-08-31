#!/usr/bin/env python3
from _paths import SRC
from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
source = (SRC / "ships" / "ship_shop.c").read_text()

shop = source[source.index("int ship_shop_proc(") :]
list_branch = shop[shop.index("if (cmd == CMD_LIST)") : shop.index("if (cmd == CMD_BUY)")]

assert "if (!*arg1)" in list_branch
assert "return list_hulls(ch, ship, owned);" in list_branch
assert list_branch.index("if (!*arg1)") < list_branch.index("if (*arg1)")

print("[PASS] bare list at a ship shop defaults to the purchasable hull catalog")
