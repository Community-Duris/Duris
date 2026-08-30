"""Source contracts for the CREATION_ALL_RACES toggle.

CREATION_ALL_RACES=TRUE opens the player races that are deliberately absent
from playable_races[] -- mostly the undead forms reached in-game with
'descend' -- to character creation, in their own labelled menu block.
"""

import re
from pathlib import Path
from contract_text import contains, find, index, split_at

ROOT = Path(__file__).resolve().parents[2]

race_table = (ROOT / "lib/creation/racetable").read_text()
for marker in (
    "&+YGOOD   RACES",
    "&+rEVIL   RACES",
    "&+LNEUTRAL   RACES",
    "&+RDuris: Land of BloodLust",
):
    assert marker in race_table


# --- the toggle itself -------------------------------------------------------
cfg = (ROOT / "src/creation_availability_config.c").read_text()
assert contains(cfg, "bool creation_all_races_enabled(void)")
assert contains(cfg, 'getenv("CREATION_ALL_RACES")')
assert contains(cfg, 'strcasecmp(value, "TRUE")')
assert "bool creation_all_races_enabled(void);" in (
    ROOT / "src/creation_availability_config.h"
).read_text()

# Documented in the committed template, and off by default there.
example = (ROOT / ".env.example").read_text()
assert "CREATION_ALL_RACES=FALSE" in example


# --- restricted_races[] must hold only non-roster player races ---------------
defines = (ROOT / "src/defines.h").read_text()
race_ids = {
    name: int(value)
    for name, value in re.findall(r"#define (RACE_[A-Z_]+)\s+(\d+)", defines)
}
player_max = race_ids[
    re.search(r"#define RACE_PLAYER_MAX\s+(RACE_[A-Z_]+)", defines).group(1)
]

constant = (ROOT / "src/constant.c").read_text()


def table_entries(source, marker):
    body = split_at(source, marker, 1)[1].split("};", 1)[0]
    return re.findall(r"RACE_[A-Z_]+", body)


roster = set(table_entries(constant, "const struct playable_race_info        playable_races[] = {"))
restricted = table_entries(
    constant, "const struct restricted_race_info        restricted_races[] = {"
)

assert restricted, "restricted_races[] is empty"
assert len(restricted) == len(set(restricted)), "duplicate entry in restricted_races[]"
for name in restricted:
    assert name in race_ids, name
    assert 0 < race_ids[name] <= player_max, f"{name} is not a player race"
    assert name not in roster, f"{name} is in both race tables"

# The two tables together must not miss a player race, so the toggle really
# does open everything a character can be.
covered = {race_ids[n] for n in roster | set(restricted)}
missing = [
    name
    for name, rid in race_ids.items()
    if 0 < rid <= player_max and rid not in covered and name != "RACE_PLAYER_MAX"
]
assert not missing, f"player races in neither table: {sorted(missing)}"


# --- telnet menu: separate, labelled block ----------------------------------
nanny = (ROOT / "src/nanny.c").read_text()
# Anchor on the definition, not the forward declaration: the signature alone
# matches both.
menu = split_at(nanny, "static void display_available_races(P_desc d)\n{", 1)[1]
menu = menu.split("\n}", 1)[0]
assert contains(menu, "creation_all_races_enabled()")
assert contains(menu, "NORMALLY UNAVAILABLE RACES")
assert contains(menu, "restricted_races[i].note")
assert contains(menu, "SEND_TO_Q(racetable, d)")
assert contains(menu, "show_formatted_table = racetable != NULL")
# A deliberately disabled standard race still uses the policy-aware fallback
# rather than advertising an option that selection will reject.
assert index(menu, "creation_race_enabled(playable_races[i].race_id)") < index(
    menu, "SEND_TO_Q(racetable, d)"
)
# Gated: the block only renders behind the toggle.
assert index(menu, "creation_all_races_enabled()") < index(menu, "NORMALLY UNAVAILABLE RACES")


# --- telnet selection: by name, and before the single-key search -------------
select = split_at(nanny, "void select_race(P_desc d, char *arg)", 1)[1]
select = split_at(select, "/* Krov: select class is next */", 1)[0]
assert contains(select, "normalize_race_token")
assert contains(select, "restricted_races[i].race_id")
# "Shade" must not be eaten by the 'S' (Minotaur help) key.
assert index(select, "creation_all_races_enabled()") < select.index(
    "/* Search playable_races[] array for matching key */"
)
# The key search only runs when no restricted race matched.
assert contains(select, "if (GET_RACE(d->character) == RACE_NONE)")


# --- websocket path stays in step -------------------------------------------
ws = (ROOT / "src/ws_handlers.c").read_text()
playable = split_at(ws, "static int ws_is_playable_race(int race)", 1)[1].split("\n}", 1)[0]
assert contains(playable, "creation_all_races_enabled()")
assert contains(playable, "ws_find_restricted_race(race)")
# Clients get a flag they can use to present these apart from the roster.
assert contains(ws, 'cJSON_AddBoolToObject(race_obj, "restricted"')
assert contains(ws, 'cJSON_AddStringToObject(race_obj, "restricted_note"')

# --- every unlocked race must offer at least one class ----------------------
# A race whose class_table[] row is entirely 5 dead-ends creation with an empty
# class menu, so such races need a restricted_class_rows[] stand-in.
def parse_rows(source, marker, name_group=2):
    body = split_at(source, marker, 1)[1].split("\n};", 1)[0]
    return body


ct_body = parse_rows(
    constant, "const int        class_table[LAST_RACE + 1][CLASS_COUNT + 1] = {"
)
ct_rows = []
# Rows are matched as brace groups, not per line: clang-format wraps a row
# across several lines once it passes the column limit.
for m in re.finditer(r"\{([-0-9,\s]*)\}\s*,\s*/\*\s*(.*?)\s*\*/", ct_body, re.S):
    ct_rows.append([int(x) for x in m.group(1).split(",")])

# class_table rows are emitted in race-id order starting at RACE_NONE (0).
assert len(ct_rows) > player_max, "class_table has fewer rows than player races"

ov_body = parse_rows(
    constant, "const struct restricted_class_row        restricted_class_rows[] = {"
)
overrides = {}
for m in re.finditer(r"\{\s*(RACE_[A-Z_]+),\s*\{([-0-9,\s]*)\}\s*\}", ov_body):
    overrides[race_ids[m.group(1)]] = [int(x) for x in m.group(2).split(",")]

# Classes switched off in the shipped config are not selectable either.
avail_cfg = (ROOT / "lib/creation_availability.cfg").read_text()
cfg_class_ids = dict(
    re.findall(r'\{"([a-z-]+)",\s*(\d+)\}', split_at(cfg, "class_names[]", 1)[1])
)
disabled_names = {
    name
    for name, value in re.findall(
        r"^creation\.class\.([a-z-]+)\.enabled=(\w+)", avail_cfg, re.M
    )
    if value.lower() == "false"
}
disabled_ids = {int(cfg_class_ids[n]) for n in disabled_names if n in cfg_class_ids}

for name in restricted:
    rid = race_ids[name]
    row = overrides.get(rid, ct_rows[rid])
    selectable = [c for c in range(1, len(row)) if row[c] != 5 and c not in disabled_ids]
    assert selectable, f"{name} offers no selectable class at creation"

# Overrides must exist only where class_table really is empty, so they never
# silently shadow the game's own data.
for rid in overrides:
    base = [c for c in range(1, len(ct_rows[rid])) if ct_rows[rid][c] != 5]
    assert not base, f"race {rid} has a class_table row; the override shadows it"


# --- class_table itself must stay untouched by the toggle -------------------
# random.mob.c walks class_table to pick a class for random mobs; the creation
# override must not leak into it.
randmob = (ROOT / "src/random.mob.c").read_text()
assert contains(randmob, "class_table[race][class_idx]")
assert not contains(randmob, "creation_class_align")

print("restricted race class-list contracts passed")
print("creation all-races toggle contracts passed")
