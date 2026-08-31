#!/usr/bin/env python3
"""Regression test for corpse handoffs that race an inbound item movement.

An operator created an artifact and gave it to a player whose racewar did not
match it. The sword's periodic proc detached the sword and cast a lightning
bolt that killed the holder while the give was still in flight, which produced

    domain=corpse action=failed_preserved detail=item_uid=...

even though nothing was lost. Two source contracts are at fault:

1. item_movement_transaction_submit() refuses whenever movement_conflicts()
   finds the player on either side of a pending entry, but
   item_movement_transaction_player_busy() only reported movements the player
   themselves submitted. A give or an operator creation grant aimed at the
   player therefore left death believing the pipeline was idle: make_corpse()
   submitted the first corpse transfer, it was refused, and the refusal was
   logged as a preservation failure.

2. holy_weapon()'s racewar jump detached the sword with obj_from_char() and only
   rehomed it after spell_lightning_bolt(), so a death triggered by that bolt
   ran make_corpse() with the sword in limbo - on no character, no room and no
   container.
"""

from _paths import SRC
from pathlib import Path
import sys

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
fight = (SRC / "fight.c").read_text(encoding="utf-8", errors="replace")
movement = (SRC / "item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace")
specs = (SRC / "specs.object.c").read_text(encoding="utf-8", errors="replace")


def body(text, signature):
    """Return one function from its signature through its closing brace."""
    start = index(text, signature)
    depth = 0
    opening = text.index("{", start)
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start:position + 1]
    raise AssertionError(f"unterminated function: {signature}")


checks = []

busy = body(movement, "bool item_movement_transaction_player_busy(P_char actor)")
checks.append((
    "busy covers movements submitted toward the player, not just by them",
    contains(busy, "const item_owner_identity owner = { item_owner_type::player, pid, 0 };")
    and contains(busy, "owner_conflicts(entry.second, owner)")
    and contains(busy, "entry.second.actor_pid == pid")
))
checks.append((
    "busy covers a creation grant queued for the player as recipient",
    contains(busy, "for (const pending_creation_grant &request : queue.requests)")
    and contains(busy, "if (!request.to_room && request.recipient_pid == pid)")
))
checks.append((
    "busy still reports the player's own queued creation grants",
    contains(busy, "if (creation_grants.find(pid) != creation_grants.end())")
))

corpse = body(fight, "P_obj make_corpse(P_char ch, int loss)")
checks.append((
    "make_corpse defers the first handoff instead of logging failed_preserved",
    contains(corpse, "if (!item_movement_transaction_player_busy(ch))")
    and corpse.index("if (!item_movement_transaction_player_busy(ch))")
    < corpse.index("(void)submit_next_corpse_item(ch, corpse);")
))
checks.append((
    "the deferred corpse is still written and the owner still marked dirty",
    contains(corpse, "writeCorpse(corpse);")
    and corpse.index("writeCorpse(corpse);")
    < corpse.index("if (!item_movement_transaction_player_busy(ch))")
))

blade = body(specs, "int holy_weapon(P_obj obj, P_char ch, int cmd, char *arg)")
jump = blade[index(blade, "should_jump"):]
checks.append((
    "the jumped sword reaches its new owner before the bolt can kill the old one",
    contains(jump, "obj_to_char(obj, tch);")
    and jump.index("obj_to_char(obj, tch);")
    < jump.index("spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, ch, 0);")
))
checks.append((
    "the vanishing sword is extracted before the bolt can kill its holder",
    contains(jump, "extract_obj(obj, TRUE); // Bye arti sword.")
    and jump.index("extract_obj(obj, TRUE); // Bye arti sword.")
    < jump.rindex("spell_lightning_bolt(61, ch, 0, SPELL_TYPE_SPELL, ch, 0);")
))

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

print("\nAll in-flight corpse handoff checks passed successfully.")
