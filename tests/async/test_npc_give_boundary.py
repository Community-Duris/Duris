#!/usr/bin/env python3
"""Regression contract for player-to-NPC durable item handoffs."""

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

guard = index(body, "if (IS_PC(ch) && IS_NPC(vict) &&")
give = index(body, "obj_from_char(obj);")
quest_guard = index(quest_source, "if (item_command_uses_durable_ownership(offering))")
durable_quest = index(quest_source, "if (submit_durable_quest_offering(ch, pl, quester_id, offering))")
quest_give = index(quest_source, "do_give(pl, arg, -4);")
quest_complete = index(quest_source, "quest_completion(qcp, ch, pl)")

checks = [
    ("all PC-to-NPC give paths inspect durable custody", contains(
        body[guard:guard + 700], "item_command_uses_durable_ownership(obj)")),
    ("PC-to-NPC gives are refused before detaching the item", guard < give),
    ("the refusal explains that NPC custody is not durable", contains(
        body[guard:guard + 700], "custody cannot be saved yet")),
    ("safe durable quest consumption precedes the private NPC handoff",
     quest_guard < durable_quest < quest_give and durable_quest < quest_complete),
    ("unsupported durable offerings are refused before give and completion",
     quest_guard < quest_give and quest_guard < quest_complete and contains(
         quest_source[quest_guard:quest_give], "return (TRUE);")),
    ("private quest/spec paths pass through the shared give guard", contains(
        quest_source, "do_give(pl, arg, -4);") and contains(
            spec_source, "do_give(pl, arg, 0);")),
]

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")
if failed:
    raise SystemExit("\n".join(failed))
print("\nNPC give boundary contract passed")
