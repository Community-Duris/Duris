#!/usr/bin/env python3
"""Contracts for pre-entry Chaos equipment preparation."""

from __future__ import annotations

import pathlib
import re


ROOT = pathlib.Path(__file__).resolve().parents[2]
NANNY = (ROOT / "src/account/nanny.c").read_text(encoding="utf-8", errors="replace")
TRANSACTION_C = (ROOT / "src/item/item_movement_transaction.c").read_text(
    encoding="utf-8", errors="replace"
)
TRANSACTION_H = (ROOT / "src/item/item_movement_transaction.h").read_text(
    encoding="utf-8", errors="replace"
)


disclaimer = NANNY.split("\tcase CON_DISCLMR:", 1)[1].split(
    "\tcase CON_ACCEPTWAIT:", 1
)[0]
enter_game = NANNY.split("void enter_game", 1)[1].split("void reconnect", 1)[0]
chaos_loader = NANNY.split("static void load_chaos_new_character_kit", 1)[1].split(
    "void load_obj_to_newbies", 1
)[0]
schedule_helper = NANNY.split("static void schedule_chaos_new_character_kit_before_entry", 1)[1].split(
    "void load_obj_to_newbies", 1
)[0]

# The grant must have an explicit, non-blocking pre-entry mode.
assert "item_creation_grant_submit_to_player_before_entry" in TRANSACTION_H
assert "allow_pre_entry" in TRANSACTION_C
assert "announce_on_completion" in TRANSACTION_C
assert "Your Chaos Equipment has been prepared!!" in TRANSACTION_C

# The active rules-agreement path must schedule after the baseline save and before
# it transitions to CON_RMOTD; the old enter_game submission must be gone.
preentry_call = "schedule_chaos_new_character_kit_before_entry(d->character)"
assert preentry_call in disclaimer
assert "writeCharacter(ch, 2, NOWHERE)" in schedule_helper
assert schedule_helper.index("writeCharacter(ch, 2, NOWHERE)") < schedule_helper.index("load_chaos_new_character_kit(ch)")
assert "load_chaos_new_character_kit(ch);" not in enter_game
assert "item_creation_grant_submit_to_player_before_entry(ch, bag, ch)" in chaos_loader
assert "item_creation_grant_mark_blocking(ch)" not in chaos_loader

# A pre-entry direct-to-player grant may validate by durable PID ownership, but
# target-container grants must not silently inherit this exception.
assert re.search(
    r"if \(!request\.allow_pre_entry && !find_player_by_pid\(request\.recipient_pid\)\)",
    TRANSACTION_C,
)
assert "request.allow_pre_entry && request.target_container_uid" in TRANSACTION_C

print("pre-entry Chaos grant contracts passed")
