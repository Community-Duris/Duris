#!/usr/bin/env python3
"""Round-trip SQL item properties and verify their live hydration contract."""

import ast
import hashlib
import json
import subprocess
import tempfile
from pathlib import Path

from _paths import SRC, rel

ROOT = Path(__file__).resolve().parents[2]

# The SQL stores only the fields missing from the existing player_items projection.
SAVE = (SRC / "player_snapshot_repository.c").read_text()
LOAD = (SRC / "player_load_repository.c").read_text()
ITEMS = (SRC / "player_load_items.c").read_text()
SQL_PLAYER = (ROOT / "src/sql/sql_player.c").read_text()
LOAD_HEADER = (SRC / "player_load_repository.h").read_text()
MIGRATION_PATH = ROOT / "migrations/immutable/0035_player_item_dynamic_state.sql"
VERIFIER_PATH = ROOT / "migrations/immutable/0035_player_item_dynamic_state.sh"
MIGRATION = MIGRATION_PATH.read_text()
VERIFIER = VERIFIER_PATH.read_text()

shop_start = SQL_PLAYER.index("static int sql_save_shopkeeper_item(")
shop_end = SQL_PLAYER.index("static bool sql_save_shopkeeper_affects(", shop_start)
shop_save = SQL_PLAYER[shop_start:shop_end]
property_writer = "sql_player_item_properties_value_suffix(obj, &properties_suffix)"
if SQL_PLAYER.count(property_writer) != 4 or shop_save.count(property_writer) != 1:
    raise SystemExit("a synchronous player or shopkeeper item INSERT omits item_properties")
if SQL_PLAYER.count("player_item_properties_sql_column_suffix()") != 3:
    raise SystemExit("not every synchronous player-item INSERT declares item_properties")
if "if (obj->affects)" not in SQL_PLAYER:
    raise SystemExit("dynamic-affect items can still bypass the versioned single-row writer")
LOAD_COMPACT = " ".join(LOAD.split())
if "player_item_properties_decode_sql_row(row[42], row[43], &item.extra2_flags," \
        not in LOAD_COMPACT or "if (has_item_properties)" not in LOAD_COMPACT:
    raise SystemExit("SQL row payload and legacy-NULL handling are not wired into loading")

for token in (
    "player_item_properties_encode(",
    "row.extra2_flags, row.dynamic_affects",
    'sql << ",item_properties"',
    "quote(connection, item_properties)",
):
    if token not in SAVE:
        raise SystemExit(f"player item SQL save contract is missing: {token}")
for token in (
    '"pi.item_properties,OCTET_LENGTH(pi.item_properties),own.equipment_slot,"',
    '"FROM player_items pi "',
    "PLAYER_LOAD_ITEM_OVERRIDE_EXTRA2_FLAGS",
    "PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS",
    "player_item_properties_decode_sql_row(",
    "if (has_item_properties)",
):
    if token not in LOAD:
        raise SystemExit(f"player item SQL load contract is missing: {token}")
for token in (
    "PLAYER_LOAD_ITEM_OVERRIDE_EXTRA2_FLAGS",
    "PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS",
):
    if token not in LOAD_HEADER:
        raise SystemExit(f"player item override contract is missing: {token}")
for token in (
    "complete_snapshot_state ||",
    "PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS",
    "std::find_if(",
    "TAG_ALTERED_EXTRA2",
    "set_obj_affected_extra(object",
    "set_obj_affected(object",
):
    if token not in ITEMS:
        raise SystemExit(f"dynamic affect materialization contract is missing: {token}")
for token in (
    "ADD COLUMN item_properties MEDIUMTEXT CHARACTER SET ascii DEFAULT NULL",
    "FROM information_schema.COLUMNS",
    "@item_properties_exists=0",
    "PREPARE item_properties_stmt",
):
    if token not in MIGRATION:
        raise SystemExit(f"additive item_properties migration is incomplete: {token}")
if "column_name='item_properties'" not in VERIFIER or \
        "mediumtext:ascii:YES:<NULL>" not in VERIFIER:
    raise SystemExit("item_properties schema verifier is incomplete")

manifest = json.loads((ROOT / "migrations/migration_manifest.json").read_text())
step = next((item for item in manifest["migrations"]
             if item["id"] == "0035_player_item_dynamic_state"), None)
if step is None or step["sequence"] != 35 or \
        step["apply"] != "immutable/0035_player_item_dynamic_state.sql" or \
        step["verify"] != "immutable/0035_player_item_dynamic_state.sh" or \
        step["apply_checksum"] != hashlib.sha256(MIGRATION_PATH.read_bytes()).hexdigest() or \
        step["verify_checksum"] != hashlib.sha256(VERIFIER_PATH.read_bytes()).hexdigest():
    raise SystemExit("0035 migration manifest entry is missing or stale")

# Reuse the existing production-loader fixture, adding a focused dynamic affect
# materialization case. This checks the real player_load_items.c path and the
# harness's in-memory set_obj_affected stubs rather than only spying on requests.
loader_test = (ROOT / "tests/async/test_player_load_items.py").read_text()
module = ast.parse(loader_test)
harness = None
for node in module.body:
    if isinstance(node, ast.Assign) and any(
        isinstance(target, ast.Name) and target.id == "HARNESS"
        for target in node.targets
    ):
        harness = ast.literal_eval(node.value)
        break
if harness is None:
    raise SystemExit("could not locate player-load in-memory test harness")
anchor = "    return 0;\n}\n"
if not harness.endswith(anchor):
    raise SystemExit("player-load harness final return changed")
case = r'''    {
        reset_test_state();
        test_character owner(42);
        player_load_result result = base_result();
        add_item(result, 71, 700, 100, PLAYER_SNAPSHOT_NO_PARENT, 0);
        add_item(result, 72, 701, 101, 0, 0);
        constexpr uint32_t base_flags = UINT32_C(0x01000000);
        constexpr uint64_t affect_flag = UINT64_C(0x00000020);
        auto &item = result.snapshot.items[1];
        item.extra2_flags = static_cast<uint32_t>(base_flags | affect_flag);
        item.dynamic_affects = {
            { TAG_ALTERED_EXTRA2, 0, base_flags },
            { 7, -41, affect_flag },
            { 8, 42, 0 },
        };
        std::string encoded_properties;
        assert(player_item_properties_encode(item.extra2_flags, item.dynamic_affects,
                                             &encoded_properties) ==
               player_snapshot_codec_result::ok);
        item.extra2_flags = 0;
        item.dynamic_affects.clear();
        assert(player_item_properties_decode(encoded_properties, &item.extra2_flags,
                                             &item.dynamic_affects) ==
               player_snapshot_codec_result::ok);
        result.item_identities[1].override_mask |=
            PLAYER_LOAD_ITEM_OVERRIDE_EXTRA2_FLAGS |
            PLAYER_LOAD_ITEM_OVERRIDE_DYNAMIC_AFFECTS;
        player_load_item_materialize_metrics metrics = {};
        assert(player_load_item_graph_materialize_for_owner(
            &owner.character, result.snapshot.items, result.item_identities,
            { item_owner_type::player, 42, 0 }, result.item_owner_revision,
            true, false, &metrics));
        P_obj loaded = owner.character.carrying;
        assert(loaded && loaded->obj_uid == 700 && loaded->contains &&
               loaded->contains->obj_uid == 701);
        P_obj affected = loaded->contains;
        assert(affected->extra2_flags == (base_flags | affect_flag));
        assert(affected->affects && affected->affects->type == 7 &&
               affected->affects->data == -41 &&
               affected->affects->extra2 == affect_flag);
        assert(affected->affects->next && affected->affects->next->type == 8 &&
               affected->affects->next->data == 42 &&
               affected->affects->next->extra2 == 0 &&
               !affected->affects->next->next);
        uint64_t owner_revision = 0;
        assert(item_ownership_runtime_owner_revision(
            { item_owner_type::player, 42, 0 }, &owner_revision));
        assert(owner_revision == 7);
        release_tree(loaded);
    }
'''
harness = harness[:-len(anchor)] + case + anchor

with tempfile.TemporaryDirectory(prefix="duris-pa-item-dynamic-state-") as temporary:
    temporary = Path(temporary)
    codec_binary = temporary / "item_dynamic_state_codec"
    compile_result = subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-Isrc", "tests/async/pa_item_dynamic_state_codec_harness.cpp",
            rel("player_snapshot_codec.c"), "-o", str(codec_binary),
        ], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    codec_result = subprocess.run(
        [str(codec_binary)], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20,
    )
    if codec_result.returncode:
        raise SystemExit(codec_result.stdout)

    loader_source = temporary / "item_dynamic_state_loader.cpp"
    loader_binary = temporary / "item_dynamic_state_loader"
    loader_source.write_text(harness)
    compile_result = subprocess.run(
        [
            "g++", "-std=c++20", "-Wall", "-Wextra", "-Wpedantic", "-Werror",
            "-Isrc", str(loader_source), rel("player_load_items.c"),
            rel("player_load_pets.c"), "src/player/pet_restore_state.c",
            "src/player/pet_restore_runtime.c", rel("player_snapshot_codec.c"),
            rel("item_transfer_command.c"), rel("craft_pouch_mutation.c"), rel("chaos_pouch_ledger.c"), rel("item_ownership_runtime.c"),
            rel("critical_command.c"), "-lcrypto", "-o", str(loader_binary),
        ], cwd=ROOT, text=True, stdout=subprocess.PIPE, stderr=subprocess.STDOUT,
    )
    if compile_result.returncode:
        raise SystemExit(compile_result.stdout)
    loader_result = subprocess.run(
        [str(loader_binary)], cwd=ROOT, text=True,
        stdout=subprocess.PIPE, stderr=subprocess.STDOUT, timeout=20,
    )
    if loader_result.returncode:
        raise SystemExit(loader_result.stdout)

print(codec_result.stdout.strip())
print("player item dynamic-state SQL and in-memory hydration contracts passed")
