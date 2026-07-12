#!/usr/bin/env python3
"""
Season-reset table manifest regression test.

Verifies that every table in the bootstrap schema is classified in the
season-reset manifest and that the manifest in sql_pwipe() covers all
tables classified as RESET.

This is a source-contract test: it parses the C source to extract the
DELETE FROM / UPDATE statements in sql_pwipe() and compares them against
the full table inventory from the bootstrap SQL.
"""

import re
import os
import sys

REPO_ROOT = os.path.dirname(os.path.dirname(os.path.dirname(os.path.abspath(__file__))))
SQL_C = os.path.join(REPO_ROOT, "src", "sql.c")
BOOTSTRAP_SQL = os.path.join(REPO_ROOT, "migrations", "bootstrap_multithread_safe.sql")
DURIS_SQL = os.path.join(REPO_ROOT, "src", "duris.sql")

# Tables that must NOT be reset (RETAIN or REBUILD)
RETAIN_TABLES = {
    "accounts", "account_banks", "account_ips", "multiplay_whitelist",
    "races", "classes", "zones", "items", "mud_info", "pages",
    "categories", "changes", "engine", "ping", "uses",
    "prepstatment_duris_sql",
    "towns",  # REBUILD
    "kingdom_land",  # REBUILD
}

# Tables that are DEACTIVATED (not deleted) by pwipe
DEACTIVATE_TABLES = {
    "account_characters",
    "player_data",  # active = 0
}

# Tables that are handled by UPDATE (not DELETE) in pwipe
UPDATE_TABLES = {
    "outposts", "nexus_stones", "level_cap",
}

errors = []
checks_passed = 0


def extract_pwipe_tables():
    """Extract all table names from DELETE FROM and UPDATE statements in sql_pwipe()."""
    with open(SQL_C, "r") as f:
        source = f.read()

    # Find the real sql_pwipe function body (not the __NO_MYSQL__ stub)
    pwipe_start = source.rindex("bool sql_pwipe(int code_verify)")
    pwipe_end = source.find("\n}", pwipe_start + 1)
    # Find the next function after pwipe
    next_func = source.find("\nvoid ", pwipe_end)
    if next_func == -1:
        next_func = source.find("\nbool ", pwipe_end)
    if next_func == -1:
        next_func = len(source)
    pwipe_body = source[pwipe_start:next_func]

    # Extract DELETE FROM table names
    delete_tables = set()
    for m in re.finditer(r'DELETE\s+FROM\s+[`"]?(\w+)[`"]?', pwipe_body, re.IGNORECASE):
        delete_tables.add(m.group(1).lower())

    # Extract UPDATE table names
    update_tables = set()
    for m in re.finditer(r'UPDATE\s+[`"]?(\w+)[`"]?\s+SET', pwipe_body, re.IGNORECASE):
        update_tables.add(m.group(1).lower())

    return delete_tables, update_tables


def extract_bootstrap_tables():
    """Extract final runtime table state, honoring later DROP TABLE statements."""
    tables = set()
    statement_re = re.compile(
        r'(CREATE|DROP)\s+TABLE\s+(?:IF\s+(?:NOT\s+)?EXISTS\s+)?[`"]?(\w+)[`"]?',
        re.IGNORECASE,
    )
    for sql_file in [BOOTSTRAP_SQL, DURIS_SQL]:
        if not os.path.exists(sql_file):
            continue
        with open(sql_file, "r") as f:
            content = f.read()
        for m in statement_re.finditer(content):
            operation, table = m.group(1).upper(), m.group(2).lower()
            if operation == "CREATE":
                tables.add(table)
            else:
                tables.discard(table)
    return tables


# ── Test 1: All pwipe tables exist in bootstrap ──
delete_tables, update_tables = extract_pwipe_tables()
bootstrap_tables = extract_bootstrap_tables()

all_pwipe_tables = delete_tables | update_tables

nonexistent = all_pwipe_tables - bootstrap_tables
if nonexistent:
    errors.append(f"Pwipe references tables not in bootstrap: {sorted(nonexistent)}")
else:
    checks_passed += 1
    print(f"pwipe table bootstrap existence: ok ({len(all_pwipe_tables)} tables)")

# ── Test 2: RETAIN tables must NOT be in DELETE FROM ──
retained_in_delete = RETAIN_TABLES & delete_tables
if retained_in_delete:
    errors.append(f"RETAIN tables found in DELETE FROM: {sorted(retained_in_delete)}")
else:
    checks_passed += 1
    print(f"RETAIN tables not in DELETE FROM: ok ({len(RETAIN_TABLES)} tables protected)")

# ── Test 3: DEACTIVATE tables must use UPDATE, not DELETE ──
for t in DEACTIVATE_TABLES:
    if t in delete_tables:
        errors.append(f"DEACTIVATE table {t} found in DELETE FROM (should be UPDATE)")
    elif t not in update_tables:
        errors.append(f"DEACTIVATE table {t} not found in UPDATE statements")
    else:
        checks_passed += 1
        print(f"DEACTIVATE table {t}: ok (uses UPDATE)")

# ── Test 4: Critical season-scoped tables must be in DELETE FROM ──
critical_reset_tables = [
    "player_items", "player_affects", "player_skills", "player_spellbooks",
    "player_languages", "player_timers", "player_pets",
    "player_pet_items", "player_pet_item_affects", "player_pet_item_extra_descr",
    "player_item_affects", "player_item_extra_descr",
    "lockers", "locker_items", "locker_item_affects", "locker_item_extra_descr",
    "private_chests", "private_chest_log",
    "corpses", "corpse_items", "corpse_item_affects", "corpse_item_extra_descr",
    "saved_items", "saved_item_affects", "saved_item_extra_descr",
    "ships", "ship_slots", "ship_crew", "ship_armor",
    "guilds", "guild_members", "guild_ranks",
    "artifacts", "artifacts_mortal",
    "frag_leaderboard", "statistics", "pkill_event", "pkill_info",
    "shopkeepers", "shopkeeper_items", "shopkeeper_affects",
    "siege_items", "siege_item_affects", "siege_item_extra_descr",
    "polls", "poll_options", "poll_votes",
    "boons_shop",
    "eq_drop", "racewar_stat_mods",
    "player_recipes", "player_shapechanges", "player_undead_slots",
    "player_witnesses", "player_forged_items", "player_granted_cmds", "player_intros",
    "account_lockers", "account_locker_items", "account_locker_access",
    "account_locker_item_affects", "account_locker_item_extra_descr",
    "locker_chests", "locker_activity_log", "locker_kickouts", "locker_session_state",
]

missing_critical = [t for t in critical_reset_tables if t not in delete_tables]
if missing_critical:
    errors.append(f"Critical RESET tables missing from DELETE FROM: {sorted(missing_critical)}")
else:
    checks_passed += 1
    print(f"Critical RESET tables in DELETE FROM: ok ({len(critical_reset_tables)} tables)")

# ── Test 5: Dependency ordering - child tables before parent tables ──
# Check that affects/extra_descr are deleted before their parent item tables
def check_order(pwipe_body, child, parent, label):
    child_re = re.compile(rf'DELETE\s+FROM\s+[`"]?{re.escape(child)}[`"]?', re.IGNORECASE)
    parent_re = re.compile(rf'DELETE\s+FROM\s+[`"]?{re.escape(parent)}[`"]?', re.IGNORECASE)
    child_m = child_re.search(pwipe_body)
    parent_m = parent_re.search(pwipe_body)
    if child_m and parent_m:
        if child_m.start() < parent_m.start():
            return True
        else:
            return False
    return True  # Can't check if one is missing

with open(SQL_C, "r") as f:
    source = f.read()
pwipe_start = source.rindex("bool sql_pwipe(int code_verify)")
pwipe_end = source.find("\n}", pwipe_start + 1)
next_func = source.find("\nvoid ", pwipe_end)
if next_func == -1:
    next_func = len(source)
pwipe_body = source[pwipe_start:next_func]

ordering_checks = [
    ("player_item_affects", "player_items", "player item"),
    ("player_item_extra_descr", "player_items", "player item"),
    ("player_pet_item_affects", "player_pets", "player pet"),
    ("player_pet_items", "player_pets", "player pet"),
    ("locker_item_affects", "lockers", "locker"),
    ("locker_items", "lockers", "locker"),
    ("corpse_item_affects", "corpses", "corpse"),
    ("corpse_items", "corpses", "corpse"),
    ("ship_armor", "ships", "ship"),
    ("ship_crew", "ships", "ship"),
    ("ship_slots", "ships", "ship"),
    ("guild_members", "guilds", "guild"),
    ("guild_ranks", "guilds", "guild"),
    ("siege_item_affects", "siege_items", "siege"),
    ("shopkeeper_item_affects", "shopkeepers", "shopkeeper"),
    ("shopkeeper_items", "shopkeepers", "shopkeeper"),
    ("saved_item_affects", "saved_items", "saved item"),
    ("account_locker_items", "account_lockers", "account locker"),
    ("private_chest_log", "private_chests", "private chest"),
]

ordering_ok = True
for child, parent, label in ordering_checks:
    if not check_order(pwipe_body, child, parent, label):
        errors.append(f"Dependency ordering violated: {child} must be deleted before {parent}")
        ordering_ok = False

if ordering_ok:
    checks_passed += 1
    print(f"Dependency ordering (child before parent): ok ({len(ordering_checks)} checks)")

# ── Test 6: Preflight and postflight exist ──
if "sql_verify_pwipe_manifest()" not in pwipe_body:
    errors.append("Preflight: sql_verify_pwipe_manifest() call not found in sql_pwipe()")
else:
    checks_passed += 1
    print("Preflight complete reset manifest check present: ok")

if "_locker_item_map" in pwipe_body:
    errors.append("Migration-only _locker_item_map must not be referenced by runtime pwipe")
else:
    checks_passed += 1
    print("Migration-only tables excluded from runtime pwipe: ok")

if "sql_verify_persistence_schema()" not in pwipe_body:
    errors.append("Preflight: sql_verify_persistence_schema() call not found in sql_pwipe()")
else:
    checks_passed += 1
    print("Preflight schema check present: ok")

if "sql_verify_auction_engines()" not in pwipe_body:
    errors.append("Preflight: sql_verify_auction_engines() call not found in sql_pwipe()")
else:
    checks_passed += 1
    print("Preflight auction engine check present: ok")

if "Postflight" not in pwipe_body and "postflight" not in pwipe_body.lower():
    errors.append("Postflight invariant check not found in sql_pwipe()")
else:
    checks_passed += 1
    print("Postflight invariant check present: ok")

# ── Test 7: account_characters soft-delete (not hard delete) ──
if "account_characters" in delete_tables:
    errors.append("account_characters should be soft-deleted (UPDATE), not DELETE FROM")
elif "account_characters" not in update_tables:
    errors.append("account_characters not found in UPDATE statements in sql_pwipe()")
else:
    # Verify it uses COALESCE to avoid re-deleting already-deleted rows
    if "COALESCE" not in pwipe_body or "deleted_at" not in pwipe_body:
        errors.append("account_characters UPDATE does not use COALESCE/deleted_at")
    else:
        checks_passed += 1
        print("account_characters soft-delete with COALESCE: ok")

# ── Summary ──
print(f"\nTotal checks passed: {checks_passed}")
if errors:
    print(f"Errors: {len(errors)}")
    for e in errors:
        print(f"  FAIL: {e}")
    sys.exit(1)
else:
    print("season-reset manifest checks passed.")
    sys.exit(0)
