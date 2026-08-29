#!/usr/bin/env python3

import pathlib
import subprocess
import tempfile


ROOT = pathlib.Path(__file__).resolve().parents[2]

with tempfile.TemporaryDirectory(prefix="duris-flat-association-") as temporary:
    temporary_path = pathlib.Path(temporary)
    binary = temporary_path / "flatfile_association_test"
    compile_result = subprocess.run(
        [
            "g++",
            "-std=c++20",
            "-Wall",
            "-Wextra",
            "-Wpedantic",
            "-Werror",
            "-Isrc",
            "tests/async/flatfile_association_repository_harness.cpp",
            "src/flatfile_association_repository.c",
            "src/flatfile_authority_transaction.c",
            "src/flatfile_store.c",
            "-lcrypto",
            "-pthread",
            "-o",
            str(binary),
        ],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    run_result = subprocess.run(
        [str(binary), str(temporary_path / "state")],
        cwd=ROOT,
        text=True,
        stdout=subprocess.PIPE,
        stderr=subprocess.STDOUT,
    )
    if run_result.returncode:
        raise SystemExit(run_result.stdout)
    print(run_result.stdout.strip())

preprocess = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-I/usr/include/libxml2",
        "-E",
        "-P",
        "src/assocs.c",
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if preprocess.returncode:
    raise SystemExit(preprocess.stdout)
for token in (
    "flatfile_association_list(root, &records, &error)",
    "flatfile_association_establish(root, {}, &error)",
    "flatfile_association_save(root, record, &error)",
    "flatfile_association_erase(root, id_number, &error)",
    "flatfile_association_ledger_append(root, id_number",
    "flatfile_association_ledger_list(root, id_number, system_entries",
    "missing = load_guild(id) ? 0 : missing + 1",
    "const bool destroyed = owned->destroy()",
    "hall = Guildhall::guildhalls.erase(hall)",
    "if (!reset_one_outpost(building))",
):
    if token not in preprocess.stdout:
        raise SystemExit(f"client-free guild runtime route is missing {token}")
if preprocess.stdout.index("const bool destroyed = owned->destroy()") > preprocess.stdout.index(
    "flatfile_association_erase(root, id_number, &error)"
):
    raise SystemExit("client-free guild deletion does not erase guildhalls before guild authority")
if preprocess.stdout.index("if (!reset_one_outpost(building))") > preprocess.stdout.index(
    "flatfile_association_erase(root, id_number, &error)"
):
    raise SystemExit("client-free guild deletion does not reset outposts before guild authority")
for query in (
    "SELECT id, name FROM associations WHERE id",
    "SELECT id, name, prestige, construction_points FROM associations",
    "guild_transactions",
):
    if query in preprocess.stdout:
        raise SystemExit(f"client-free guild runtime still contains SQL: {query}")

source = (ROOT / "src/assocs.c").read_text()
for token in (
    'sscanf(buf, "%u %ld %ld %13s %c"',
    'sscanf(buf, "%13s %u %u %c"',
    '((frag_fields == 3) != (new_guild->frags.top_frags == 0))',
):
    if token not in source:
        raise SystemExit(f"historical guild parser safety is missing {token}")

alliance_preprocess = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-I/usr/include/libxml2",
        "-E",
        "-P",
        "src/alliances.c",
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if alliance_preprocess.returncode:
    raise SystemExit(alliance_preprocess.stdout)
for token in (
    "flatfile_alliance_list(root, &records, &error)",
    "flatfile_alliance_replace(root, records, &error)",
    "alliance.tribute_owed = record.tribute_owed",
):
    if token not in alliance_preprocess.stdout:
        raise SystemExit(f"client-free alliance runtime route is missing {token}")
for query in (
    "SELECT forging_assoc_id",
    "DELETE FROM alliances",
    "INSERT INTO alliances",
):
    if query in alliance_preprocess.stdout:
        raise SystemExit(f"client-free alliance runtime still contains SQL: {query}")

guildhall_preprocess = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-I/usr/include/libxml2",
        "-E",
        "-P",
        "src/guildhall_db.c",
        "src/guildhall.c",
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if guildhall_preprocess.returncode:
    raise SystemExit(guildhall_preprocess.stdout)
for token in (
    "flatfile_guildhall_list(root, &records, &error)",
    "flatfile_guildhall_save(root, record, &error)",
    "flatfile_guildhall_erase(root, gh->id, &error)",
    "flatfile_guildhall_room_erase(",
):
    if token not in guildhall_preprocess.stdout:
        raise SystemExit(f"client-free guildhall runtime route is missing {token}")
for query in (
    "select coalesce(max(id), 0) from guildhalls",
    "select id, assoc_id, type, outside_vnum, racewar from guildhalls",
    "select id, vnum, guildhall_id, name, type",
    "replace into guildhalls",
    "replace into guildhall_rooms",
    "delete from guildhalls",
    "delete from guildhall_rooms",
):
    if query in guildhall_preprocess.stdout:
        raise SystemExit(f"client-free guildhall runtime still contains SQL: {query}")
if "this->rooms[i]->save()" in guildhall_preprocess.stdout:
    raise SystemExit("client-free guildhall save is not a single catalog snapshot")

outpost_preprocess = subprocess.run(
    [
        "g++",
        "-std=c++20",
        "-D__NO_MYSQL__",
        "-Isrc/no_mysql",
        "-Isrc",
        "-Isrc/ships",
        "-I/usr/include/libxml2",
        "-E",
        "-P",
        "src/outposts.c",
        "src/buildings.c",
    ],
    cwd=ROOT,
    text=True,
    stdout=subprocess.PIPE,
    stderr=subprocess.STDOUT,
)
if outpost_preprocess.returncode:
    raise SystemExit(outpost_preprocess.stdout)
for token in (
    "flatfile_outpost_list(root, &stored, &error)",
    "flatfile_outpost_establish(root, defaults, &error)",
    "flatfile_outpost_save(root, flat_outpost(record), &error)",
    "record.portal_room = 1",
    "record.golems = get_outpost_golems(building) + 1",
    "record.archers = 1",
    "record.meurtriere = 1",
    "record.hitpoints = (",
    "record.level = 8",
    "record.owner_id = owner_id",
    "persist_outpost_owner(this, new_guild)",
    "void event_outposts_upkeep(",
):
    if token not in outpost_preprocess.stdout:
        raise SystemExit(f"client-free outpost runtime route is missing {token}")
for query in (
    "INSERT IGNORE into outposts",
    "SELECT id, owner_id, level, walls",
    "UPDATE outposts SET",
    "SELECT id, wood, stone FROM associations",
    "UPDATE associations SET wood",
):
    if query in outpost_preprocess.stdout:
        raise SystemExit(f"client-free outpost runtime still contains SQL: {query}")
