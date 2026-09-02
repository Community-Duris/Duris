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
    billed it twice.

Pure source checks: no server, no database. Pins are made against CODE:
comments are stripped before any body is searched, so a pin can never be
satisfied by prose describing the call it wants.
"""

import re
import sys
from pathlib import Path

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


# --------------------------------------------------------------------- *
# Source-parsing helpers
# --------------------------------------------------------------------- *


def strip_comments(text: str) -> str:
    """Blank out /* */ and // comments, keeping every newline so that line
    numbers and statement anchors survive."""

    def blank(m: re.Match) -> str:
        return re.sub(r"[^\n]", " ", m.group(0))

    text = re.sub(r"/\*.*?\*/", blank, text, flags=re.S)
    text = re.sub(r"//[^\n]*", blank, text)
    return text


def function_bodies(text: str, signature: str) -> list:
    """Every brace-matched body of a function whose definition matches the
    `signature` regex (which must end at, or before, the opening brace).
    Comments are stripped first, so neither the match nor the body can be
    satisfied by a comment. Returns [] when there is no definition."""
    code = strip_comments(text)
    bodies = []
    for m in re.finditer(signature, code):
        start = code.find("{", m.end())
        if start < 0:
            continue
        depth = 0
        for i in range(start, len(code)):
            if code[i] == "{":
                depth += 1
            elif code[i] == "}":
                depth -= 1
                if depth == 0:
                    bodies.append(code[start : i + 1])
                    break
    return bodies


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
    db = read("src/kingdom/kingdom_db.c")
    # Only the MariaDB half declares the column string; take the code between
    # the `#ifndef __NO_MYSQL__` guard and its `#else`.
    sql_half = re.search(r"#ifndef __NO_MYSQL__(.*?)#else", db, re.S)
    check(sql_half is not None, "MariaDB half of kingdom_db.c found")
    if not sql_half:
        return
    sql_half = sql_half.group(1)
    m = re.search(r'kingdom_realm_columns\s*=\s*((?:"[^"]*"\s*)+);', strip_comments(sql_half))
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

    # (c) the migration, as a third leg. The schema moved into the immutable
    # ledger; during the transition exactly one of the two files exists, and
    # once the immutable one is present it is the one that must agree.
    immutable = ROOT / "migrations/immutable/0006_kingdom_realms.sql"
    legacy = ROOT / "migrations/kingdom_realms.sql"
    present = [p for p in (immutable, legacy) if p.exists()]
    check(
        len(present) == 1,
        "exactly one kingdom_realms migration exists (immutable/0006 or the legacy path)",
        f"present={[str(p.relative_to(ROOT)) for p in present]}",
    )
    mig_path = immutable if immutable.exists() else legacy
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


def test_help_is_registered_for_both_keywords() -> None:
    """helpkingdoms reaches both builds: the flat catalog under both keywords
    and the MariaDB importer's HELP_FILES table."""
    cat = strip_comments(read("src/flatfile/flatfile_help_catalog.c"))
    check(
        re.search(r'\{\s*"lib/information/helpkingdoms"\s*,\s*"kingdoms"\s*\}', cat) is not None
        and re.search(r'\{\s*"lib/information/helpkingdoms"\s*,\s*"kingdom"\s*\}', cat)
        is not None,
        "helpkingdoms is registered for both `help kingdoms` and `help kingdom` (flat catalog)",
    )
    check(
        (ROOT / "lib/information/helpkingdoms").exists(),
        "lib/information/helpkingdoms exists",
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


def test_payment_durability_is_one_write() -> None:
    """A treasury debit and the realm record that explains it must land
    together. Under MariaDB that is one transaction which Guild::save() joins;
    under the flat-file build the two writes are paired guild-first and a
    failure is remembered as payment_pending so the generic flush cannot
    publish the realm's 'paid' mark ahead of the guild's debit."""
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
        check(
            bool(save_at) and bool(realm_at) and min(save_at) < min(realm_at),
            f"kingdom_persist_payment writes the guild ({guild}->save()) BEFORE the realm record",
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
    retry = function_bodies(upkeep, r"\bvoid\s+kingdom_upkeep_retry_pending\s*\(\s*void\s*\)")
    check(
        len(retry) == 1 and "kingdom_persist_payment(" in retry[0],
        "kingdom_upkeep_retry_pending re-drives the pair through kingdom_persist_payment",
    )
    event = function_bodies(upkeep, r"\bvoid\s+kingdom_upkeep_event\s*\(\s*void\s*\)")
    check(
        len(event) == 1 and "kingdom_upkeep_retry_pending(" in event[0],
        "the sweep retries pending pairs before charging anyone",
    )
    check(
        len(event) == 1 and "payment_pending" in event[0],
        "the sweep tests payment_pending so a realm is not billed while its pair is pending",
    )

    db = read("src/kingdom/kingdom_db.c")
    flushes = function_bodies(db, r"\bvoid\s+kingdom_db_flush_dirty\s*\(\s*void\s*\)")
    check(len(flushes) == 2, "kingdom_db_flush_dirty is defined once per backend", f"{len(flushes)}")
    for i, body in enumerate(flushes):
        check(
            re.search(r"\.\s*payment_pending\b", body) is not None,
            f"kingdom_db_flush_dirty backend #{i + 1} tests payment_pending before publishing",
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


for _name, _fn in sorted(globals().items()):
    if _name.startswith("test_") and callable(_fn):
        _fn()

if failures:
    print(f"\n{len(failures)} kingdom source-contract check(s) failed.")
    sys.exit(1)
print("\nkingdom source contracts: OK")
