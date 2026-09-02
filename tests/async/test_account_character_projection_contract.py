#!/usr/bin/env python3
"""Account projection recovery and post-extraction cache regressions."""

from _paths import SRC
import os
from pathlib import Path
import subprocess
import sys
import tempfile

from contract_text import contains, index

ROOT = Path(__file__).resolve().parents[2]
account = (SRC / "account.c").read_text(encoding="utf-8", errors="replace")
sql = (SRC / "sql.c").read_text(encoding="utf-8", errors="replace")
sql_player = (SRC / "sql_player.c").read_text(
    encoding="utf-8", errors="replace"
)
mysql_sql_player = sql_player[sql_player.index("\n#else\n\n// globals") :]


def body(text, signature):
    """Return one function or structure definition through its closing brace."""
    start = index(text, signature)
    while ";" in text[start + len(signature) : text.index("{", start)]:
        start = index(text, signature, start + len(signature))
    depth = 0
    opening = text.index("{", start)
    for position in range(opening, len(text)):
        if text[position] == "{":
            depth += 1
        elif text[position] == "}":
            depth -= 1
            if depth == 0:
                return text[start : position + 1]
    raise AssertionError(f"unterminated definition: {signature}")


checks = []

mapping = body(sql, "void sql_update_account_character(P_char ch)")
checks.append(
    (
        "the account mapping is never rewritten to a placeholder account name",
        contains(
            mapping,
            "if (!ch->desc || !ch->desc->account || !ch->desc->account->acct_name ||",
        )
        and contains(mapping, "outcome=skipped_no_account")
        and mapping.index("outcome=skipped_no_account")
        < mapping.index("INSERT INTO account_characters "),
    )
)

reload_body = body(account, "int read_account(P_acct acct)")
repair_call = reload_body.index(
    "sql_repair_account_character_projection(name_backup)"
)
checks.append(
    (
        "every MariaDB account load repairs its durable projection before reading it",
        contains(
            reload_body, "sql_repair_account_character_projection(name_backup)"
        )
        and contains(reload_body, "loaded = sql_load_account(name_backup);")
        and repair_call
        < reload_body.index("loaded = sql_load_account(name_backup);", repair_call)
        < reload_body.index("acct->acct_name = check_and_clear(acct->acct_name);"),
    )
)
checks.append(
    (
        "a legitimate empty account is not rebuilt from process-local state in MariaDB",
        contains(reload_body, "if (repaired > 0)")
        and not contains(reload_body, "character_projection_empty"),
    )
)
checks.append(
    (
        "abandoned reload objects are released on repair paths",
        contains(account, "static void free_acct_entry_shallow(struct acct_entry *loaded)")
        and contains(reload_body, "free_acct_entry_shallow(loaded);")
        and contains(reload_body, "flatfile_account_state_release(loaded);"),
    )
)

repair = body(
    mysql_sql_player,
    "int sql_repair_account_character_projection(const char *account_name)",
)
checks.append(
    (
        "repair is sourced from active durable player ownership and honors tombstones",
        contains(repair, "FROM player_data pd")
        and contains(repair, "pd.active=1")
        and contains(repair, "pd.account_name")
        and contains(repair, "tombstone.deleted_at IS NOT NULL")
        and contains(repair, "ON DUPLICATE KEY UPDATE"),
    )
)

pending = body(mysql_sql_player, "struct pending_account_character_cache_update")
checks.append(
    (
        "the queued cache update is an immutable account and character snapshot",
        all(
            contains(pending, field)
            for field in (
                "int pid;",
                "int room;",
                "int level;",
                "char account_name[256];",
                "char character_name[256];",
            )
        )
        and not contains(pending, "P_char"),
    )
)

sync = body(
    mysql_sql_player,
    "static void\nsql_sync_account_character_cache("
    "const struct pending_account_character_cache_update &update)",
)
checks.append(
    (
        "commit-time cache publication targets surviving account objects",
        contains(sync, "for (P_acct account = account_list;")
        and contains(sync, "account->acct_character_list")
        and not contains(sync, "for (P_char candidate = character_list;")
        and not contains(sync, "P_char"),
    )
)

save = body(
    mysql_sql_player, "bool sql_save_player_status(P_char ch, int type, int room)"
)
checks.append(
    (
        "new-character cache snapshots use the database-assigned pid",
        save.index("mysql_insert_id(DB)")
        < save.index("sql_queue_account_character_cache_sync(ch, room)")
        < save.rindex("if (own_txn)"),
    )
)

add_character = body(account, "void add_char_to_account(P_desc d)")
flatfile_add_character = add_character[
    add_character.index("#ifdef __NO_MYSQL__") : add_character.index("#else")
]
mariadb_add_character = add_character[
    add_character.index("#else") : add_character.index("#endif")
]
checks.append(
    (
        "a new character remains in the live account until its first player save",
        contains(add_character, "d->account->acct_character_list = c;")
        and not contains(mariadb_add_character, "write_account("),
    )
)
checks.append(
    (
        "flatfile accounts persist a new character before its first player save",
        contains(flatfile_add_character, "write_account(d->account)"),
    )
)

failed = [name for name, ok in checks if not ok]
for name, ok in checks:
    print(f"[{'PASS' if ok else 'FAIL'}] {name}")

if failed:
    print("\nFailed regression checks:")
    for name in failed:
        print(f"- {name}")
    sys.exit(1)

cache_harness = f"""
#include <cstdlib>
#include <ctime>
#include <strings.h>

struct acct_chars
{{
    int pid;
    char *charname;
    int level;
    int last_room;
    long last_save;
    struct acct_chars *next;
}};

struct acct_entry
{{
    char *acct_name;
    struct acct_chars *acct_character_list;
    struct acct_entry *next;
}};

using P_acct = struct acct_entry *;
P_acct account_list = nullptr;

{pending};

{sync}

static void require(bool condition, int code)
{{
    if (!condition)
        std::exit(code);
}}

int main()
{{
    char account_name[] = "RepairAcct";
    char other_account_name[] = "OtherAcct";
    char character_name[] = "RepairHero";
    char other_character_name[] = "OtherHero";
    acct_chars hero = {{ 7, character_name, 12, 80, 1, nullptr }};
    acct_chars other_hero = {{ 8, other_character_name, 13, 81, 2, nullptr }};
    acct_entry other = {{ other_account_name, &other_hero, nullptr }};
    acct_entry surviving_account = {{ account_name, &hero, &other }};
    account_list = &surviving_account;

    pending_account_character_cache_update update = {{}};
    update.pid = 9001;
    update.room = 4096;
    update.level = 58;
    __builtin_strcpy(update.account_name, account_name);
    __builtin_strcpy(update.character_name, character_name);
    const long before = std::time(nullptr);

    // No character object or character_list exists in this harness: it models
    // commit after extract_char() while the descriptor account menu survives.
    sql_sync_account_character_cache(update);

    require(hero.pid == 9001, 1);
    require(hero.last_room == 4096, 2);
    require(hero.level == 58, 3);
    require(hero.last_save >= before, 4);
    require(other_hero.pid == 8 && other_hero.last_room == 81, 5);
    return 0;
}}
"""

with tempfile.TemporaryDirectory(prefix="duris-account-cache-") as directory:
    temp = Path(directory)
    source = temp / "account_cache.cpp"
    binary = temp / "account_cache"
    source.write_text(cache_harness, encoding="utf-8")
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-O1",
            "-Wall",
            "-Wextra",
            "-Werror",
            "-fsanitize=address,undefined",
            "-fno-omit-frame-pointer",
            str(source),
            "-o",
            str(binary),
        ],
        capture_output=True,
        text=True,
        check=False,
    )
    assert compile_result.returncode == 0, compile_result.stderr
    environment = os.environ.copy()
    environment["ASAN_OPTIONS"] = "detect_leaks=1:halt_on_error=1"
    environment["UBSAN_OPTIONS"] = "halt_on_error=1:print_stacktrace=1"
    subprocess.run([str(binary)], check=True, env=environment)

print("[PASS] post-extraction cache publication updates the surviving account menu")
print("\nAll account character projection checks passed successfully.")
