#!/usr/bin/env python3
"""Regression contract for restoring a player after a terminal death save."""

import re

from _paths import extract_function


capture = extract_function(
    "player_snapshot_capture.c", "bool capture_status(P_char ch"
)
materialize = extract_function(
    "player_load_materialize.c",
    "bool player_load_materialize(P_char ch, const player_load_result &result)",
)
enter_game = extract_function("nanny.c", "void enter_game(P_desc d)")
account_load = extract_function(
    "account.c", "P_char load_char_into_game(struct acct_chars *c, P_desc d)"
)
legacy_complete = extract_function(
    "nanny.c", "void nanny_player_load_complete(P_desc d, player_load_result result)"
)
die = extract_function("fight.c", "void die(P_char ch, P_char killer)")

# The terminal save must remain before the live-character 1 HP assignment: a
# failed terminal save leaves the still-live dead character in the retry flow.
assert die.index("persistence_save_character_terminal(ch, RENT_DEATH)") < die.index(
    "GET_HIT(ch) = 1;"
)

# Snapshot capture, not die(), owns the persisted death-HP rule so immediate and
# retried terminal saves serialize the same missing-HP difference.
assert "int save_intent" in capture
compact_capture = re.sub(r"\s+", " ", capture)
assert "save_intent == RENT_DEATH ? MAX(0, GET_MAX_HIT(ch) - 1)" in compact_capture
assert ": MAX(0, GET_MAX_HIT(ch) - GET_HIT(ch))" in compact_capture

# Materialization must restore Lom's legacy pre-affect convention: while max HP
# is still zero, GET_HIT temporarily carries the positive missing-HP difference.
assert "GET_HIT(ch) = hit_difference;" in materialize
assert "GET_HIT(ch) = GET_MAX_HIT(ch) - hit_difference;" not in materialize

# Account and legacy logins must both preserve the save intent for room routing,
# messaging, and the no-offline-regeneration death branch.
assert "d->rtype = loaded.snapshot.save_intent;" in account_load
assert "d->rtype = result.snapshot.save_intent;" in legacy_complete
assert "d->rtype == RENT_DEATH" in enter_game
assert "r_room = real_room(GET_BIRTHPLACE(ch));" in enter_game
assert "You rejoin the land of the living" in enter_game
assert "if (d->rtype != RENT_DEATH)" in enter_game

# Snapshot materialization must resolve the temporary difference after loading
# base stats, affects, and equipment, before enter_game uses derived maxima.
materialize_affect_total = materialize.rindex("affect_total(ch, FALSE);")
assert materialize.index("GET_HIT(ch) = hit_difference;") < materialize_affect_total
assert materialize_affect_total < materialize.rindex("return true;")
assert "const int loaded_mana = GET_MANA(ch);" in materialize
assert "const int loaded_vitality = GET_VITALITY(ch);" in materialize
assert materialize_affect_total < materialize.index(
    "GET_MANA(ch) = BOUNDED(1, loaded_mana, GET_MAX_MANA(ch));"
)
assert materialize_affect_total < materialize.index(
    "GET_VITALITY(ch) = BOUNDED(1, loaded_vitality, GET_MAX_VITALITY(ch));"
)
assert enter_game.index("GET_HIT(ch) = BOUNDED") < enter_game.index(
    "affect_total(ch, FALSE);"
)

print("death respawn state regression contract passed")
