"""Source contracts for the kingdom module.

Every check here pins an invariant that was actually broken (or nearly
shipped broken) while the module was built, so each is a regression guard
for a defect class with a body count, not a style preference:

  * the OUTWARD lifecycle hooks were once wired to nothing, so a deleted
    guild's realm was inherited by the next guild on the reused id and
    guard NPCs kept a dangling assoc pointer;
  * the map glyph table is indexed by its enum, so an entry added to one
    but not the other silently shifts every glyph after it;
  * the command table's name array INDEX is the command number, and the
    attributes file must gain an entry per command (its own test enforces
    the latter; this one pins the index arithmetic);
  * the SQL loader reads its row positionally from ONE column string, so
    the migration's column ORDER is part of the contract;
  * the boot, shutdown and upkeep wiring each had a stretch of life as
    exported-but-never-called dead code.

Pure source checks: no server, no database.
"""

import re
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def read(rel: str) -> str:
    return (ROOT / rel).read_text(encoding="latin-1")


failures = []


def check(ok: bool, label: str, extra: str = "") -> None:
    if ok:
        print(f"OK: {label}")
    else:
        failures.append(label)
        print(f"FAIL: {label}" + (f"\n      {extra}" if extra else ""))


def test_lifecycle_hooks_are_wired() -> None:
    assocs = read("src/guild/assocs.c")
    # The deletion hook must run for EVERY deletion path, which means the
    # destructor, not one call site among several.
    d = assocs.find("Guild::~Guild")
    check(d >= 0, "Guild::~Guild found in assocs.c")
    if d >= 0:
        # the destructor body runs to the next close-brace at column 0; a
        # first draft used a fixed 2000-char regex window and missed the hook
        end = assocs.find("\n}", d)
        body = assocs[d : end if end > 0 else d + 8000]
        check(
            "kingdom_on_guild_deleted" in body,
            "Guild::~Guild calls kingdom_on_guild_deleted (realm not inherited on id reuse)",
        )
    halls = read("src/guild/guildhall_cmds.c")
    check(
        halls.count("kingdom_on_guildhall_changed(") >= 2,
        "guildhall destroy/move/re-site notify kingdom_on_guildhall_changed",
        f"call sites found: {halls.count('kingdom_on_guildhall_changed(')}",
    )


def test_glyph_tables_are_compiler_length_checked() -> None:
    # The glyph enum cannot be length-checked lexically (CONTAINS_CH carries
    # an explicit `= NUM_SECT_TYPES` initialiser; five regex drafts failed
    # before this was accepted). The compiler CAN check it, so map.c defines
    # both glyph tables UNSIZED and static_asserts their element count
    # against NUM_GLYPHS: a sized array would silently default-fill a short
    # table. This test only pins that the mechanism stays in place.
    mapc = read("src/world/map.c")
    check(
        "const AnsiString sector_symbol[] = {" in mapc,
        "sector_symbol is defined UNSIZED (so the compiler counts the entries)",
    )
    check(
        "const char *glyph_names[] = {" in mapc,
        "glyph_names is defined UNSIZED (so the compiler counts the entries)",
    )
    check(
        "sizeof(sector_symbol) / sizeof(sector_symbol[0]) == NUM_GLYPHS" in mapc,
        "static_assert pins sector_symbol's length to NUM_GLYPHS",
    )
    check(
        "sizeof(glyph_names) / sizeof(glyph_names[0]) == NUM_GLYPHS" in mapc,
        "static_assert pins glyph_names' length to NUM_GLYPHS",
    )


def test_command_table_arithmetic() -> None:
    interp_c = read("src/cmd/interp.c")
    interp_h = read("src/cmd/interp.h")
    config_h = read("src/core/config.h")
    block = re.search(r"const char \*command\[MAX_CMD\] = \{(.*?)\n\};", interp_c, re.S)
    check(block is not None, "command[] name array found")
    if block:
        names = re.findall(r'"((?:[^"\\]|\\.)+)"', block.group(1))
        check(names[-1] == "\\n", "name array ends with its sentinel")
        real = names[:-1]
        m = re.search(r"#define CMD_KINGDOM (\d+)", interp_h)
        check(m is not None, "CMD_KINGDOM is defined")
        if m:
            idx = int(m.group(1))
            # The engine numbers commands from 1: the array INDEX of a name is
            # its command number MINUS ONE (upstream ground truth: "abort" sits
            # at index 856 with CMD_ABORT 857). A first draft of this test
            # assumed index == number and wrongly flagged correct code.
            check(
                0 < idx <= len(real) and real[idx - 1] == "kingdom",
                "the name array position of 'kingdom' matches CMD_KINGDOM (index+1)",
                f"CMD_KINGDOM={idx}, name at index {idx - 1}: "
                f"{real[idx - 1] if 0 < idx <= len(real) else '<out of range>'}",
            )
        m2 = re.search(r"#define MAX_CMD (\d+)", config_h)
        check(m2 is not None, "MAX_CMD is defined")
        if m2:
            check(
                int(m2.group(1)) == len(names),
                "MAX_CMD equals the name array length including the sentinel",
                f"MAX_CMD={m2.group(1)}, array length={len(names)}",
            )
    check(
        "CMD_N(CMD_KINGDOM" in interp_c or "CMD_Y(CMD_KINGDOM" in interp_c,
        "do_kingdom is dispatched in the command table",
    )


def test_sql_columns_match_the_migration_in_order() -> None:
    db = read("src/kingdom/kingdom_db.c")
    mig = read("migrations/kingdom_realms.sql")
    m = re.search(
        r'kingdom_realm_columns\s*=\s*((?:"[^"]*"\s*)+);', db
    )
    check(m is not None, "kingdom_realm_columns single column string found")
    if m:
        cols = "".join(re.findall(r'"([^"]*)"', m.group(1))).split(",")
        # migration column order: first identifier on each column-definition line
        body = re.search(r"create table kingdom_realms \((.*?)\)\s*engine", mig, re.S | re.I)
        check(body is not None, "migration CREATE TABLE body found")
        if body:
            mig_cols = []
            for line in body.group(1).splitlines():
                line = line.strip()
                w = re.match(r"([a-z_][a-z0-9_]*)\s", line)
                if w and w.group(1) not in ("primary", "key", "unique", "constraint"):
                    mig_cols.append(w.group(1))
            check(
                cols == mig_cols,
                "the loader's column string matches the migration, IN ORDER "
                "(rows are read positionally)",
                f"loader={cols}\n      migration={mig_cols}",
            )


def test_boot_shutdown_and_upkeep_are_wired() -> None:
    comm = read("src/net/comm.c")
    events = read("src/world/new_events.c")
    check("kingdom_initialize();" in comm, "kingdom_initialize() called from the boot path")
    check("kingdom_shutdown();" in comm, "kingdom_shutdown() called from the shutdown path")
    check(
        '"kingdom-upkeep"' in events,
        "the kingdom-upkeep periodic job is registered in ne_init_events",
    )
    harvest = read("src/kingdom/kingdom.c")
    check(
        "kingdom_harvest_initialize();" in harvest,
        "kingdom_harvest_initialize() is called (was exported-and-dead once)",
    )
    check(
        "kingdom_guards_refresh_all();" in harvest,
        "kingdom_guards_refresh_all() is called (was exported-and-dead once)",
    )


def test_help_is_registered_for_both_keywords() -> None:
    cat = read("src/flatfile/flatfile_help_catalog.c")
    check(
        '"kingdoms"' in cat and '"kingdom"' in cat,
        "helpkingdoms is registered for both `help kingdoms` and `help kingdom`",
    )
    check(
        (ROOT / "lib/information/helpkingdoms").exists(),
        "lib/information/helpkingdoms exists",
    )


for _name, _fn in sorted(globals().items()):
    if _name.startswith("test_") and callable(_fn):
        _fn()

if failures:
    print(f"\n{len(failures)} kingdom source-contract check(s) failed.")
    sys.exit(1)
print("\nkingdom source contracts: OK")
