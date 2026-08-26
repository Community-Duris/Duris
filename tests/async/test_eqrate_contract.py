"""Source contracts for the eqrate wizard command.

eqrate was ported from Realms of Luminari, whose object model diverged from
Duris years ago.  These checks pin the parts of the port that are easy to get
wrong: the command-table wiring, the fact that the summary and the breakdown
share one scoring helper, and that every constant the port references is a
real Duris constant rather than a leftover RoL one.
"""

import re
from pathlib import Path
from contract_text import contains, find, index

ROOT = Path(__file__).resolve().parents[2]
ACTWIZ_C = (ROOT / "src/actwiz.c").read_text()
INTERP_C = (ROOT / "src/interp.c").read_text()
INTERP_H = (ROOT / "src/interp.h").read_text()
CONFIG_H = (ROOT / "src/config.h").read_text()
DEFINES_H = (ROOT / "src/defines.h").read_text()
PROTOTYPES_H = (ROOT / "src/prototypes.h").read_text()

# --------------------------------------------------------------------------
# 1. Command table wiring.  interp.c looks a command up with old_search_block,
#    which returns (index + 1), so command[CMD_X - 1] must be the keyword.
# --------------------------------------------------------------------------
table = re.search(r"const char \*command\[MAX_CMD\] = \{(.*?)\n\};", INTERP_C, re.DOTALL)
assert table is not None, "command[] table not found in src/interp.c"

entries = re.findall(r'"((?:[^"\\]|\\.)*)"', table.group(1))
assert entries[-1] == "\\n", "command[] must still be newline-terminated"

max_cmd = int(re.search(r"#define MAX_CMD\s+(\d+)", CONFIG_H).group(1))
assert max_cmd == len(entries), f"MAX_CMD is {max_cmd} but command[] holds {len(entries)} entries"

assert contains(entries, "eqrate"), "eqrate missing from command[] in src/interp.c"

cmd_eqrate = int(re.search(r"#define CMD_EQRATE\s+(\d+)", INTERP_H).group(1))
assert entries[cmd_eqrate - 1] == "eqrate", (
    f'command[CMD_EQRATE - 1] is "{entries[cmd_eqrate - 1]}", expected "eqrate"'
)

# Every command number must stay unique after the insertion.
numbers = {}
for name, value in re.findall(r"#define (CMD_[A-Z0-9_]+)\s+(\d+)", INTERP_H):
    numbers.setdefault(int(value), []).append(name)
assert numbers[cmd_eqrate] == ["CMD_EQRATE"], f"CMD_EQRATE collides with {numbers[cmd_eqrate]}"

# Immortal-only and grantable, matching the other object-database tools.
assert re.search(
    r"CMD_GRT\(CMD_EQRATE, STAT_DEAD \+ POS_PRONE, do_eqrate, IMMORTAL\);", INTERP_C
), "CMD_EQRATE is not registered as a grantable IMMORTAL command"

# do_wizhelp walks command[] directly, so eqrate needs no separate wizhelp index.
assert contains(PROTOTYPES_H, "void do_eqrate(P_char, char *, int);")
assert contains(PROTOTYPES_H, "int  rate_object(P_obj);")
assert contains(PROTOTYPES_H, "void rate_object_detailed(P_char, P_obj);")

# --------------------------------------------------------------------------
# 2. The listing score and the breakdown must not drift apart: both apply
#    paths go through eqrate_apply_score, and both flag/affect paths through
#    the same tables.
# --------------------------------------------------------------------------
for func in ("int rate_object(P_obj obj)", "void rate_object_detailed(P_char ch, P_obj obj)"):
    start = ACTWIZ_C.index(func)
    end = ACTWIZ_C.index("\n}\n", start)
    body = ACTWIZ_C[start:end]
    assert contains(body, "eqrate_apply_score("), f"{func} does not use eqrate_apply_score"
    assert contains(body, "eqrate_aff_table["), f"{func} does not use eqrate_aff_table"

# Every breakdown row goes through eqrate_row so the score column stays put.
detailed = ACTWIZ_C[index(ACTWIZ_C, "void rate_object_detailed(P_char ch, P_obj obj)"):]
detailed = detailed[: detailed.index("\n}\n")]
assert detailed.count("eqrate_row(") >= 10, "breakdown rows bypass eqrate_row"
assert re.search(r"eqrate_cat\(buf, size, len, \" &\+c%-33\.33s&N  &\+w%-33\.33s&N  &\+G%\+6d&N", ACTWIZ_C), (
    "eqrate_row no longer emits the fixed 33/33/6 column layout"
)

# --------------------------------------------------------------------------
# 3. Duris-specific remapping of the RoL original.
# --------------------------------------------------------------------------
rate_start = index(ACTWIZ_C, "int rate_object(P_obj obj)")
rate_body = ACTWIZ_C[rate_start:ACTWIZ_C.index("\n}\n", rate_start)]

# Shields are their own item type here and carry AC in value[3].
assert contains(rate_body, "obj->type == ITEM_SHIELD"), "eqrate must score ITEM_SHIELD"
assert contains(rate_body, "obj->value[3] * 5"), "shield AC comes from value[3] in Duris"
# Armor AC still lives in value[0].
assert contains(rate_body, "obj->value[0] * 5"), "armor AC comes from value[0]"
# A weapon's value[0] is the WEAPON_xxx type here, never a proc value.
assert not contains(ACTWIZ_C[rate_start:], "Weapon Proc"), "weapons have no proc value in Duris"
# MAGIC and BLESS moved to extra2_flags.
assert contains(rate_body, "ITEM2_MAGIC") and contains(rate_body, "ITEM2_BLESS")
# Special procs are func.obj or ITEM_PROCLIB; there is no spec_flag here.
assert contains(rate_body, "obj_index[obj->R_num].func.obj")
assert contains(rate_body, "ITEM_PROCLIB")

# RoL-only identifiers must not have survived the port.
eqrate_block = ACTWIZ_C[index(ACTWIZ_C, "#define EQRATE_MAX_LIST"):]
for stale in (
    "tagBogusArtifact",
    "spec_flag",
    "sets_affs",
    "IS_CSET",
    "APPLY_SAVING_PETRI",
    "APPLY_MAGIC_RESIST",
    "APPLY_CUR_HIT",
    "ITEM_NOBURN",
    "AFF_SANCTUARY",
    "AFF_TRUE_SIGHT",
    "AFF_DRAGONSCALES",
    "obj_index[obj->R_num].virtual,",
):
    assert stale not in eqrate_block, f"RoL-only identifier '{stale}' leaked into the eqrate port"

# --------------------------------------------------------------------------
# 4. Every continuous-affect row names a bit that exists, in the bank whose
#    naming prefix matches.  A row in the wrong bank would silently test the
#    wrong bitvector.
# --------------------------------------------------------------------------
defined_bits = set(re.findall(r"#define (AFF\d?_[A-Z0-9_]+)\s+BIT_\d+", DEFINES_H))
aff_table = re.search(
    r"static const struct eqrate_aff_def eqrate_aff_table\[\] = \{(.*?)\n\};",
    ACTWIZ_C,
    re.DOTALL,
)
assert aff_table is not None, "eqrate_aff_table not found"

rows = re.findall(r"\{\s*(\d+),\s*(AFF\d?_[A-Z0-9_]+),\s*\"([A-Z0-9_]+)\",\s*(-?\d+)\s*\}", aff_table.group(1))
assert len(rows) >= 50, f"eqrate_aff_table looks truncated ({len(rows)} rows parsed)"

seen = set()
for bank, bit, label, points in rows:
    assert bit in defined_bits, f"{bit} is not defined in src/defines.h"
    expected_prefix = "AFF_" if bank == "1" else f"AFF{bank}_"
    assert bit.startswith(expected_prefix), (
        f"{bit} is listed in bank {bank}, which reads obj->bitvector"
        f"{'' if bank == '1' else bank} and expects a {expected_prefix}* bit"
    )
    assert bit not in seen, f"{bit} is scored twice in eqrate_aff_table"
    seen.add(bit)
    assert int(points) != 0, f"{label} scores zero and should be dropped instead"

# The bank selector must cover exactly banks 1..5.
bank_fn = ACTWIZ_C[index(ACTWIZ_C, "static ulong eqrate_bank_bits(P_obj obj, int bank)"):]
bank_fn = bank_fn[: bank_fn.index("\n}\n")]
for bank, field in ((1, "obj->bitvector;"), (2, "obj->bitvector2;"), (3, "obj->bitvector3;"),
                    (4, "obj->bitvector4;"), (5, "obj->bitvector5;")):
    assert contains(bank_fn, f"case {bank}:") and field in bank_fn, f"bank {bank} not wired to {field}"
assert {int(b) for b, _, _, _ in rows} <= {1, 2, 3, 4, 5}

# --------------------------------------------------------------------------
# 5. Every APPLY_ case in the scorer is a real Duris apply, and the tiers the
#    RoL original defined are preserved.
# --------------------------------------------------------------------------
defined_applies = set(re.findall(r"#define (APPLY_[A-Z0-9_]+)\s+\d+", DEFINES_H))
score_fn = ACTWIZ_C[index(ACTWIZ_C, "static int eqrate_apply_score(int loc, int mod)"):]
score_fn = score_fn[: score_fn.index("\n}\n")]
for apply_name in set(re.findall(r"case (APPLY_[A-Z0-9_]+):", score_fn)):
    assert apply_name in defined_applies, f"{apply_name} is not a Duris apply"

for required, tier in (
    ("APPLY_STR", "mod * 10"),
    ("APPLY_STR_MAX", "mod * 25"),
    ("APPLY_STR_RACE", "mod * 15"),
    ("APPLY_HITROLL", "mod * 15"),
    ("APPLY_DAMROLL", "mod * 20"),
    ("APPLY_AC", "-mod * 5"),
    ("APPLY_SAVING_PARA", "-mod * 8"),
):
    assert contains(score_fn, f"case {required}:"), f"{required} is not scored"
    assert tier in score_fn, f"tier '{tier}' missing from eqrate_apply_score"

# Duris renamed RoL's petrification save and has no magic-resist apply.
assert contains(score_fn, "case APPLY_SAVING_FEAR:"), "APPLY_SAVING_FEAR replaces RoL's APPLY_SAVING_PETRI"
# Lower pulse is faster in Duris, so the pulse applies must score negated.
assert contains(score_fn, "case APPLY_COMBAT_PULSE:") and contains(score_fn, "-mod * 40")
assert contains(score_fn, "case APPLY_SPELL_PULSE:") and contains(score_fn, "-mod * 30")

# --------------------------------------------------------------------------
# 6. Argument surface matches the RoL command, plus the Duris-only wear slots.
# --------------------------------------------------------------------------
do_start = index(ACTWIZ_C, "void do_eqrate(P_char ch, char *argument, int")
do_body = ACTWIZ_C[do_start:]

for sub in ("check", "stats", "show", "detail"):
    assert contains(do_body, f'str_cmp(arg, "{sub}")'), f'eqrate subcommand "{sub}" missing'

for slot, bit in (
    ("finger", "ITEM_WEAR_FINGER"),
    ("neck", "ITEM_WEAR_NECK"),
    ("body", "ITEM_WEAR_BODY"),
    ("shield", "ITEM_WEAR_SHIELD"),
    ("wield", "ITEM_WIELD"),
    ("hold", "ITEM_HOLD"),
    ("throw", "ITEM_THROW"),
    ("quiver", "ITEM_WEAR_QUIVER"),
    ("insignia", "ITEM_GUILD_INSIGNIA"),
    # Duris-only slots that RoL never had.
    ("back", "ITEM_WEAR_BACK"),
    ("belt", "ITEM_ATTACH_BELT"),
    ("horse", "ITEM_HORSE_BODY"),
    ("nose", "ITEM_WEAR_NOSE"),
    ("horn", "ITEM_WEAR_HORN"),
    ("ioun", "ITEM_WEAR_IOUN"),
    ("spider", "ITEM_SPIDER_BODY"),
):
    assert contains(do_body, f'str_cmp(arg, "{slot}")'), f'wear position "{slot}" not accepted'
    assert bit in do_body, f"{bit} not mapped in do_eqrate"

# Listing limit is clamped, and every prototype copy is released again.
assert contains(do_body, "BOUNDED(1, atoi(arg2), EQRATE_MAX_LIST)"), "display limit is not clamped"
assert do_body.count("extract_obj(") >= 1, "prototype copies are not extracted"
assert contains(do_body, "FREE(out_buf);") and contains(do_body, "FREE(ratings);"), "eqrate leaks its buffers"

# The scan loop must extract unconditionally, or read_object leaks one object
# per prototype per invocation.
loop = do_body[index(do_body, "for (int r_num = 0; r_num <= top_of_objt; r_num++)"):]
loop = loop[: index(loop, "\n\tif (num_found > 0)")]
assert loop.count("extract_obj(obj, FALSE);") == 1
assert loop.rstrip().endswith("}"), "scan loop body ended unexpectedly"
assert index(loop, "extract_obj(obj, FALSE);") > index(loop, "rate_object(obj)"), (
    "the prototype copy must be rated before it is extracted"
)

print("eqrate source contracts: OK")
