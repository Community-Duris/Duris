"""Keep cleric utility unlocks consistent with spell registration and help."""

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
source = (ROOT / "src/classes/skills.c").read_text()
circles = {}
for block in re.split(r"\n\s*SPELL_CREATE", source)[1:]:
    name = re.match(r'(?:_MSG)?\("([^"]+)"', block)
    assignment = re.search(r"^\s*SPELL_ADD\(CLASS_CLERIC, (\d+)\);", block, re.M)
    if name and assignment:
        circles[name[1]] = int(assignment[1])

assert circles["miracle"] == 7, "Clerics must receive Miracle at seventh circle"
assert circles["divine warding"] == 11
assert circles["mass purification"] == 12
assert circles["mass preserve"] == 6
for name, circle in {
    "cure light": 1,
    "cure serious": 2,
    "cure critic": 3,
    "heal": 5,
    "full heal": 7,
    "mass heal": 8,
    "accelerated healing": 10,
    "lesser resurrect": 7,
    "resurrect": 9,
}.items():
    assert circles[name] == circle, f"Existing {name} progression must stay unchanged"

def cleric_help(path):
    text = (ROOT / path).read_text().lower()
    if path == "lib/information/help_index":
        return next(entry for entry in text.split("\n#\n")
                    if entry.strip().startswith("skill_cleric ("))
    start = text.index("the following is a list of all skills and spells available to the cleric class:")
    return re.search(r"==spells==\n(.*?)(?:\n\s*\n|\n#0|\Z)",
                     text[start:], re.S)[1]


for path in ("help/duris_help.hlp", "help/duris_help_parsed.hlp",
             "lib/information/help_index"):
    help_text = cleric_help(path)
    listed = {}
    for match in re.finditer(r"^(\d+)(?:st|nd|rd|th) circle:([^\n]*(?:\n +[^\n]+)*)",
                             help_text, re.M):
        for name in match[2].replace("\n", " ").split(","):
            if name.strip():
                assert name.strip() not in listed, (path, name)
                listed[name.strip()] = int(match[1])
    for name in ("miracle", "mass preserve", "divine warding", "mass purification"):
        assert listed[name] == circles[name], (path, name)
    full_help = (ROOT / path).read_text().lower()
    assert "cast 'divine warding'" in full_help, path
    assert "cast 'mass purification'" in full_help, path


# New IDs must be unique and within the spellbook/learning iteration boundary.
header = (ROOT / "src/magic/spells.h").read_text()
ids = re.findall(r"^#define (SPELL_\w+) (\d+)$", header, re.M)
for name in ("SPELL_DIVINE_WARDING", "SPELL_MASS_PURIFICATION"):
    value = dict(ids)[name]
    assert sum(number == value for _, number in ids) == 1, name
last_spell = re.search(r"^#define LAST_SPELL (SPELL_\w+)$", header, re.M)[1]
assert int(dict(ids)[last_spell]) >= int(dict(ids)["SPELL_MASS_PURIFICATION"])
first_skill = int(re.search(r"^#define FIRST_SKILL (\d+)", header, re.M)[1])
assert int(dict(ids)[last_spell]) < first_skill
for name, spell_id, target in (
    ("divine warding", "SPELL_DIVINE_WARDING", "TAR_CHAR_ROOM"),
    ("mass purification", "SPELL_MASS_PURIFICATION", "TAR_IGNORE"),
):
    assert re.search(r'SPELL_CREATE\("' + name + r'", ' + spell_id +
                     r', PULSE_SPELLCAST \* 4,\s*' + target +
                     r',\s*spell_' + name.replace(" ", "_") + r'\);', source)

# Both help import formats must expose standalone entries for the new spells.
parsed = (ROOT / "help/duris_help_parsed.hlp").read_text()
parsed_titles = [entry.split("\n")[1].strip().split(" - Last Edited:")[0].lower()
                 for entry in parsed.split("\n#0\n") if len(entry.split("\n")) >= 2]
index = (ROOT / "lib/information/help_index").read_text()
index_titles = [entry.strip().split("\n")[0].split("(")[0].strip().strip('"').lower()
                for entry in index.split("\n#\n") if entry.strip()]
for name in ("divine warding", "mass purification"):
    assert parsed_titles.count(name) == 1, name
    assert index_titles.count(name) == 1, name

print("Cleric utility registration, progression, spell IDs, and help contracts passed.")
