from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]

new_events = (ROOT / "src/new_events.c").read_text()
handler = (ROOT / "src/handler.c").read_text()
necromancy = (ROOT / "src/necromancy.c").read_text()
affects = (ROOT / "src/affects.c").read_text()

# Equipment recalculation is player-visible and must not sit behind normal
# event work when the queue is busy. equip_char schedules this callback.
assert "event_balance_affects(P_char, P_char, P_obj, void *)" in new_events
assert "func == event_spellcast || func == event_memorize || func == event_balance_affects" in new_events
assert "add_event(event_balance_affects, 0, ch" in affects
assert "SET_BIT(ch->runtime_flags, CHAR_RFLAG_DIRTY_EQUIPMENT)" in handler

# The dracolich path must capture the corpse's remaining decay before the
# corpse is extracted, then derive the pet duration from that captured value.
prepare = necromancy.index("corpse_trace dracolich_prepare")
extract = necromancy.index("extract_obj(obj);", prepare)
created = necromancy.index("corpse_trace dracolich_created", extract)
assert prepare < extract < created
assert "timeToDecay / 2" in necromancy[created - 500:created + 500]

# Decay tracing is opt-in and limited to corpse objects.
assert 'getenv("DURIS_CORPSE_TRACE")' in necromancy
assert 'getenv("DURIS_CORPSE_TRACE")' in affects
assert "obj->type == ITEM_CORPSE" in affects

print("equipment affect priority and corpse lifecycle contracts passed")
