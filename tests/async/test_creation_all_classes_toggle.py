"""Source contracts for the CREATION_ALL_CLASSES character-creation override."""

import re
from pathlib import Path
from contract_text import contains, find, index


ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"

impl = (SRC / "creation_availability_config.c").read_text()
header = (SRC / "creation_availability_config.h").read_text()
nanny = (SRC / "nanny.c").read_text()
ws = (SRC / "ws_handlers.c").read_text()
skills = (SRC / "skills.c").read_text()
specs = (SRC / "specializations.c").read_text()
start = (SRC / "actwiz.c").read_text()
defines = (SRC / "defines.h").read_text()


# The committed environment turns on a strict, explicit creation-only switch.
assert contains(impl, "bool creation_all_classes_enabled(void)")
assert contains(impl, 'getenv("CREATION_ALL_CLASSES")')
assert contains(impl, 'strcasecmp(value, "TRUE")')
assert contains(header, "bool creation_all_classes_enabled(void);")
assert "CREATION_ALL_CLASSES=TRUE" in (ROOT / ".env.example").read_text()


# All 30 class IDs are represented by the availability layer.
# Braces carry inner spaces under .clang-format: { "warrior", 1 }.
class_entries = re.findall(
    r'\{\s*"[a-z-]+",\s*(\d+)\s*\}', impl.split("class_names[]", 1)[1].split("};", 1)[0]
)
assert [int(value) for value in class_entries] == list(range(1, 31))


# The override bypasses both policy-disabled classes and forbidden race/class
# cells without changing class_table[], which random mob generation still owns.
enabled = impl.split("bool creation_class_enabled(int class_id)", 1)[1].split("\n}", 1)[0]
assert contains(enabled, "creation_all_classes_enabled() || class_enabled[class_id]")
align = impl.split("int creation_class_align(int race_id, int class_id)", 1)[1].split("\n}", 1)[0]
assert contains(align, "align == 5 && creation_all_classes_enabled()")
assert contains(align, "race_id >= 0 && race_id <= LAST_RACE")
assert contains(align, "return 2;")
assert contains(header, "creation_class_normally_available")
randmob = (SRC / "random.mob.c").read_text()
assert contains(randmob, "class_table[race][class_idx]")
assert not contains(randmob, "creation_class_align")


# Telnet keeps normal choices in the normal block, clearly separates override
# choices, and accepts full names before legacy one-character keys. This is
# required because Rogue and Thief intentionally share 't'.
menu = nanny.split("void display_classtable(P_desc d)", 1)[1].split("/* Krov: ALIGN", 1)[0]
assert contains(menu, "creation_class_normally_available")
assert contains(menu, "NORMALLY UNAVAILABLE CLASSES")
assert contains(menu, "Type the full class name")
assert index(menu, "creation_all_classes_enabled()") < index(menu, "NORMALLY UNAVAILABLE CLASSES")
select = nanny.split("void select_class(P_desc d, char *arg)", 1)[1].split("void display_classtable", 1)[0]
assert contains(select, "normalize_race_token(class_names_table[cls].normal")
assert index(select, "creation_all_classes_enabled()") < select.index(
    "d->character->player.m_class == CLASS_NONE && cls <= CLASS_COUNT"
)


# WebSocket chargen uses the same gates and exposes presentation metadata for
# clients to label the normally unavailable choices as clearly as telnet does.
ws_menu = ws.split("static void ws_add_chargen_race", 1)[1].split("void ws_cmd_chargen_options", 1)[0]
assert contains(ws_menu, "creation_class_enabled(j)")
assert contains(ws_menu, "creation_class_align(race_id, j)")
assert contains(ws_menu, 'cJSON_AddBoolToObject(class_obj, "restricted"')
assert contains(ws_menu, 'cJSON_AddStringToObject(class_obj, "restricted_note"')


# A direct-created class retains its existing skill/spell metadata and can use
# its existing specialization definitions regardless of the selected race.
class_symbols = re.findall(r"#define (CLASS_[A-Z]+)\s+BIT_\d+", defines)
assert len(class_symbols) == 30
missing_skill_metadata = [name for name in class_symbols if not re.search(rf"(?:SKILL|SPELL)_ADD\({name}\b", skills)]
assert not missing_skill_metadata, missing_skill_metadata
allowed = specs.split("bool is_allowed_race_spec", 1)[1].split("\n}", 1)[0]
assert contains(allowed, "creation_all_classes_enabled()")
assert index(allowed, "creation_all_classes_enabled()") < index(allowed, "allowed_race_specs")

# Assassin and Thief are legacy standalone classes whose specialization menu
# rows were removed when Rogue specializations replaced them. Direct creation
# assigns their legacy slots before NewbySkillSet so those abilities still
# participate in leveling and skill initialization.
do_start = start.split("static void do_start_impl(P_char ch, int nomsg", 1)[1].split(
    "void do_start(P_char ch, int nomsg)", 1
)[0]
assert contains(do_start, "CLASS_ASSASSIN") and contains(do_start, "SPEC_ASSMASTER")
assert contains(do_start, "CLASS_THIEF") and contains(do_start, "SPEC_CUTPURSE")
assert index(do_start, "SPEC_ASSMASTER") < index(do_start, "NewbySkillSet")


# Every class has a deterministic starter-kit path when arbitrary race/class
# combinations are opened. Existing Human kits cover the common case; explicit
# representative fallbacks cover the classes without one.
# Tolerate the wrap clang-format puts after CREATE_KIT( on long kit lines.
human_kits = set(re.findall(r"CREATE_KIT\(\s*RACE_HUMAN,\s*(CLASS_[A-Z]+)\s*,", nanny))
fallback = nanny.split("if (!class_kit && creation_all_classes_enabled())", 1)[1].split("if (class_kit)", 1)[0]
fallback_classes = set(re.findall(r"case (CLASS_[A-Z]+):", fallback))
covered = human_kits | fallback_classes | {"CLASS_BLIGHTER"}
assert set(class_symbols) <= covered, sorted(set(class_symbols) - covered)
assert contains(nanny, "AddSpellToSpellBook")

print("creation all-classes toggle contracts passed")
