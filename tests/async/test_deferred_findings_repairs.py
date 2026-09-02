#!/usr/bin/env python3
"""Contracts for the findings the warning-cleanup project recorded but deferred.

Each item below was a pre-existing defect noticed while resolving a compiler
warning, and each was left alone at the time because fixing it changes behavior
rather than tidying code.  They are repaired now; these contracts keep them
repaired.  See docs/guides/BUILDING.md ("Warning profile") for the compiler policy the
cleanup established, and docs/reference/CODEBASE.md for the conventions it enforces.
"""
from _paths import SRC
from pathlib import Path

root = Path(__file__).resolve().parents[2]


def src(name):
    return (SRC / name).read_text()


actoth = src("actoth.c")
sql_player = src("sql_player.c")
sql_player_h = src("sql_player.h")
enhance = src("enhance.c")
nq = src("nq.c")
artifact = src("artifact.c")
language = src("language.c")
randobj = src("randobj.c")
randomeq = src("randomeq.c")
mining = src("mining.c")
mining_config = src("mining_config.c")
prototypes = src("prototypes.h")

failures = []


def check(name, ok):
    print(("[PASS] " if ok else "[FAIL] ") + name)
    if not ok:
        failures.append(name)


def body(text, signature, last=False):
    """The text of one function, from its signature to the closing brace."""
    start = text.rindex(signature) if last else text.index(signature)
    depth = 0
    i = text.index("{", start)
    for j in range(i, len(text)):
        if text[j] == "{":
            depth += 1
        elif text[j] == "}":
            depth -= 1
            if depth == 0:
                return text[start : j + 1]
    raise AssertionError("unterminated body for " + signature)


# 1. do_fly and do_swim tested `buf` before anything filled it.
for name, sig, parse in (
    ("do_fly", "void do_fly(P_char ch, char *argument", "argument_interpreter(argument, buf"),
    ("do_swim", "void do_swim(P_char ch, char *argument", "one_argument(argument, buf)"),
):
    text = body(actoth, sig)
    check(
        f"{name} parses its argument before testing the parsed buffer",
        text.index(parse) < text.index("if (!*buf)"),
    )

# 2. A corpse item that fails to load must not hand its affect rows to the
#    previous, unrelated object at obj_map[num_objs - 1].
# sql_player.c carries a no-DB stub of the same name first; take the real one.
corpses = body(sql_player, "bool sql_load_all_corpses(void)\n{", last=True)
check(
    "corpse affect rows are gated on the last item actually being stored",
    "if (item_id == last_item_id && last_item_stored && num_objs > 0)" in corpses,
)
check(
    "every skipped corpse item clears last_item_stored",
    corpses.count("last_item_stored = false;") >= 3
    and corpses.count("last_item_stored = true;") == 1,
)

# 3. modenhance deliberately has no enhance_material_ival_delta floor: essences
#    are purpose-built stock, priced by their own tiers.  enhance() keeps it.
check(
    "enhance() still computes and enforces the material floor",
    "minval = itemvalue(source) - enhance_material_ival_delta;" in enhance
    and "if (!pouch_material && itemvalue(material) < minval)" in enhance,
)
check(
    "modenhance() records why the floor does not apply to essences",
    "enhance_material_ival_delta floor here" in body(enhance, "void modenhance("),
)

# 4. The <class>/<race> quest children were parsed and ignored, so a quest using
#    listedclasses="allow" allowed nobody at all.
check(
    "nq resolves class and race element text",
    "static uint nq_parse_class_bit(" in nq and "static int nq_parse_race_index(" in nq,
)
quest = body(nq, "struct nq_quest *nq_parse_quest(char *fname)")
check(
    "the <class> handler honours the listedclasses allow/deny mode",
    "quest->allowed_classes |= bit;" in quest and "quest->allowed_classes &= ~bit;" in quest,
)
check(
    "the <race> handler honours the listedraces allow/deny mode",
    "quest->allowed_races[race] = races_are_allow_list ? 1 : 0;" in quest,
)

# 5. artifact.wars.modifier was read and thrown away; the penalty it scales is
#    a cut to the hoarded artifacts' remaining life.
wars = body(artifact, "void event_artifact_wars_sql(")
check(
    "artifact.wars.modifier defaults to no timer penalty when unset",
    'get_property("artifact.wars.modifier", 0.0)' in wars,
)
check(
    "the modifier scales a cut to each hoarded artifact's timer",
    "float burn = modifier * (float)punish_level;" in wars
    and "UPDATE artifacts SET timer = FROM_UNIXTIME" in wars,
)

# 6. Functions that ignored an argument their caller still supplied.
check(
    "the sql_link_player_to_account stub is gone",
    "sql_link_player_to_account" not in sql_player
    and "sql_link_player_to_account" not in sql_player_h,
)
check(
    "the inert quested_spell predicate is gone",
    "quested_spell" not in prototypes and "quested_spell" not in src("memorize.c"),
)
check(
    "language_known answers from can_understand_language",
    "can_understand_language(ch, vict)" in body(language, "const char *language_known("),
)
check(
    "the set/unique stubs that always returned NULL are gone",
    "createSetItem" not in randobj and "createUniqueItem" not in randobj,
)
check(
    "create_material maps its level-derived index onto a quality tier",
    "P_obj create_material(int index)" in randomeq
    and "MATERIAL_TIERS / (MAXMATERIAL + 1)" in randomeq,
)
check(
    "create_stones no longer takes a character it ignores",
    "P_obj create_stones(void);" in prototypes,
)
check(
    "gem mines scale with mine quality, like the sibling ore roll",
    "mining_config_gem_vnum(mine_quality)" in mining
    and "int draws = 1 + BOUNDED(0, mine_quality, 3);" in mining_config,
)

if failures:
    raise SystemExit("FAILED: " + ", ".join(failures))
print("deferred-findings repair contracts passed")
