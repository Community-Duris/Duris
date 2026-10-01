#!/usr/bin/env python3
"""Regression contract for the regular player-to-NPC durable item boundary."""

from _paths import SRC
from contract_text import contains, index


source = (SRC / "actobj.c").read_text(encoding="utf-8", errors="replace")
quest_source = (SRC / "world" / "quest.c").read_text(encoding="utf-8", errors="replace")
spec_source = (SRC / "specs" / "specs.mobile.c").read_text(encoding="utf-8", errors="replace")
start = index(source, "void do_give(P_char ch, char *argument, int cmd)")
opening = source.index("{", start)
depth = 1
end = opening + 1
while depth:
    depth += (source[end] == "{") - (source[end] == "}")
    end += 1
body = source[start:end]

guard = index(body, "if (cmd == CMD_GIVE && IS_PC(ch) && IS_NPC(vict) &&")
give = index(body, "obj_from_char(obj);")

checks = [
    ("regular PC-to-NPC gives inspect the command boundary", contains(
        body[guard:guard + 700], "item_command_uses_durable_ownership(obj)")),
    ("regular PC-to-NPC gives are refused before detaching the item", guard < give),
    ("the refusal explains that NPC custody is not durable", contains(
        body[guard:guard + 700], "custody cannot be saved yet")),
    ("internal quest/spec callers retain their explicit private command paths", contains(
        body[guard:guard + 700], "cmd == CMD_GIVE") and contains(
            quest_source, "do_give(pl, arg, -4);") and contains(
            spec_source, "do_give(pl, arg, 0);")),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit("\n".join(failed))
print("\nNPC give boundary contract passed")
