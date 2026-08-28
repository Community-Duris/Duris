#!/usr/bin/env python3

from pathlib import Path


ROOT = Path(__file__).resolve().parents[2]
SQL_PLAYER = (ROOT / "src/sql_player.c").read_text()
SQL_HEADER = (ROOT / "src/sql_player.h").read_text()
DRANNAK = (ROOT / "src/drannak.c").read_text()
MAKEFILE = (ROOT / "src/Makefile").read_text()

flat_start = SQL_PLAYER.index("#ifdef __NO_MYSQL__")
flat_end = SQL_PLAYER.index("#else", flat_start)
flat = SQL_PLAYER[flat_start:flat_end]

for function, repository_call in (
    ("bool sql_add_spellbook_mob", "flatfile_spellbook_add"),
    ("bool sql_remove_spellbook_mob", "flatfile_spellbook_remove"),
    ("bool sql_has_spellbook_mob", "flatfile_spellbook_contains"),
    ("int *sql_get_spellbook_mobs", "flatfile_spellbook_list"),
    ("bool sql_delete_spellbook_mobs", "flatfile_spellbook_clear"),
):
    start = flat.index(function)
    end = flat.index("\n}", start)
    body = flat[start:end]
    if repository_call not in body:
        raise SystemExit(f"{function} does not route to {repository_call}")
    if "persistence_alert" not in body:
        raise SystemExit(f"{function} does not alert on flat authority failure")

if "bool sql_remove_spellbook_mob(int pid, int mob_vnum);" not in SQL_HEADER:
    raise SystemExit("single-minion removal is missing from the SQL player API")

remove_start = DRANNAK.index('else if (is_abbrev(arg1, "remove"))')
remove_end = DRANNAK.index("\n\telse\n", remove_start)
remove_body = DRANNAK[remove_start:remove_end]
if "sql_remove_spellbook_mob(pid, selected)" not in remove_body:
    raise SystemExit("conjure remove does not use authoritative single-minion deletion")
if "fopen(" in remove_body or "fprintf(" in remove_body:
    raise SystemExit("conjure remove still mutates a legacy spellbook file")

learn = DRANNAK.index("if (!sql_add_spellbook_mob(pid, recipenumber))")
success = DRANNAK.index("You have learned a new minion", learn)
guard = DRANNAK[learn : DRANNAK.index("\n\tact(", learn)]
if "return;" not in guard or learn > success:
    raise SystemExit("conjure learning can report success after a failed persistence write")

normal = SQL_PLAYER[flat_end:]
for token in (
    "insert ignore into player_spellbooks",
    "delete from player_spellbooks where pid=%d and mob_vnum=%d",
    "select 1 from player_spellbooks",
    "select mob_vnum from player_spellbooks",
    "delete from player_spellbooks where pid=%d",
):
    if token not in normal:
        raise SystemExit(f"MariaDB spellbook behavior lost: {token}")

if "flatfile_spellbook_repository.o" not in MAKEFILE:
    raise SystemExit("flat spellbook repository is missing from server build")

print("flat-file spellbook runtime contract passed")
