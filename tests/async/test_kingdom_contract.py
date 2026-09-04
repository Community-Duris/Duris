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
    token i of that string must be the field the loader assigns from row[i]
    and the i-th value the upsert supplies -- those are the load-bearing
    pairs; the migration is cross-checked as a third leg;
  * the boot, shutdown and upkeep wiring each had a stretch of life as
    exported-but-never-called dead code;
  * a treasury debit and the realm record that explains it were once two
    unrelated writes, so a crash between them either forgave a cycle or
    billed it twice;
  * two writers then bypassed the payment_pending guard that was added to fix
    that, so a hall move or an abandon published a paid mark whose debit was
    still only in memory -- THE PENDING-WRITE RULE below pins the guard at
    every call site, and the enumeration of call sites with it;
  * dormancy was not sticky: "hall_vnum still names a map room" is not "a
    hall still stands there", so any verb that re-resolved an anchor quietly
    re-anchored a realm that should have been dormant, and with the anchor
    came its billing, its garrison and its right to claim;
  * `help kingdom` found nothing in game: lib/information/help_index carried
    no kingdom entry at all, and the flat catalog's private alias for the long
    file masked the gap from the flat build -- so the two help paths disagreed
    about a topic that existed on only one of them.

Pure source checks: no server, no database. Pins are made against CODE:
comments are stripped before any body is searched, so a pin can never be
satisfied by prose describing the call it wants.
"""

import re
import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))
from _source_contract import (  # noqa: E402
    block_start,
    enclosing_definition,
    function_bodies,
    strip_comments,
    top_level_definitions,
)

ROOT = Path(__file__).resolve().parents[2]
SRC = ROOT / "src"


def read(rel: str) -> str:
    """Text of a repo-relative file, decoded permissively."""
    return (ROOT / rel).read_text(encoding="latin-1")


failures = []


def check(ok: bool, label: str, extra: str = "") -> None:
    """Print one OK/FAIL line and record a failure for the exit status."""
    if ok:
        print(f"OK: {label}")
    else:
        failures.append(label)
        print(f"FAIL: {label}" + (f"\n      {extra}" if extra else ""))



def statement_present(text: str, call: str) -> bool:
    """True when `call` (a regex for the call expression, no trailing ';')
    begins a statement: start of line, optional whitespace, the call, then
    a ';' closing it (possibly on a later line, as long as no '{' or another
    ';' intervenes). Lines that begin with '*' or '//' are comment lines and
    never count, and the text is comment-stripped anyway."""
    code = strip_comments(text)
    pattern = re.compile(r"^[ \t]*(" + call + r")", re.M)
    for m in pattern.finditer(code):
        line_start = code.rfind("\n", 0, m.start()) + 1
        line = code[line_start : code.find("\n", m.start())].lstrip()
        if line.startswith("*") or line.startswith("//"):
            continue
        tail = code[m.end() : m.end() + 400]
        semi = tail.find(";")
        if semi < 0:
            continue
        if "{" in tail[:semi]:
            continue
        return True
    return False


# --------------------------------------------------------------------- *
# The contracts
# --------------------------------------------------------------------- *


def test_lifecycle_hooks_are_wired() -> None:
    """The guild-deleted and guildhall-changed hooks reach the module from
    every path that needs them."""
    # Comment-stripped like every sibling pin: a destructor whose only
    # mention of the hook is a comment saying it ought to call it must fail.
    assocs = strip_comments(read("src/guild/assocs.c"))
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
    halls = strip_comments(read("src/guild/guildhall_cmds.c"))
    # Exactly three anchor-changing paths exist in guildhall_cmds.c and each
    # must notify the realm: construct_main_guildhall() (a hall re-sited by
    # building anew), destroy_guildhall() and move_guildhall(). A fourth path
    # would need a fourth call AND this pin raised; a dropped call is a realm
    # anchored on a room that is no longer its hall.
    sites = len(re.findall(r"(?m)^[ \t]*kingdom_on_guildhall_changed\(", halls))
    check(
        sites == 3,
        "construct_main/destroy/move guildhall each notify kingdom_on_guildhall_changed "
        "(exactly 3 call sites)",
        f"call-site statements found: {sites}",
    )
    for fn in ("construct_main_guildhall", "destroy_guildhall", "move_guildhall"):
        bodies = function_bodies(halls, r"\bbool\s+" + fn + r"\s*\(")
        check(
            any("kingdom_on_guildhall_changed(" in b for b in bodies),
            f"{fn}() is one of the three notifying call sites",
        )


def test_glyph_tables_are_compiler_length_checked() -> None:
    """map.c keeps its two glyph tables unsized and static_asserted."""
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
    """CMD_KINGDOM, the name array index and MAX_CMD agree, and do_kingdom
    is dispatched."""
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


def _resource_columns() -> list:
    """The res_* column names in KRES enum order, read from the enum itself
    so a reordered or added resource moves the expectation with it."""
    header = strip_comments(read("src/kingdom/kingdom_internal.h"))
    enum = re.search(r"enum\s+kingdom_resource\s*\{(.*?)\}", header, re.S)
    if not enum:
        return []
    names = re.findall(r"\bKRES_([A-Z]+)\b", enum.group(1))
    return ["res_" + n.lower() for n in names if n != "MAX"]


def _loader_fields_by_row_index(sql_half: str, res_cols: list) -> dict:
    """Map each row[N] index the MariaDB loader reads to the realm field it
    assigns. Direct assignments are `realm.<field> = f(row[N])`; the
    resource loop is `realm.resources[res] = strtol(row[BASE + res], ...)`
    and expands to BASE..BASE+KRES_MAX-1 in enum order."""
    bodies = function_bodies(sql_half, r"\bbool\s+kingdom_db_load_all\s*\(\s*void\s*\)")
    if not bodies:
        return {}
    body = bodies[0]
    fields = {}
    for m in re.finditer(r"realm\.(\w+)\s*=\s*[^;]*?\brow\[(\d+)\]", body):
        fields[int(m.group(2))] = m.group(1)
    loop = re.search(r"realm\.resources\[res\]\s*=\s*[^;]*?\brow\[(\d+)\s*\+\s*res\]", body)
    if loop:
        base = int(loop.group(1))
        for offset, col in enumerate(res_cols):
            fields[base + offset] = col
    return fields


def _upsert_value_fields(sql_half: str) -> list:
    """The realm fields kingdom_db_save_realm hands the INSERT, in argument
    order, normalised to column names: realm.assoc_id -> assoc_id,
    realm.resources[KRES_WOOD] -> res_wood, static_cast<..>(realm.x) -> x."""
    bodies = function_bodies(
        sql_half, r"\bbool\s+kingdom_db_save_realm\s*\(\s*const\s+kingdom_realm\s*&\s*realm\s*\)"
    )
    if not bodies:
        return None
    body = bodies[0]
    call = re.search(r"qry\s*\(\s*((?:\"[^\"]*\"\s*)+)(.*?)\)\s*\)\s*\n", body, re.S)
    if not call:
        return None
    args = call.group(2)
    # the leading comma-separated args: kingdom_realm_columns, then values
    fields = []
    for token in re.split(r",(?![^\[\(]*[\]\)])", args):
        token = token.strip()
        if not token or token == "kingdom_realm_columns":
            continue
        m = re.search(r"realm\.resources\[KRES_([A-Z]+)\]", token)
        if m:
            fields.append("res_" + m.group(1).lower())
            continue
        m = re.search(r"realm\.(\w+)", token)
        fields.append(m.group(1) if m else token)
    return fields


def _migration_columns(mig: str) -> list:
    """Column names of the kingdom_realms CREATE TABLE, in declaration order:
    the first identifier of every column line inside the body."""
    body = re.search(
        r"create\s+table\s+(?:if\s+not\s+exists\s+)?`?kingdom_realms`?\s*\((.*?)\)\s*engine",
        mig,
        re.S | re.I,
    )
    if not body:
        return None
    cols = []
    for line in body.group(1).splitlines():
        line = line.strip().lstrip("`")
        w = re.match(r"([a-z_][a-z0-9_]*)\b", line, re.I)
        if w and w.group(1).lower() not in (
            "primary",
            "key",
            "unique",
            "constraint",
            "index",
            "foreign",
        ):
            cols.append(w.group(1).lower())
    return cols


def test_sql_columns_match_loader_upsert_and_migration() -> None:
    """Token i of kingdom_realm_columns is the field the loader reads from
    row[i] and the i-th VALUES argument of the upsert; the migration agrees."""
    # Comment-stripped BEFORE anything is parsed out of it: every regex below
    # runs on code, so a column list, a count or a VALUES format written out
    # in a comment can never stand in for the real one.
    db = strip_comments(read("src/kingdom/kingdom_db.c"))
    # Only the MariaDB half declares the column string; take the code between
    # the `#ifndef __NO_MYSQL__` guard and its `#else`.
    sql_half = re.search(r"#ifndef __NO_MYSQL__(.*?)#else", db, re.S)
    check(sql_half is not None, "MariaDB half of kingdom_db.c found")
    if not sql_half:
        return
    sql_half = sql_half.group(1)
    m = re.search(r'kingdom_realm_columns\s*=\s*((?:"[^"]*"\s*)+);', sql_half)
    check(m is not None, "kingdom_realm_columns single column string found")
    if not m:
        return
    cols = "".join(re.findall(r'"([^"]*)"', m.group(1))).split(",")
    count = re.search(r"kingdom_realm_column_count\s*=\s*(\d+)", sql_half)
    check(
        count is not None and int(count.group(1)) == len(cols),
        "kingdom_realm_column_count equals the token count of the column string",
        f"count={count.group(1) if count else None}, tokens={len(cols)}",
    )

    # (a) token i <-> the field the loader assigns from row[i]. This is the
    # pair that actually breaks: a column string reordered without moving the
    # row[] reads assigns hall_vnum to highest_claim and nobody notices until
    # a realm owns a vnum's worth of squares.
    res_cols = _resource_columns()
    check(len(res_cols) == 4, "KRES enum yields four res_* columns", f"{res_cols}")
    loader = _loader_fields_by_row_index(sql_half, res_cols)
    check(bool(loader), "the loader's row[N] reads were parsed", f"{loader}")
    if loader:
        loader_order = [loader.get(i) for i in range(len(cols))]
        check(
            sorted(loader) == list(range(len(cols))),
            "the loader reads row[0..N-1] exactly once each, N = column count",
            f"indices read={sorted(loader)}",
        )
        check(
            loader_order == cols,
            "token i of kingdom_realm_columns is the field the loader assigns from row[i]",
            f"columns={cols}\n      loader ={loader_order}",
        )

    # (b) the INSERT's VALUES arguments, in order, are the same token list.
    upsert = _upsert_value_fields(sql_half)
    check(upsert is not None, "the upsert's qry() argument list was parsed")
    if upsert is not None:
        check(
            upsert == cols,
            "kingdom_db_save_realm's VALUES arguments follow kingdom_realm_columns in order",
            f"columns={cols}\n      values ={upsert}",
        )
        fmt = re.search(r'VALUES\s*"\s*"\(([^)]*)\)', sql_half)
        specs = len(re.findall(r"%", fmt.group(1))) if fmt else -1
        check(
            specs == len(cols),
            "the VALUES format carries one conversion per column",
            f"conversions={specs}, columns={len(cols)}",
        )

    # (b2) the UPSERT half of the same statement. The three legs above all
    # walk the INSERT; a crossed assignment in the ON DUPLICATE KEY UPDATE
    # clause -- `arrears=VALUES(missed_cycles)` -- passes every one of them
    # and yet writes the wrong field on every save after the first, which is
    # every save a live realm ever gets.
    dup = re.search(r"ON DUPLICATE KEY UPDATE(.*?)\"\s*,", sql_half, re.S)
    check(dup is not None, "the upsert's ON DUPLICATE KEY UPDATE clause was found")
    if dup is not None:
        pairs = re.findall(r"(\w+)\s*=\s*VALUES\s*\(\s*(\w+)\s*\)", dup.group(1))
        crossed = [(left, right) for left, right in pairs if left != right]
        check(
            not crossed,
            "every ON DUPLICATE KEY UPDATE assignment takes its OWN column's VALUES()",
            f"crossed assignments={crossed}",
        )
        # The key itself is not re-assigned; everything else is, or an update
        # silently keeps a stale field.
        updated = [left for left, _ in pairs]
        check(
            updated == cols[1:],
            "the clause updates every column but the assoc_id key, in column order",
            f"updated  ={updated}\n      expected ={cols[1:]}",
        )

    # (c) the migration, as a third leg. The schema lives in the immutable
    # ledger and nowhere else: the transition off migrations/kingdom_realms.sql
    # is over, so that path is not a fallback, and a pin that still accepted it
    # would let the ledger entry be deleted without failing.
    mig_path = ROOT / "migrations/immutable/0006_kingdom_realms.sql"
    check(
        mig_path.exists(),
        "migrations/immutable/0006_kingdom_realms.sql is the kingdom_realms schema",
    )
    if mig_path.exists():
        mig_cols = _migration_columns(mig_path.read_text(encoding="latin-1"))
        check(mig_cols is not None, f"{mig_path.relative_to(ROOT)}: CREATE TABLE body found")
        if mig_cols is not None:
            check(
                mig_cols == cols,
                f"{mig_path.relative_to(ROOT)} declares the columns in the loader's order",
                f"loader   ={cols}\n      migration={mig_cols}",
            )


def test_boot_shutdown_and_upkeep_are_wired() -> None:
    """Boot, shutdown, copyover and the upkeep job call the module from real
    statements, not from comments."""
    comm = read("src/net/comm.c")
    events = read("src/world/new_events.c")
    harvest = read("src/kingdom/kingdom.c")
    # Statement-anchored: a comment mentioning the call does not count.
    check(
        statement_present(comm, r"kingdom_initialize\(\)"),
        "kingdom_initialize(); is a statement on the boot path",
    )
    check(
        statement_present(comm, r"kingdom_shutdown\(\)"),
        "kingdom_shutdown(); is a statement on the shutdown path",
    )
    check(
        statement_present(comm, r"kingdom_flush_persistent_state\(\)"),
        "kingdom_flush_persistent_state(); is a statement on the copyover path",
    )
    check(
        statement_present(
            events, r"nevent_register_periodic_job\(\s*\"kingdom-upkeep\"\s*,[^;]*"
        ),
        "the kingdom-upkeep periodic job is registered by a statement in new_events.c",
    )
    check(
        statement_present(harvest, r"kingdom_harvest_initialize\(\)"),
        "kingdom_harvest_initialize(); is a statement (was exported-and-dead once)",
    )
    check(
        statement_present(harvest, r"kingdom_guards_refresh_all\(\)"),
        "kingdom_guards_refresh_all(); is a statement (was exported-and-dead once)",
    )


def help_index_entries() -> list:
    """(title, body) pairs read the way BOTH production parsers read them.

    scripts/import_help_to_prod.sh (SECTION 2) and
    src/flatfile/flatfile_help_catalog.c::parse_help_index agree on the
    grammar: entries are separated by a line holding only '#', the first line
    of an entry is its title line, a leading "quoted" run is the title, and
    otherwise the title is everything before the first '('. NEITHER parser
    splits that line into keywords, so a title line naming two keywords yields
    ONE title containing both -- which is exactly why the parse is mirrored
    here instead of pattern-matched loosely against the file.
    """
    entries = []
    for block in read("lib/information/help_index").split("\n#\n"):
        block = block.strip()
        if not block or block.startswith("last update:"):
            continue
        lines = block.split("\n")
        title_line = lines[0].strip()
        quoted = re.match(r'^"([^"]+)"', title_line)
        title = quoted.group(1).strip() if quoted else title_line.split("(")[0].strip()
        body = re.sub(r"\n=+$", "", re.sub(r"^=+\n", "", "\n".join(lines[1:]).strip())).strip()
        if title and body:
            entries.append((title, body))
    return entries


def test_kingdom_help_is_two_entries_that_agree() -> None:
    """The kingdom help is two texts with one job each, and the split has to
    hold in BOTH builds:

      * lib/information/helpkingdoms -- title KINGDOMS, the long rulebook,
        authoritative, reached through the flat catalog's source table and
        through the importer's HELP_FILES table;
      * a lib/information/help_index entry -- title KINGDOM, the short summary,
        reached through the index-parsing pass of the same two loaders, whose
        body points the reader at the long one.

    Three failure modes are pinned here, all of them silent:

    1. NO INDEX ENTRY. That was the shipped bug: help_index mentioned kingdoms
       nowhere, so `help kingdom` found nothing in game.
    2. NO EXACT TITLE. Both engines answer a multi-title match by looking for a
       title equal to the search string and, failing that, printing a bare list
       of topics instead of any help (src/cmd/wikihelp.c, flat and MariaDB
       branches alike). So 'kingdom' must be a whole title on its own, not one
       keyword inside a longer one, or `help kingdom` still shows no help.
    3. A CLOBBERING TITLE. The importer writes the rulebook as page 'kingdoms'
       in SECTION 1 and then DELETE-then-INSERTs every index title in SECTION
       2, and the pages title comparison is case-insensitive -- so an index
       entry titled KINGDOMS would delete the rulebook page on the next
       import, with no error line anywhere.
    """
    cat = strip_comments(read("src/flatfile/flatfile_help_catalog.c"))
    check(
        re.search(r'\{\s*"lib/information/helpkingdoms"\s*,\s*"kingdoms"\s*\}', cat) is not None,
        "flat catalog registers lib/information/helpkingdoms as `kingdoms`",
    )
    # The singular belongs to the index entry. parse_help_index runs AFTER the
    # source table and overwrites by key, so a second registration here is both
    # dead and a lie -- and it would let the flat build answer `help kingdom`
    # from the long file while MariaDB answered it from the index entry.
    check(
        re.search(r'\{\s*"lib/information/helpkingdoms"\s*,\s*"kingdom"\s*\}', cat) is None,
        "flat catalog does NOT also claim the singular `kingdom` for the long file "
        "(that key is the help_index entry's, and the index pass overwrites it anyway)",
    )
    check(
        (ROOT / "lib/information/helpkingdoms").exists(),
        "lib/information/helpkingdoms exists",
    )
    # A reader who lands on the long file must be told the short one exists.
    opening = "\n".join(read("lib/information/helpkingdoms").splitlines()[:25]).lower()
    check(
        "help kingdom" in opening,
        "helpkingdoms' opening names `help kingdom`, the short entry, so a reader who lands "
        "on the rulebook knows the other text exists",
    )
    # The MariaDB path is fed by the importer's HELP_FILES table, not by the
    # flat catalog; a production `help kingdoms` needs this entry too.
    importer = read("scripts/import_help_to_prod.sh")
    table = re.search(r"declare\s+-A\s+HELP_FILES=\((.*?)\n\)", importer, re.S)
    check(table is not None, "importer's HELP_FILES table found")
    if table:
        entries = [
            ln.strip()
            for ln in table.group(1).splitlines()
            if ln.strip() and not ln.strip().startswith("#")
        ]
        check(
            '["helpkingdoms"]="kingdoms"' in entries,
            'scripts/import_help_to_prod.sh HELP_FILES carries ["helpkingdoms"]="kingdoms"',
            f"entries={entries}",
        )
    # SECTION 1.5's collision report is what makes a future clobber visible at
    # import time; the pins below are what make it visible at review time.
    check(
        re.search(r"^\s*PAGE_TITLES_WRITTEN\+=\(", importer, re.M) is not None
        and re.search(r'^\s*IMPORT_RESERVED_TITLES="[^"]*PAGE_TITLES_WRITTEN', importer, re.M)
        is not None,
        "the importer reports which later titles overwrite a page it just wrote from a "
        "lib/information help file (SECTION 1.5)",
    )

    index = help_index_entries()
    titles = [title for title, _ in index]
    kingdomish = [t for t in titles if "kingdom" in t.lower()]
    singular = [(t, b) for t, b in index if t.strip().lower() == "kingdom"]
    check(
        len(singular) == 1,
        "exactly one help_index entry's parsed title is `KINGDOM`, so `help kingdom` renders "
        "that entry instead of a bare list of near-misses",
        f"parsed kingdom-ish titles={kingdomish!r} (the parsers do not split keyword lists: "
        "a title line naming two keywords is one title naming both)",
    )
    check(
        not [t for t in titles if t.strip().lower() == "kingdoms"],
        "no help_index entry is titled `KINGDOMS`, which would DELETE the rulebook page "
        "imported from lib/information/helpkingdoms",
        f"parsed kingdom-ish titles={kingdomish!r}",
    )
    if singular:
        check(
            "help kingdoms" in singular[0][1].lower(),
            "the short help_index entry points at `help kingdoms`, the long rulebook, so the "
            "two texts cannot silently drift into disagreeing",
        )


def test_payment_durability_is_one_write() -> None:
    """A treasury debit and the realm record that explains it must land
    together. Under MariaDB that is one transaction which Guild::save() joins;
    under the flat-file build both catalogue after-images share one recovery
    journal. A failure is remembered as payment_pending so the generic flush
    cannot publish the realm's 'paid' mark ahead of the guild's debit."""
    sql_player = read("src/sql/sql_player.c")
    # Two definitions exist: the __NO_MYSQL__ stub and the real one. The real
    # one is the body that runs queries; pin the join pattern there.
    bodies = [
        b
        for b in function_bodies(sql_player, r"\bbool\s+sql_save_guild\s*\(\s*Guild\s*\*\s*\w+\s*\)")
        if "sql_run_query(" in b
    ]
    check(len(bodies) == 1, "the MariaDB sql_save_guild definition was found", f"{len(bodies)}")
    if bodies:
        body = bodies[0]
        check(
            re.search(r"const\s+bool\s+own_txn\s*=\s*!\s*sql_in_transaction\s*\(\s*\)", body)
            is not None,
            "sql_save_guild joins an enclosing transaction: "
            "const bool own_txn = !sql_in_transaction()",
        )
        check(
            re.search(r"own_txn\s*&&\s*!\s*sql_begin_transaction\s*\(\s*\)", body) is not None,
            "sql_save_guild opens its own transaction only when it owns one",
        )
        check(
            re.search(r"own_txn\s*&&\s*!\s*sql_commit\s*\(\s*\)", body) is not None,
            "sql_save_guild commits only the transaction it owns: own_txn && !sql_commit()",
        )
        rollbacks = re.findall(r"sql_rollback\s*\(\s*\)", body)
        guarded = re.findall(r"if\s*\(\s*own_txn\s*\)\s*\n?\s*sql_rollback\s*\(\s*\)", body)
        check(
            rollbacks and len(rollbacks) == len(guarded),
            "every sql_rollback() in sql_save_guild is guarded by own_txn "
            "(a joined transaction is the owner's to roll back)",
            f"rollbacks={len(rollbacks)}, guarded={len(guarded)}",
        )

    assocs_h = strip_comments(read("src/guild/assocs.h"))
    assocs_c = read("src/guild/assocs.c")
    guild_class = re.search(r"class\s+Guild\b.*?\n\};", assocs_h, re.S)
    check(
        guild_class is not None
        and re.search(r"\bbool\s+save\s*\(\s*(?:void)?\s*\)\s*;", guild_class.group(0))
        is not None,
        "Guild::save is declared bool in assocs.h",
    )
    save_bodies = function_bodies(assocs_c, r"\bbool\s+Guild::save\s*\(\s*(?:void)?\s*\)")
    check(len(save_bodies) == 1, "Guild::save is defined bool in assocs.c")
    if save_bodies:
        check(
            "sql_save_guild(" in save_bodies[0] and "return false" in save_bodies[0],
            "Guild::save reports sql_save_guild's failure instead of swallowing it",
        )

    internal = strip_comments(read("src/kingdom/kingdom_internal.h"))
    for decl in (
        r"\bbool\s+kingdom_persist_payment\s*\(\s*Guild\s*\*\s*\w+\s*,\s*kingdom_realm\s*&\s*\w+\s*\)\s*;",
        r"\bvoid\s+kingdom_upkeep_retry_pending\s*\(\s*void\s*\)\s*;",
        r"\bvoid\s+kingdom_upkeep_forget_guild\s*\(\s*int\s+\w+\s*\)\s*;",
        r"\bvoid\s+kingdom_upkeep_reset\s*\(\s*void\s*\)\s*;",
    ):
        check(
            re.search(decl, internal) is not None,
            "kingdom_internal.h declares " + re.search(r"kingdom_\w+", decl).group(0),
        )
    realm_struct = re.search(r"struct\s+kingdom_realm\s*\{(.*?)\n\};", internal, re.S)
    check(
        realm_struct is not None
        and re.search(r"\bbool\s+payment_pending\s*=\s*false\s*;", realm_struct.group(1))
        is not None,
        "kingdom_realm carries `bool payment_pending = false`",
    )

    upkeep = read("src/kingdom/kingdom_upkeep.c")
    persist = function_bodies(
        upkeep,
        r"\bbool\s+kingdom_persist_payment\s*\(\s*(?:P_Guild|Guild\s*\*)\s*(\w+)\s*,\s*kingdom_realm\s*&\s*(\w+)\s*\)",
    )
    check(
        len(persist) == 1,
        "kingdom_persist_payment is defined (non-static, the header's signature) in kingdom_upkeep.c",
    )
    if persist:
        body = persist[0]
        sig = re.search(
            r"\bbool\s+kingdom_persist_payment\s*\(\s*(?:P_Guild|Guild\s*\*)\s*(\w+)\s*,\s*kingdom_realm\s*&\s*(\w+)\s*\)",
            strip_comments(upkeep),
        )
        guild, realm = sig.group(1), sig.group(2)
        save_at = [m.start() for m in re.finditer(guild + r"\s*->\s*save\s*\(\s*\)", body)]
        realm_at = [m.start() for m in re.finditer(r"kingdom_db_save_realm\s*\(", body)]
        check(
            bool(save_at) and bool(realm_at),
            f"kingdom_persist_payment writes both {guild}->save() and kingdom_db_save_realm()",
            f"save()={len(save_at)}, save_realm={len(realm_at)}",
        )
        # EVERY branch, not just the first. Comparing min() to min() only
        # pinned the join branch, which happens to be spelled first, so the
        # own-transaction branch could have been reversed
        # (`ok = kingdom_db_save_realm(realm) && guild->save();`) and still
        # passed -- and that reversal is exactly the half-state this whole
        # design exists to prevent. Each realm write must be preceded by a
        # guild write inside its OWN innermost block.
        unpaired = [
            at
            for at in realm_at
            if not any(block_start(body, at) <= s < at for s in save_at)
        ]
        check(
            bool(save_at) and bool(realm_at) and not unpaired,
            f"EVERY kingdom_persist_payment branch writes the guild ({guild}->save()) "
            "BEFORE the realm record",
            f"realm writes with no preceding guild write in their block: {unpaired}",
        )
        # The MariaDB branch must refuse to write when it cannot open the
        # transaction: an unpaired write is exactly the double-bill window.
        # The refusal has to be branch-specific, because under __NO_MYSQL__
        # sql_begin_transaction() is a stub answering false and an unguarded
        # refusal would make every flat-file payment fail.
        split = re.search(r"#ifndef\s+__NO_MYSQL__(.*?)#else", body, re.S)
        check(
            split is not None,
            "kingdom_persist_payment splits the MariaDB and flat-file branches on __NO_MYSQL__",
        )
        mariadb = split.group(1) if split else ""
        flatfile = body[split.end() :] if split else ""
        check(
            "save_with_kingdom(" in flatfile,
            "the flat-file payment path journals the guild and realm through "
            "Guild::save_with_kingdom",
        )
        refuse = re.search(r"!\s*sql_begin_transaction\s*\(\s*\)", mariadb)
        check(
            refuse is not None,
            "the MariaDB branch of kingdom_persist_payment tests !sql_begin_transaction()",
        )
        # From the refusal, the next write on that path must be preceded by a
        # return: nothing may be saved on the path where no transaction opened.
        # (A join branch that writes into an ALREADY-open transaction may sit
        # before the refusal; it never begins one, so it is not this path.)
        next_write = [
            m.start()
            for m in re.finditer(guild + r"\s*->\s*save\s*\(|kingdom_db_save_realm\s*\(", mariadb)
            if refuse is not None and m.start() > refuse.end()
        ]
        check(
            refuse is not None
            and bool(next_write)
            and re.search(r"\breturn\b", mariadb[refuse.end() : min(next_write)]) is not None,
            "kingdom_persist_payment returns without writing when it cannot begin a transaction",
        )
        # A failed pair must leave the realm payment_pending, either set here
        # or by a helper in this file whose body sets it.
        setters = [
            name
            for name, helper in re.findall(
                r"\bstatic\s+\w[\w\s\*&]*?\b(\w+)\s*\([^)]*\)\s*(\{)", strip_comments(upkeep)
            )
            if any(
                re.search(r"\.\s*payment_pending\s*=\s*true|->\s*payment_pending\s*=\s*true", b)
                for b in function_bodies(upkeep, r"\bstatic\s+\w[\w\s\*&]*?\b" + name + r"\s*\(")
            )
        ]
        marks_direct = re.search(realm + r"\s*\.\s*payment_pending\s*=\s*true", body) is not None
        marks_via = bool(setters) and re.search(
            r"\b(?:" + "|".join(sorted(set(setters))) + r")\s*\(", body
        ) is not None
        check(
            marks_direct or marks_via,
            "kingdom_persist_payment marks the realm payment_pending when the pair did not land "
            "(directly, or through a helper that sets it)",
            f"setters in file={sorted(set(setters))}",
        )
        check(
            "sql_commit(" in mariadb and "sql_rollback(" in mariadb,
            "kingdom_persist_payment commits its own transaction and rolls back on failure",
        )
    db = read("src/kingdom/kingdom_db.c")
    paired = function_bodies(db, r"\bbool\s+kingdom_db_save_payment_pair\s*\(")
    check(len(paired) == 1, "the flat-file paid-pair writer is defined", f"{len(paired)}")
    if paired:
        code = strip_comments(paired[0])
        check(
            "flatfile_association_prepare_save(" in code
            and "flatfile_authority_store::metadata" in code
            and "flatfile_authority_transaction_commit_operations(" in code,
            "the guild and realm catalogue after-images share one authority transaction",
        )
    roster_bodies = function_bodies(db, r"\bbool\s+kingdom_db_save_roster\s*\(")
    mariadb_roster = next(
        (body for body in roster_bodies if "DELETE FROM kingdom_garrison" in body), ""
    )
    check(
        bool(mariadb_roster),
        "the MariaDB garrison roster writer is defined",
        f"save_roster definitions={len(roster_bodies)}",
    )
    if mariadb_roster:
        roster_code = strip_comments(mariadb_roster)
        check(
            "sql_in_transaction(" in roster_code
            and "sql_begin_transaction(" in roster_code
            and "sql_commit(" in roster_code
            and "sql_rollback(" in roster_code,
            "a direct MariaDB roster replacement owns a transaction while an enclosing "
            "payment transaction remains caller-owned",
        )
        check(
            "mysql_errno(" in roster_code
            and "ER_NO_SUCH_TABLE" in roster_code
            and "ER_NO_SUCH_TABLE_IN_ENGINE" in roster_code,
            "only the two missing-table server errors trigger rosterless degradation",
        )
        failure_helper = re.search(
            r"statement_failure\s*=\s*\[.*?\]\s*\([^)]*\)\s*->\s*bool\s*(\{.*?\n\s*\});",
            mariadb_roster,
            re.S,
        )
        failure_code = strip_comments(failure_helper.group(1)) if failure_helper else ""
        check(
            bool(failure_code)
            and re.search(r"if\s*\(\s*missing\s*\).*?return\s+true\s*;", failure_code, re.S)
            is not None
            and re.search(r"return\s+false\s*;\s*\}", failure_code, re.S) is not None,
            "a missing garrison table is accepted but every other roster statement error fails",
        )
    retry = function_bodies(upkeep, r"\bvoid\s+kingdom_upkeep_retry_pending\s*\(\s*void\s*\)")
    check(
        len(retry) == 1 and "kingdom_persist_payment(" in retry[0],
        "kingdom_upkeep_retry_pending re-drives the pair through kingdom_persist_payment",
    )
    event = function_bodies(upkeep, r"\bvoid\s+kingdom_upkeep_event\s*\(\s*void\s*\)")
    # ORDER, not presence: a retry that ran after the billing loop would let a
    # realm whose pair is still only in memory be charged a second time this
    # cycle, and the presence-only form of this pin could not tell the two
    # arrangements apart.
    retry_at = (
        [m.start() for m in re.finditer(r"kingdom_upkeep_retry_pending\s*\(", event[0])]
        if event
        else []
    )
    charge_at = (
        [m.start() for m in re.finditer(r"charge_treasury\s*\(|kingdom_upkeep_due\s*\(", event[0])]
        if event
        else []
    )
    check(
        len(event) == 1 and bool(retry_at) and bool(charge_at) and min(retry_at) < min(charge_at),
        "the sweep retries pending pairs BEFORE charging anyone (retry precedes the first "
        "charge in kingdom_upkeep_event)",
        f"retry at {retry_at}, first charge at {charge_at[:1]}",
    )
    check(
        len(event) == 1 and "payment_pending" in event[0],
        "the sweep tests payment_pending so a realm is not billed while its pair is pending",
    )

    flushes = function_bodies(db, r"\bvoid\s+kingdom_db_flush_dirty\s*\(\s*void\s*\)")
    check(len(flushes) == 2, "kingdom_db_flush_dirty is defined once per backend", f"{len(flushes)}")
    # ORDER, not presence. "payment_pending appears somewhere in the body" was
    # satisfied by a mention in the tail log line while the publishing loop
    # skipped the test entirely; what the label claims -- and what keeps a
    # paid mark from being published ahead of its guild debit -- is that the
    # test comes BEFORE every write. Each backend publishes through its own
    # call: the upsert under MariaDB, the catalogue merge under flat file.
    # The guard must sit in the publishing loop's OWN block, ahead of the
    # write: "somewhere earlier in the function" is not enough either, since
    # the flat-file body counts pending records in a first loop and would go
    # on satisfying a whole-body test after the second loop stopped checking.
    for i, body in enumerate(flushes):
        guard_at = [m.start() for m in re.finditer(r"\.\s*payment_pending\b", body)]
        write_at = [
            m.start()
            for m in re.finditer(r"kingdom_db_save_realm\s*\(|upsert_record\s*\(", body)
        ]
        unguarded = [
            at for at in write_at if not any(block_start(body, at) <= g < at for g in guard_at)
        ]
        check(
            bool(guard_at) and bool(write_at) and not unguarded,
            f"kingdom_db_flush_dirty backend #{i + 1} tests payment_pending BEFORE it publishes, "
            "in the publishing loop itself",
            f"guards at {guard_at}, writes with no guard in their block: {unguarded}",
        )
    # payment_pending is runtime-only: never a column, never an encoded field.
    columns = re.search(r'kingdom_realm_columns\s*=\s*((?:"[^"]*"\s*)+);', strip_comments(db))
    check(
        columns is not None
        and "payment_pending" not in "".join(re.findall(r'"([^"]*)"', columns.group(1))),
        "payment_pending is not a column of kingdom_realm_columns",
    )
    for fn in ("encode_catalog", "decode_catalog"):
        bodies = function_bodies(db, r"\bbool\s+" + fn + r"\s*\(")
        check(
            len(bodies) == 1 and "payment_pending" not in bodies[0],
            f"{fn} does not encode payment_pending (runtime state never persisted)",
        )

    claim = strip_comments(read("src/kingdom/kingdom_claim.c"))
    check(
        re.search(r"\bkingdom_persist_payment\s*\(", claim) is not None,
        "kingdom_claim.c persists its debits through kingdom_persist_payment",
    )
    # The module core owns the guild-deleted hook and the shutdown/copyover
    # flushes, so it is where the retry list is forgotten, drained and reset.
    core = "\n".join(
        strip_comments(p.read_text(encoding="latin-1"))
        for p in sorted((SRC / "kingdom").glob("*.c"))
        if p.name != "kingdom_upkeep.c"
    )
    for hook in (
        "kingdom_upkeep_forget_guild",
        "kingdom_upkeep_retry_pending",
        "kingdom_upkeep_reset",
    ):
        check(
            re.search(r"\b" + hook + r"\s*\(", core) is not None,
            f"{hook}() is called from the module core (not exported-and-dead)",
        )


def test_pending_write_rule_is_obeyed_by_every_call_site() -> None:
    """THE PENDING-WRITE RULE: kingdom_db_save_realm() is a primitive that does
    NOT test payment_pending, so every caller outside kingdom_db.c -- except
    kingdom_persist_payment(), which is the one function allowed to publish a
    pending record together with the guild debit that justifies it -- must be
    guarded by !payment_pending and leave a pending record dirty for
    kingdom_upkeep_retry_pending() to carry.

    Two writers once bypassed the guard entirely (the hall-change rehome and
    the abandon path), so a hall move or an abandon published a raised claim
    or a paid mark whose debit was still only in memory. The enumeration is
    part of the pin: a NEW call site fails this test even if it happens to be
    guarded, so nobody adds one without reading the rule."""
    expected_sites = {
        ("kingdom.c", "kingdom_rehome_realm"),
        ("kingdom_claim.c", "kingdom_persist_realm"),
        ("kingdom_upkeep.c", "kingdom_persist_payment"),
        ("kingdom_upkeep.c", "kingdom_upkeep_event"),
    }
    found = set()
    unguarded = []
    for path in sorted((SRC / "kingdom").glob("*.c")):
        if path.name == "kingdom_db.c":
            continue  # the primitive's own file; the rule is about its callers
        code = strip_comments(path.read_text(encoding="latin-1"))
        defs = top_level_definitions(code)
        for m in re.finditer(r"\bkingdom_db_save_realm\s*\(", code):
            owner = enclosing_definition(defs, m.start())
            name = owner[0] if owner else "<file scope>"
            found.add((path.name, name))
            if name == "kingdom_persist_payment":
                continue  # the pairing itself: a guard here would deadlock it
            body = code[owner[1] : owner[2]] if owner else ""
            if "payment_pending" not in body:
                unguarded.append(f"{path.name}:{name}")
    check(
        found == expected_sites,
        "the kingdom_db_save_realm() call sites outside kingdom_db.c are exactly the "
        "four the pending-write rule was written for",
        f"found   ={sorted(found)}\n      expected={sorted(expected_sites)}",
    )
    check(
        not unguarded,
        "every kingdom_db_save_realm() call site outside kingdom_db.c and "
        "kingdom_persist_payment() tests payment_pending",
        f"unguarded: {unguarded}",
    )


def test_node_population_is_one_random_mix_per_region() -> None:
    """A region keeps ONE population of any resource, not a quota per
    resource, and every replacement rolls its own kind.

    The first live test found the world over-filled -- 225 nodes standing at
    once -- and the mix deterministic, because each region kept a separate
    target for each of the four resources. Two regions now carry a single
    total each, and the refill picks the resource at random, so a worked-out
    node comes back as a random kind in a random room rather than as the same
    kind moved. The Tharnadia Rift is not a region at all -- and a node left
    standing outside every region by an older table is reaped, since the
    census never counts it and no sweep would otherwise ever refill its slot."""
    harvest = read("src/kingdom/kingdom_harvest.c")
    code = strip_comments(harvest)

    table = re.search(
        r"kingdom_node_regions\[\]\s*=\s*\{(.*?)\n\};", code, re.S
    )
    check(table is not None, "the node region table is found")
    if table:
        rows = re.findall(
            r'\{\s*"([^"]+)"\s*,\s*(\d+)\s*,\s*(\d+)\s*,\s*(true|false)\s*,'
            r'\s*"([^"]+)"\s*,\s*(\d+)\s*\}',
            table.group(1),
        )
        check(
            len(rows) == 2,
            "exactly two node regions, each with ONE total (not a quota per resource)",
            f"rows parsed: {rows}",
        )
        names = [r[0] for r in rows]
        check(
            not any("Tharn" in n for n in names),
            "the Tharnadia Rift is not a node region",
            f"regions: {names}",
        )
        totals = {r[0]: int(r[5]) for r in rows}
        check(
            totals.get("Surface Map") == 40 and totals.get("Underdark") == 30,
            "the surface keeps 40 nodes and the Underdark 30, of all kinds together",
            f"totals: {totals}",
        )

    refill = function_bodies(harvest, r"\bstatic\s+void\s+kingdom_nodes_reload\s*\(")
    check(len(refill) == 1, "kingdom_nodes_reload is defined once")
    if refill:
        check(
            re.search(r"kingdom_load_one_node\s*\(\s*region\s*,\s*number\s*\(", refill[0])
            is not None,
            "each replacement rolls its own resource, so the standing mix is random",
            "the refill does not pass a rolled resource to kingdom_load_one_node()",
        )

    reap = function_bodies(harvest, r"\bstatic\s+bool\s+kingdom_node_should_reap\s*\(")
    check(len(reap) == 1, "kingdom_node_should_reap is defined once")
    if reap:
        body = strip_comments(reap[0])
        check(
            re.search(r"kingdom_room_in_node_region\s*\(\s*rnum\s*\)", body) is not None,
            "a node standing outside every configured region is reaped",
            "kingdom_node_should_reap() never asks kingdom_room_in_node_region()",
        )
        helper = function_bodies(
            harvest, r"\bstatic\s+bool\s+kingdom_room_in_node_region\s*\("
        )
        check(len(helper) == 1, "kingdom_room_in_node_region is defined once")
        if helper:
            check(
                re.search(r"start_vnum\b", helper[0]) is not None
                and re.search(r"end_vnum\b", helper[0]) is not None,
                "region membership is asked of the vnum ranges, as the census asks it",
                "kingdom_room_in_node_region() does not test a region's vnum range",
            )


def test_a_room_outside_the_grid_has_no_square() -> None:
    """kingdom_square_of_room() must REFUSE a room whose offset lies past the
    end of its zone's grid, not fold it back with a modulo.

    A map zone may define more rooms than its grid holds -- the surface zone
    has 160,004 in a 400x400 grid -- and `(offset / mapx) % mapy` sends every
    one of those tail rooms to row 0, so offset 160000 reports square (0,0).
    kingdom_room_at() guards the square -> room direction by mapping back, but
    nothing catches a HALL already standing in a tail room: its square would
    read as (0,0) and the realm would be anchored over whatever genuinely
    occupies it."""
    geometry = read("src/kingdom/kingdom_geometry.c")
    bodies = function_bodies(
        geometry, r"\bbool\s+kingdom_square_of_room\s*\("
    )
    check(len(bodies) == 1, "kingdom_square_of_room is defined once", f"{len(bodies)}")
    if bodies:
        body = strip_comments(bodies[0])
        row = re.search(r"local_y\s*=\s*([^;]+);", body)
        check(row is not None, "kingdom_square_of_room derives a local row")
        if row:
            check(
                "%" not in row.group(1),
                "the row is NOT folded back into the grid with a modulo",
                f"local_y = {row.group(1).strip()}",
            )
        check(
            re.search(r"local_y\s*>=\s*\w+->mapy", body) is not None,
            "a row past the end of the grid is refused",
            "no `local_y >= zone->mapy` bound test in the body",
        )


def test_dormancy_is_sticky_and_unbilled() -> None:
    """A realm whose main hall no longer stands is DORMANT: it cannot be
    re-anchored by a verb that merely re-resolves the vnum, and it is billed
    nothing. Without the hall test, a destroyed hall left its map room
    standing and any re-resolution silently re-anchored the realm, undoing
    the not-billed / guards-despawned / claim-refused behaviour."""
    kingdom_c = read("src/kingdom/kingdom.c")
    code = strip_comments(kingdom_c)
    resolve = function_bodies(
        kingdom_c, r"\bbool\s+kingdom_resolve_anchor\s*\(\s*kingdom_realm\s*&\s*\w+\s*\)"
    )
    check(len(resolve) == 1, "kingdom_resolve_anchor is defined in kingdom.c", f"{len(resolve)}")
    if resolve:
        # Either it asks kingdom_main_hall() itself, or it asks a helper in
        # this file that does -- the seat test may be factored out, but it
        # must be ASKED, and "the vnum resolves to a map room" is not it.
        # A CALLER of kingdom_resolve_anchor is not a helper of it, so the
        # rehome path cannot lend its own kingdom_main_hall() call to this pin.
        helpers = sorted(
            {
                name
                for name, start, end in top_level_definitions(code)
                if name
                and name not in ("kingdom_main_hall", "kingdom_resolve_anchor")
                and "kingdom_main_hall(" in code[start:end]
                and "kingdom_resolve_anchor(" not in code[start:end]
            }
        )
        via_helper = bool(helpers) and (
            re.search(r"\b(?:" + "|".join(helpers) + r")\s*\(", resolve[0]) is not None
        )
        check(
            "kingdom_main_hall(" in resolve[0] or via_helper,
            "kingdom_resolve_anchor requires the association's MAIN hall to stand on "
            "hall_vnum (directly, or through a helper in kingdom.c that asks)",
            f"helpers that ask kingdom_main_hall: {helpers}",
        )

    upkeep = read("src/kingdom/kingdom_upkeep.c")
    due = function_bodies(upkeep, r"\blong\s+kingdom_upkeep_due\s*\(\s*const\s+kingdom_realm\s*&")
    check(len(due) == 1, "kingdom_upkeep_due is defined in kingdom_upkeep.c", f"{len(due)}")
    if due:
        check(
            re.search(r"hall_rnum\s*<=\s*0\s*\)\s*return\s+0\s*;", due[0]) is not None,
            "a dormant realm (anchor unresolved, hall_rnum <= 0) is billed 0",
        )
    event = function_bodies(upkeep, r"\bvoid\s+kingdom_upkeep_event\s*\(\s*void\s*\)")
    if event:
        check(
            re.search(r"hall_rnum\s*<=\s*0", event[0]) is not None,
            "the sweep itself skips a dormant realm rather than relying on the 0 due alone",
        )


def test_arrears_ladder_and_boot_grace() -> None:
    """The bottom rung reverts a ring, and a realm stripped to nothing is
    taken off the ladder -- it owes nothing and could never pay its way off.
    Boot grace is a per-realm, once-per-boot warning, not a blanket amnesty."""
    upkeep = read("src/kingdom/kingdom_upkeep.c")
    apply_bodies = function_bodies(
        upkeep, r"\bvoid\s+kingdom_apply_arrears\s*\(\s*kingdom_realm\s*&\s*\w+\s*\)"
    )
    check(len(apply_bodies) == 1, "kingdom_apply_arrears is defined", f"{len(apply_bodies)}")
    if apply_bodies:
        # Whitespace-collapsed so the pins read as the statements they pin.
        flat = re.sub(r"\s+", " ", apply_bodies[0])
        check(
            re.search(
                r"arrears == KARR_RINGS_REVERTING\s*\)\s*\(?\s*void\s*\)?\s*revert_outer_ring\s*\(",
                flat,
            )
            is not None,
            "the bottom rung (KARR_RINGS_REVERTING) is what reverts the outer ring",
        )
        check(
            re.search(r"highest_claim <= 0\s*\)\s*kingdom_clear_arrears\s*\(", flat) is not None,
            "arrears clear once the last ring has reverted (nothing held, nothing owed)",
        )

    event = function_bodies(upkeep, r"\bvoid\s+kingdom_upkeep_event\s*\(\s*void\s*\)")
    check(len(event) == 1, "kingdom_upkeep_event is defined", f"{len(event)}")
    if event:
        body = event[0]
        check(
            "boot_time" in body and "KINGDOM_UPKEEP_BOOT_GRACE_SECONDS" in body,
            "the boot-grace window is bounded and anchored on boot_time",
        )
        # The USES of the flag are the conditions it gates; its definition
        # ends in ';', a condition ends at the branch's '{'.
        uses = []
        for m in re.finditer(r"\bin_boot_grace\b", body):
            tail = body[m.end() :]
            brace, semi = tail.find("{"), tail.find(";")
            if brace >= 0 and (semi < 0 or brace < semi):
                uses.append(tail[:brace])
        blanket = [u.strip() for u in uses if not re.search(r"\brealm\s*->", u)]
        check(
            bool(uses) and not blanket,
            "boot grace is decided PER REALM, not granted to everyone in the window",
            f"gates with no per-realm term: {blanket}",
        )
        # ONCE per realm per boot. Two implementations satisfy it: the branch
        # remembers that this realm has spent its grace, or the window cannot
        # outlast one billing period at its configured MINIMUM, so a second
        # sweep inside the window is impossible. Without one of them a mud
        # running a short upkeep period grants the same realm grace on every
        # sweep of the window and the ladder never advances after a reboot.
        config = strip_comments(read("src/kingdom/kingdom_config.c"))
        window = re.search(r"#define\s+KINGDOM_UPKEEP_BOOT_GRACE_SECONDS\s+(\d+)", upkeep)
        period_min = re.search(r"#define\s+KINGDOM_UPKEEP_PERIOD_MIN\s+(\d+)", config)
        bounded = (
            window is not None
            and period_min is not None
            and int(window.group(1)) <= int(period_min.group(1))
        )
        remembered = bool(uses) and any(re.search(r"grace", u, re.I) for u in uses)
        check(
            bounded or remembered,
            "boot grace can be taken ONCE per realm per boot (the realm remembers it "
            "spent it, or the window cannot outlast the shortest configurable period)",
            f"window={window.group(1) if window else None}, "
            f"KINGDOM_UPKEEP_PERIOD_MIN={period_min.group(1) if period_min else None}",
        )
        # Grace means the ladder is NOT advanced: the branch leaves before it.
        for use in uses:
            at = body.find(use)
            opening = body.find("{", at + len(use))
            depth, close = 0, -1
            for i in range(opening, len(body)):
                if body[i] == "{":
                    depth += 1
                elif body[i] == "}":
                    depth -= 1
                    if depth == 0:
                        close = i
                        break
            branch = body[opening : close + 1] if close > 0 else ""
            check(
                "continue" in branch and "kingdom_apply_arrears" not in branch,
                "the boot-grace branch leaves without advancing the arrears ladder",
            )


def test_verbs_settle_the_world_they_changed() -> None:
    """Claim, abandon and a hall change each reconcile the garrison at once
    rather than leaving guards on ground the realm no longer holds until the
    next upkeep tick, and a claim reaps any harvest node standing on the
    square it just took (ruling 1: no nodes on realm-controlled land)."""
    claim_c = read("src/kingdom/kingdom_claim.c")
    kingdom_c = read("src/kingdom/kingdom.c")
    claim = function_bodies(claim_c, r"\bbool\s+kingdom_claim_next\s*\(")
    abandon = function_bodies(claim_c, r"\bbool\s+kingdom_abandon_last\s*\(")
    hall = function_bodies(kingdom_c, r"\bvoid\s+kingdom_on_guildhall_changed\s*\(")
    check(len(claim) == 1, "kingdom_claim_next is defined", f"{len(claim)}")
    check(len(abandon) == 1, "kingdom_abandon_last is defined", f"{len(abandon)}")
    check(len(hall) == 1, "kingdom_on_guildhall_changed is defined", f"{len(hall)}")
    # kingdom_garrison_refresh(), NOT kingdom_guards_refresh(): since the roster
    # ruling of 2026-09-04 there are two things to reconcile -- the bought guards
    # and the champion -- and the combined entry point is the one every caller
    # outside kingdom_guards.c must use. Pinning the inner one here would let a
    # caller reconcile the guards and silently leave the champion standing on
    # land its realm no longer holds.
    if claim:
        check(
            "kingdom_garrison_refresh(" in claim[0],
            "a claim refreshes the garrison (the allowance grows with the square count)",
        )
        check(
            "kingdom_node_reap_room(" in claim[0],
            "a claim reaps a harvest node standing on the square just claimed",
        )
    if abandon:
        check(
            "kingdom_garrison_refresh(" in abandon[0],
            "an abandon refreshes the garrison (no guard on ground just given up)",
        )
    if hall:
        check(
            "kingdom_garrison_refresh(" in hall[0] and "kingdom_guards_despawn(" in hall[0],
            "a hall change re-posts the garrison when re-anchored and despawns it when the "
            "realm goes dormant",
        )


def test_realm_verbs_test_membership_not_a_bare_assoc_pointer() -> None:
    """GET_ASSOC() alone is NOT membership -- Guild::apply() points an
    applicant's assoc pointer at the guild it is applying to, and a banned
    character keeps the pointer -- so every entry point uses the engine's
    three-part test. The mutating verbs go further and demand the leader
    tier, since they spend the treasury and change what upkeep costs."""
    gates = (
        ("src/kingdom/kingdom_cmds.c", r"\bvoid\s+do_kingdom\s*\(", "do_kingdom"),
        (
            "src/kingdom/kingdom_harvest.c",
            r"\bstatic\s+kingdom_realm\s*\*\s*kingdom_realm_of_char\s*\(",
            "kingdom_realm_of_char",
        ),
        (
            "src/kingdom/kingdom.c",
            r"\bbool\s+kingdom_char_owns_room\s*\(",
            "kingdom_char_owns_room",
        ),
    )
    for rel, signature, name in gates:
        bodies = function_bodies(read(rel), signature)
        check(len(bodies) == 1, f"{name} is defined in {rel}", f"{len(bodies)}")
        if bodies:
            check(
                "IS_MEMBER(" in bodies[0] and "GT_PAROLE(" in bodies[0],
                f"{name} tests IS_MEMBER and GT_PAROLE (an applicant, a banned character, "
                "an enemy and someone on parole all answer no)",
            )
    actor = function_bodies(
        read("src/kingdom/kingdom_claim.c"), r"\bstatic\s+P_Guild\s+kingdom_actor_guild\s*\("
    )
    check(len(actor) == 1, "kingdom_actor_guild is defined in kingdom_claim.c", f"{len(actor)}")
    if actor:
        check(
            "IS_MEMBER(" in actor[0] and "GT_DEPUTY(" in actor[0],
            "the mutating verbs' front door tests IS_MEMBER and the leader tier (GT_DEPUTY), "
            "which is strictly above GT_PAROLE",
        )


def test_land_is_paid_for_in_material_before_any_coin_moves() -> None:
    """Ruled 2026-09-04: a square costs coin AND all four resources.

    The ORDER is the contract, not just the charge. Nothing in this module
    refunds (ruling 6), so a claim that debited the treasury and only then
    discovered the realm was short of wood would burn the coin for nothing.
    The store must therefore be tested before kingdom_pay_from_treasury() is
    reached, and spent only after it has answered.
    """
    body = function_bodies(read("src/kingdom/kingdom_claim.c"),
                           r"\bbool\s+kingdom_claim_next\s*\(")
    check(len(body) == 1, "kingdom_claim_next is defined", f"{len(body)}")
    if not body:
        return
    code = strip_comments(body[0])
    check("kingdom_claim_material_cost(" in code,
          "a claim prices the material as well as the coin")
    check("kingdom_resource_spend(" in code, "a claim actually spends the material")
    short = code.find("realm->resources[")
    pay = code.find("kingdom_pay_from_treasury(")
    spend = code.find("kingdom_resource_spend(")
    check(-1 < short < pay,
          "the realm's stores are checked BEFORE the treasury is debited "
          "(there is no refund for a claim that fails after the coin is gone)",
          f"check at {short}, debit at {pay}")
    check(pay < spend,
          "the material is spent only after the coin was taken, so a refused "
          "payment leaves the stores untouched",
          f"debit at {pay}, spend at {spend}")


def test_material_is_scaled_off_the_coin_not_compounded_separately() -> None:
    """The material curve must TRACK the coin curve, not merely resemble it.

    kingdom_compound() truncates to a whole unit after every step. On a base of
    a million copper that is invisible; on a base of 25 it is a ~1% loss per
    step, and compounding the material directly gave 723 at square 80 where the
    curve calls for 1,180 -- 39% short, with the drift GROWING with the index,
    so the late rings the material cost exists to make hard were the ones it
    let off. The config file and both help entries quote the true curve, so the
    code was the thing that was wrong.

    Pinned structurally: the function must scale off kingdom_claim_cost() and
    must NOT call kingdom_compound() on the material base. A reviewer reading
    only the arithmetic cannot see the truncation, which is why this is a test
    and not a comment.
    """
    body = function_bodies(read("src/kingdom/kingdom_claim.c"),
                           r"\blong\s+kingdom_claim_material_cost\s*\(")
    check(len(body) == 1, "kingdom_claim_material_cost is defined", f"{len(body)}")
    if not body:
        return
    code = strip_comments(body[0])
    check("kingdom_claim_cost(" in code,
          "the material cost is derived from the coin cost, so the two curves "
          "cannot drift apart")
    check("kingdom_compound(" not in code,
          "the material cost does NOT compound from its own small base, which "
          "truncates ~1% per step and ends 39% short at square 80")
    # Multiply-then-divide: one rounding at the end rather than one per step.
    mul = code.find("base * kingdom_claim_cost(")
    check(mul >= 0,
          "the scale multiplies before it divides, so there is a single "
          "rounding rather than a compounding one")
    check("first <= 0" in code or "first == 0" in code,
          "a mud with free land (coin base 0) cannot divide by zero here")


def test_documented_claim_curve_matches_integer_compounding() -> None:
    """Every player-facing curve corner must match the shipped integer helper."""
    value = 1_000_000
    total = 0
    corners: dict[int, tuple[int, int]] = {}
    for square in range(1, 81):
        if square > 1:
            value = value * 1050 // 1000
        total += value
        if square in (1, 8, 24, 48, 80):
            corners[square] = (value // 1000, total // 1000)
    check(
        corners == {
            1: (1000, 1000),
            8: (1407, 9549),
            24: (3071, 44501),
            48: (9905, 188024),
            80: (47201, 971221),
        },
        "the expected claim-curve corners reproduce kingdom_compound's integer arithmetic",
        repr(corners),
    )
    for path in ("lib/kingdom.cfg", "lib/information/helpkingdoms"):
        compact = read(path).replace(",", "")
        for square, (price, running) in corners.items():
            check(
                re.search(rf"^\s*#?\s*{square}\s+{price}\s+p\s+{running}\s+p", compact, re.M)
                is not None,
                f"{path} documents exact integer curve corner {square}: {price}/{running}",
            )


def test_guard_promotion_ladder_reaches_every_documented_ring_cap() -> None:
    """The paid ladder is 45→50→52→54→56, one purchase per completed ring."""
    internal = strip_comments(read("src/kingdom/kingdom_internal.h"))
    for name, value in (
        ("KINGDOM_GUARD_BASE_LEVEL", 45),
        ("KINGDOM_GUARD_FIRST_TIER_LEVEL", 50),
        ("KINGDOM_GUARD_TIER_STEP", 2),
        ("KINGDOM_GUARD_TOP_LEVEL", 56),
        ("KINGDOM_GUARD_TIERS", 4),
    ):
        check(
            re.search(rf"#define\s+{name}\s+{value}\b", internal) is not None,
            f"{name} pins the documented ladder value {value}",
        )
    guards = read("src/kingdom/kingdom_guards.c")
    cap = function_bodies(guards, r"\bint\s+kingdom_guard_level_cap\s*\(")
    next_level = function_bodies(guards, r"\bint\s+kingdom_guard_next_level\s*\(")
    cost = function_bodies(guards, r"\blong\s+kingdom_guard_promotion_cost\s*\(")
    check(len(cap) == len(next_level) == len(cost) == 1,
          "guard cap, next-rung, and promotion-cost helpers are each defined once")
    if cap:
        check("KINGDOM_GUARD_FIRST_TIER_LEVEL" in cap[0] and "rings - 1" in cap[0],
              "ring one caps at 50 and later rings add two levels")
    if next_level:
        check("level < KINGDOM_GUARD_FIRST_TIER_LEVEL" in next_level[0],
              "the first promotion bridges level 45 directly to level 50")
    if cost:
        check("span * to_tier" in cost[0] and "span * from_tier" in cost[0],
              "promotion prices use cumulative tier shares so rounding remainders reach max")
    upgrade = function_bodies(read("src/kingdom/kingdom_claim.c"),
                              r"\bvoid\s+kingdom_roster_upgrade\s*\(")
    check(len(upgrade) == 1 and "kingdom_guard_next_level(level)" in upgrade[0],
          "promotion advances to the next documented realm tier")


def test_roster_load_is_complete_for_every_loaded_realm_and_rejects_bad_classes() -> None:
    """A capped realm scan must not turn durable guards into an empty roster.

    The realm cap makes load_all report failure, but the realms already admitted
    to memory still need their rows before shutdown or any later save can publish
    them. Persisted class values are untrusted too: guards take one curated class,
    while a champion takes exactly two distinct curated class bits.
    """
    db = read("src/kingdom/kingdom_db.c")
    loads = function_bodies(db, r"\bbool\s+kingdom_db_load_all\s*\(\s*void\s*\)")
    mariadb_load = next((body for body in loads if "kingdom_realm_columns" in body), "")
    check(bool(mariadb_load), "the MariaDB realm loader is defined", f"loaders={len(loads)}")
    if mariadb_load:
        code = strip_comments(mariadb_load)
        check("kingdom_db_load_rosters();" in code,
              "the MariaDB realm loader always attempts to restore loaded realms' rosters")
        check(re.search(r"if\s*\(\s*complete\s*\)\s*kingdom_db_load_rosters", code) is None,
              "a capped realm scan does not skip every loaded realm's roster")

    rosters = function_bodies(db, r"\bstatic\s+void\s+kingdom_db_load_rosters\s*\(")
    check(len(rosters) == 1, "the MariaDB roster loader is defined once", f"{len(rosters)}")
    if rosters:
        code = strip_comments(rosters[0])
        check("kingdom_db_valid_guard_class(guard_class)" in code,
              "persisted guard rows accept only one curated guard class")
        check("kingdom_db_valid_champion_class(guard_class)" in code,
              "persisted champion rows accept exactly the champion class shape")

    champion = function_bodies(db, r"\bstatic\s+bool\s+kingdom_db_valid_champion_class\s*\(")
    check(len(champion) == 1, "the persisted champion class validator is defined once",
          f"{len(champion)}")
    if champion:
        code = strip_comments(champion[0])
        check("higher & (higher - 1u)" in code
              and code.count("kingdom_db_valid_guard_class(") == 2,
              "a champion has exactly two allowed single-class bits")


def test_the_store_has_exactly_one_way_out() -> None:
    """Resources are spendable on kingdom benefits and on nothing else.

    kingdom_resource_spend() is the only outward path and it is ALL OR NOTHING:
    a realm short of one resource must pay none of them, or a refused claim
    would leave a realm poorer with nothing to show for it.
    """
    harvest = read("src/kingdom/kingdom_harvest.c")
    body = function_bodies(harvest, r"\bbool\s+kingdom_resource_spend\s*\(")
    check(len(body) == 1, "kingdom_resource_spend is defined once", f"{len(body)}")
    if not body:
        return
    code = strip_comments(body[0])
    # Every shortfall must return before any counter moves: the decrement loop
    # is the LAST thing in the function.
    check(code.find("return false") < code.find("realm.resources[res] -= costs[res]"),
          "every refusal returns before a single counter is decremented")
    check("realm.dirty = true" in code,
          "spending marks the realm dirty so the change is published")


def test_guards_patrol_within_the_border_and_are_never_sentinel() -> None:
    """Ruled 2026-09-04: the garrison walks the ground it is paid to hold.

    ACT_SENTINEL STAYS SET, and that is not a contradiction. MobCanGo() refuses
    a sentinel mob every direction, so the ENGINE's wander -- which knows about
    zones and nothing about realms -- cannot walk a guard off the territory;
    do_move() does not consult the flag at all, so this module's own patrol
    moves it perfectly well. Containment is therefore ours to enforce, and the
    one rule is that a step's DESTINATION must belong to the guard's own realm.
    Asking kingdom_owner_of_room() of the destination is what stops a guard
    being lured across a border, and what walks it back in when a ring reverts
    under its feet.
    """
    guards = read("src/kingdom/kingdom_guards.c")
    spawn = function_bodies(guards, r"\bstatic\s+bool\s+kingdom_guard_spawn_one\s*\(")
    check(len(spawn) == 1, "kingdom_guard_spawn_one is defined", f"{len(spawn)}")
    if spawn:
        code = strip_comments(spawn[0])
        check("SET_BIT(mob->specials.act, ACT_SENTINEL)" in code,
              "a spawned guard keeps ACT_SENTINEL, so the engine's own wander "
              "cannot move it off the realm")
        check("REMOVE_BIT(mob->specials.act, ACT_SENTINEL)" not in code,
              "nothing in the spawner clears ACT_SENTINEL, which would hand the "
              "engine back the ability to move a guard")
    enter = function_bodies(guards, r"\bstatic\s+bool\s+kingdom_guard_may_enter\s*\(")
    check(len(enter) == 1, "kingdom_guard_may_enter is defined", f"{len(enter)}")
    if enter:
        check("kingdom_owner_of_room(to_room)" in strip_comments(enter[0]),
              "a patrol step is judged on the DESTINATION's owner, so a guard "
              "cannot be lured off its realm")


def test_the_standing_loadout_is_applied_in_code_not_left_to_world_data() -> None:
    """The garrison's competence is a rule of the module, not of one .mob file.

    Every flag ruled on 2026-09-04 is set by kingdom_guard_outfit() rather than
    written into areas/mob/heavens.mob, so a stray area edit cannot quietly
    produce a guard that anyone can walk past invisible.
    """
    body = function_bodies(read("src/kingdom/kingdom_guards.c"),
                           r"\bstatic\s+void\s+kingdom_guard_outfit\s*\(")
    check(len(body) == 1, "kingdom_guard_outfit is defined", f"{len(body)}")
    if not body:
        return
    code = strip_comments(body[0])
    for flag in ("AFF_DETECT_INVISIBLE", "AFF_SENSE_LIFE", "AFF_INFRAVISION",
                 "AFF2_ULTRAVISION", "AFF_FARSEE", "AFF_AWARE", "AFF_HASTE",
                 "AFF_STONE_SKIN", "AFF2_GLOBE", "AFF4_NOFEAR", "AFF_FLY",
                 "AFF3_SWIMMING", "ACT_NO_SUMMON", "ACT_BREAK_CHARM",
                 "ACT_IMMUNE_TO_PARA", "ACT_ELITE", "ACT_SCAVENGER",
                 "ACT_MEMORY", "ACT_CANFLY", "ACT_CANSWIM", "ACT_PROTECTOR"):
        check(flag in code, f"every guard is given {flag}")
    check("AGGR_EVIL_RACE" in code and "AGGR_GOOD_RACE" in code,
          "a guard is aggressive to the OPPOSING racewar side, both ways round")


def test_the_champion_needs_the_whole_map_and_two_callings() -> None:
    """One champion, only for a realm holding all eighty squares.

    Both gates are pinned because either one alone would be a different
    feature: without the square test a young realm could buy a level-60 mob,
    and without the two-class test the champion would be an ordinary guard at
    five times the price.
    """
    body = function_bodies(read("src/kingdom/kingdom_claim.c"),
                           r"\bvoid\s+kingdom_roster_champion\s*\(")
    check(len(body) == 1, "kingdom_roster_champion is defined", f"{len(body)}")
    if not body:
        return
    code = strip_comments(body[0])
    check("highest_claim < KINGDOM_MAX_SQUARES" in code,
          "a champion is refused to a realm that does not hold every square")
    check("class_one == class_two" in code,
          "a champion's two callings must differ")
    check("realm->champion_class" in code, "the champion is recorded on the realm")
    check("kingdom_champion_refresh(" in code,
          "raising a champion puts it in the world through the reconciler")


def test_garrison_identity_and_hunts_survive_extraction() -> None:
    """Garrison scans never dereference a PC, a malformed NPC, or an extracted hunter."""
    guards = read("src/kingdom/kingdom_guards.c")
    champion = function_bodies(guards, r"\bstatic\s+bool\s+kingdom_char_is_champion\s*\(")
    check(len(champion) == 1, "the shared champion identity predicate is defined once",
          f"{len(champion)}")
    if champion:
        code = strip_comments(champion[0])
        check("IS_NPC(ch)" in code and "ch->only.npc == NULL" in code
              and "ch->player.name == NULL" in code and "*ch->player.name == '\\0'" in code,
              "champion identity rejects PCs and malformed NPC records before R_num")
    check(strip_comments(guards).count("->R_num == champion_rnum") == 1,
          "all champion refresh and despawn scans use the shared safe identity predicate")

    champion_rnum = function_bodies(guards, r"\bstatic\s+int\s+kingdom_champion_rnum\s*\(")
    check(len(champion_rnum) == 1, "kingdom_champion_rnum is defined once",
          f"{len(champion_rnum)}")
    if champion_rnum:
        code = strip_comments(champion_rnum[0])
        check(-1 < code.find("mob_index == NULL") < code.find("real_mobile("),
              "champion prototype lookup waits until mob_index exists")

    hurt = function_bodies(guards, r"\bstatic\s+bool\s+kingdom_garrison_is_hurt\s*\(")
    check(len(hurt) == 1, "kingdom_garrison_is_hurt is defined once", f"{len(hurt)}")
    if hurt:
        check("kingdom_char_is_garrison_of(" in strip_comments(hurt[0]),
              "sanctity can be selected for a hurt champion even with no bought guards")

    muster = function_bodies(guards, r"\bstatic\s+void\s+kingdom_guard_call_the_garrison\s*\(")
    check(len(muster) == 1, "kingdom_guard_call_the_garrison is defined once",
          f"{len(muster)}")
    if muster:
        code = strip_comments(muster[0])
        snapshot = code.find("std::vector<P_char> responders")
        validate = code.find("char_in_list(mob)", snapshot)
        hunt = code.find("MobHuntCheck(mob, attacker)", validate)
        check(-1 < snapshot < validate < hunt and "IS_ALIVE(mob)" in code[validate:hunt],
              "the muster snapshots responders and revalidates each live one before movement")

    move = function_bodies(read("src/cmd/actmove.c"), r"\bvoid\s+do_move\s*\(")
    check(len(move) == 1, "do_move is defined once", f"{len(move)}")
    if move:
        code = strip_comments(move[0])
        moved = code.find("do_simple_move(ch")
        live = code.find("char_in_list(ch)", moved)
        after = code.find("affected_by_spell(ch, SPELL_PATH_OF_FROST)", moved)
        check(-1 < moved < live < after,
              "do_move revalidates a character after a destination proc before dereferencing it")

    hunt_check = function_bodies(read("src/mob/mobact.c"), r"\bvoid\s+MobHuntCheck\s*\(")
    check(len(hunt_check) == 1, "MobHuntCheck is defined once", f"{len(hunt_check)}")
    if hunt_check:
        code = strip_comments(hunt_check[0])
        moved = code.find("do_move(ch, NULL, a)")
        live = code.find("char_in_list(ch)", moved)
        wait = code.find("CharWait(ch", moved)
        check(-1 < moved < live < wait,
              "MobHuntCheck revalidates an extracted hunter before waiting or inspecting it")


def test_the_banner_ticks_and_can_be_broken() -> None:
    """A banner is a thing in a room, not an aura.

    The whole design rests on it being destructible: an attacking party must
    be able to spend rounds on the banner instead of on the garrison. So the
    proc must arm its own tick (CMD_SET_PERIODIC -> TRUE), buff on CMD_PERIODIC,
    and take damage from CMD_KILL / CMD_HIT.
    """
    guards = read("src/kingdom/kingdom_guards.c")
    body = function_bodies(guards, r"\bint\s+kingdom_banner_proc\s*\(")
    check(len(body) == 1, "kingdom_banner_proc is defined", f"{len(body)}")
    if body:
        code = strip_comments(body[0])
        check("CMD_SET_PERIODIC" in code and "CMD_PERIODIC" in code,
              "the banner arms and answers its own periodic tick")
        check("CMD_KILL" in code and "CMD_HIT" in code,
              "the banner can be attacked")
        check("extract_obj(" in code, "a banner destroyed leaves the world")
        check("REMOVE_BIT" in code and "AFF_INFERNAL_FURY" in code,
              "breaking the banner takes its buff away at once, so a banner "
              "that fell cannot go on helping")
        check(code.count("kingdom_char_is_garrison_of(") >= 2,
              "the banner grants and removes fury with the same guard-and-champion predicate")
        check(".virtual_number" in code and ".number" not in code,
              "banner identity uses the prototype virtual_number, not the live instance count")
    plant = function_bodies(guards, r"\bstatic\s+void\s+kingdom_banner_plant\s*\(")
    check(len(plant) == 1, "kingdom_banner_plant is defined", f"{len(plant)}")
    if plant:
        code = strip_comments(plant[0])
        check("const std::string realm" in code and "realm.c_str()" in code,
              "the realm name string outlives every c_str() use while a banner is named")
    bind = function_bodies(guards, r"\bvoid\s+kingdom_guards_bind_proc\s*\(")
    check(len(bind) == 1, "kingdom_guards_bind_proc is defined", f"{len(bind)}")
    if bind:
        code = strip_comments(bind[0])
        check("kingdom_guard_proc" in code and "kingdom_champion_proc" in code
              and "kingdom_banner_proc" in code,
              "all three procs are bound, or the feature is inert world data")
        check("real_object(" in code and "real_object0(" not in code,
              "banner prototypes resolve with real_object(), never real_object0() "
              "-- which answers 0 both for the first object and for a missing vnum")


def test_the_spec_heartbeat_is_armed_at_both_ends() -> None:
    """The patrol runs on the spec heartbeat, and arming it takes TWO things.

    read_mobile() arms event_mob_proc only for a mob carrying ACT_SPEC whose
    proc then answers TRUE to CMD_SET_PERIODIC (world/db.c). Miss either half
    and the guards silently stand exactly where they were posted -- the
    pre-2026-09-04 behaviour, with no error anywhere to say so.

    ACT_SPEC IS DERIVED, NEVER PERSISTED: read_mobile() sets it for any
    prototype with a bound function and strips it from any without one, and
    test_boot_log_hygiene.py refuses the bit in area sources. So the world-data
    half of the pair is not a flag in heavens.mob -- it is the BINDING, which
    must happen at initialize() before any guard can be spawned. That is what
    is pinned here.

    It also pins CMD_PERIODIC as the command the patrol listens for: the
    CMD_MOB_MUNDANE dispatch to spec procs in mobact.c is commented out, so a
    patrol written against that command would never once run.
    """
    guards = read("src/kingdom/kingdom_guards.c")
    for name in ("kingdom_guard_proc", "kingdom_champion_proc"):
        bodies = function_bodies(guards, r"\bint\s+" + name + r"\s*\(")
        check(len(bodies) == 1, f"{name} is defined", f"{len(bodies)}")
        if not bodies:
            continue
        code = strip_comments(bodies[0])
        arm = code.find("CMD_SET_PERIODIC")
        check(arm >= 0 and "return TRUE" in code[arm:arm + 120],
              f"{name} answers TRUE to CMD_SET_PERIODIC, which is what arms its "
              "heartbeat")
        check("CMD_PERIODIC" in code.replace("CMD_SET_PERIODIC", ""),
              f"{name} acts on CMD_PERIODIC, the live heartbeat -- not on "
              "CMD_MOB_MUNDANE, whose dispatch to spec procs is commented out")
    guard_proc = function_bodies(guards, r"\bint\s+kingdom_guard_proc\s*\(")
    check(bool(guard_proc) and "CMD_KILL" not in strip_comments(guard_proc[0]),
          "a guard reacts to CMD_GOTHIT alone: special() offers every command "
          "typed in a room to every mob in it, so reacting to CMD_KILL would "
          "muster the garrison at someone killing a rat nearby")

    # The binding half. read_mobile() derives ACT_SPEC from a bound function on
    # every spawn, so the contract is that the binding happens at initialize --
    # before a guard can be read_mobile()'d -- and not that a flag sits in the
    # area file, which test_boot_log_hygiene.py forbids.
    boot = function_bodies(read("src/kingdom/kingdom.c"),
                           r"\bvoid\s+kingdom_initialize\s*\(")
    check(len(boot) == 1, "kingdom_initialize is defined", f"{len(boot)}")
    if boot:
        code = strip_comments(boot[0])
        bind = code.find("kingdom_guards_bind_proc(")
        spawn = code.find("kingdom_guards_refresh_all(")
        check(bind >= 0, "the procs are bound at initialize, or every guard "
                         "spawns without ACT_SPEC and never patrols")
        check(-1 < bind < spawn,
              "the binding happens BEFORE the first refresh, because "
              "read_mobile() derives ACT_SPEC from the bound function at the "
              "moment of the spawn",
              f"bind at {bind}, refresh at {spawn}")

    # And the bit must NOT be persisted in the area source: it is derived, and
    # test_boot_log_hygiene.py fails the whole build if an area file carries it.
    mob = read("areas/mob/heavens.mob")
    for vnum in ("#108", "#109"):
        block = mob.split(vnum + "\n", 1)
        check(len(block) == 2, f"heavens.mob carries {vnum}")
        if len(block) != 2:
            continue
        stat_line = next((line for line in block[1].splitlines()
                          if line.strip().endswith(" S") and line[:1].isdigit()), "")
        flags = int(stat_line.split()[0]) if stat_line else 0
        check(not flags & 1,
              f"heavens.mob {vnum} does NOT persist the derived ACT_SPEC bit",
              f"act flags {flags}")


def test_prospect_is_open_to_everyone() -> None:
    """`kingdom prospect` must dispatch AHEAD of the guild membership gate.

    It is the verb someone uses to decide whether founding a guild is worth
    it, so demanding the guild would make it useless exactly when it is
    wanted. Pinned by position: the dispatch must come before GET_ASSOC() is
    read for the membership test.
    """
    body = function_bodies(read("src/kingdom/kingdom_cmds.c"),
                           r"\bvoid\s+do_kingdom\s*\(")
    check(len(body) == 1, "do_kingdom is defined", f"{len(body)}")
    if not body:
        return
    code = strip_comments(body[0])
    prospect = code.find("kingdom_prospect(")
    gate = code.find("P_Guild guild = GET_ASSOC(ch)")
    check(-1 < prospect < gate,
          "prospect dispatches before the guild gate, so it needs no guild",
          f"prospect at {prospect}, gate at {gate}")
    for prefix in ("p", "pr", "pro"):
        check(f'str_cmp(token, "{prefix}")' in code,
              f"shared prefix '{prefix}' is refused as ambiguous rather than silently "
              "becoming prospect")
    placement = function_bodies(read("src/kingdom/kingdom_placement.c"),
                                r"\bvoid\s+kingdom_prospect\s*\(")
    check(len(placement) == 1, "kingdom_prospect is defined", f"{len(placement)}")
    if placement:
        prospect_code = strip_comments(placement[0])
        check("guildhall_valid_map_seat(ch->in_room)" in prospect_code
              and "guildhall_valid_map_seat(rnum)" in prospect_code,
              "both the current and suggested seats use the guildhall terrain rule")
        check("here_x + dx" in prospect_code and "here_y + dy" in prospect_code,
              "each suggested seat reports exact map coordinates as well as its bearing")
        check("KINGDOM_PROSPECT_FOOTPRINT_BUDGET" in prospect_code
              and prospect_code.count("footprints_left > 0") >= 3,
              "every prospect spiral level stops when its per-command footprint budget is spent")
        cheap = prospect_code.find("guildhall_valid_map_seat(rnum)")
        candidate = prospect_code.find("kingdom_judge_footprint(rnum")
        decrement = prospect_code.find("footprints_left--", cheap, candidate)
        check(-1 < cheap < decrement < candidate,
              "a candidate consumes budget before its expensive footprint is judged")
        check("budget_exhausted" in prospect_code,
              "a truncated prospect reports its work limit instead of claiming the radius is empty")
    hall_check = function_bodies(read("src/guild/guildhall_cmds.c"),
                                 r"\bbool\s+guildhall_map_check\s*\(")
    check(len(hall_check) == 1 and "guildhall_valid_map_seat(rroom)" in hall_check[0],
          "guildhall placement and prospecting share one terrain predicate")


for _name, _fn in sorted(globals().items()):
    if _name.startswith("test_") and callable(_fn):
        _fn()

if failures:
    print(f"\n{len(failures)} kingdom source-contract check(s) failed.")
    sys.exit(1)
print("\nkingdom source contracts: OK")
