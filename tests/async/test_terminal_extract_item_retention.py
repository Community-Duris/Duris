"""Contracts preventing saved inventory from becoming duplicate room drops."""

from pathlib import Path

from contract_text import contains


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"
handler = (SRC / "handler.c").read_text()
limits = (SRC / "limits.c").read_text()
affects = (SRC / "affects.c").read_text()
actoth = (SRC / "actoth.c").read_text()
rooms = (SRC / "specs.room.c").read_text()
actwiz = (SRC / "actwiz.c").read_text()


def body(text, signature):
    start = text.index(signature)
    opening = text.index("{", start)
    depth = 0
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start:position + 1]
    raise AssertionError(f"unterminated function: {signature}")


saved_extract = body(handler, "void extract_char_after_terminal_save(P_char ch)")
assert saved_extract.index("CHAR_RFLAG_TERMINAL_ITEMS_SAVED") < saved_extract.index(
    "extract_char(ch)"
)

extract = body(handler, "void extract_char(P_char ch)")
assert contains(extract, "terminal_items_saved")
assert extract.count("terminal_items_saved || ch->in_room == NOWHERE") == 2
assert extract.index("terminal_items_saved || ch->in_room == NOWHERE") < extract.index(
    "obj_to_room(obj, ch->in_room)"
)

# Each non-death terminal-save flow must use the saved-item extraction path.
idle_rent = body(limits, "void point_update(void)")
assert idle_rent.index("persistence_save_character_terminal(i, RENT_LINKDEAD)") < idle_rent.index(
    "extract_char_after_terminal_save(i)"
)

camp = body(affects, "int camp(P_char ch)")
assert camp.index("persistence_save_character_terminal(ch, RENT_CAMPED)") < camp.index(
    "extract_char_after_terminal_save(ch)"
)

quit_flow = body(actoth, "void do_quit(P_char ch")
assert quit_flow.index("persistence_save_character_terminal(ch, RENT_INN)") < quit_flow.index(
    "extract_char_after_terminal_save(ch)"
)

for inn_signature in ("int inn(", "int undead_inn("):
    inn = body(rooms, inn_signature)
    assert "persistence_save_character_terminal" in inn
    assert "extract_char_after_terminal_save(ch)" in inn

ghosts = body(actwiz, "void do_extractlink(P_char ch")
assert ghosts.count("extract_char_after_terminal_save(vict)") == 2
assert "persistence_save_character_terminal(vict, RENT_LINKDEAD)" in ghosts

print("terminal saved-item extraction contracts passed")
