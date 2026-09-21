"""Source-level guards for the atomic crafting conservation contract."""

from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


def section(text: str, start: str, end: str) -> str:
    begin = text.index(start)
    finish = text.index(end, begin)
    return text[begin:finish]


def require_in_order(text: str, *needles: str) -> None:
    position = -1
    for needle in needles:
        position = text.index(needle, position + 1)


command_h = (ROOT / "src/item/item_transfer_command.h").read_text()
command_c = (ROOT / "src/item/item_transfer_command.c").read_text()
repository_c = (ROOT / "src/item/item_transfer_repository.c").read_text()
runtime_c = (ROOT / "src/item/item_ownership_runtime.c").read_text()
movement_c = (ROOT / "src/item/item_movement_transaction.c").read_text()
flatfile_c = (ROOT / "src/flatfile/flatfile_item_repository.c").read_text()
materialization_c = (ROOT / "src/flatfile/flatfile_shop_trade_materialization.c").read_text()
salchemist_c = (ROOT / "src/classes/salchemist.c").read_text()
drannak_c = (ROOT / "src/classes/drannak.c").read_text()
mysql_runner = (ROOT / "tests/async/run_item_transfer_schema_mysql.sh").read_text()

assert "craft," in command_h
assert "case item_transfer_reason::craft:" in command_c
assert "decode_craft_outputs" in command_c
assert "ITEM_TRANSFER_ABSENT_REVISION" in command_c
assert "output_key" in command_c
assert "payload.reason != item_transfer_reason::craft" in command_c

# A craft may retire inputs without producing an object, but successful outputs
# must be fenced as absent and decoded from a complete snapshot list.
require_in_order(
    command_c,
    "const bool craft = payload.reason == item_transfer_reason::craft;",
    "decode_craft_outputs(payload, &outputs)",
    "outputs.empty() && !find_payload_item(payload, payload.selected_item_uid)",
)

assert "bool execute_craft" in repository_c
assert "sync_restitution_runtime_payload(connection, payload)" in repository_c
assert "payload.reason == item_transfer_reason::craft" in repository_c
assert "Craft payloads carry output snapshots" in repository_c
assert "event_index_base + index" in repository_c
assert "insert_craft_snapshot_rows" in repository_c
assert "update_owner_revision(connection, payload.from_owner" in repository_c
assert "src/sql/item_extra_descr_codec.c" in mysql_runner
assert "tests/async/item_extra_descr_codec_sql_escape_stub.cpp" in mysql_runner
assert "src/persistence/player_death_restitution_command.c" in mysql_runner
assert "src/persistence/player_death_restitution_repository.c" in mysql_runner
assert "item_transfer_reason::craft" in (ROOT / "tests/async/item_transfer_mysql_harness.cpp").read_text()
assert "payload.reason == item_transfer_reason::craft" in runtime_c
assert "item_ownership_runtime_hydrate_many_atomic" in runtime_c

assert "unsigned int apply_craft" in flatfile_c
assert "apply_craft(&candidate, payload, &result)" in flatfile_c
assert "flatfile_item_transfer_materialization_prepare" in materialization_c
assert "payload.reason == item_transfer_reason::craft" in materialization_c

# Gameplay writers must submit before durable completion; the old direct
# extraction/publication paths must not remain in the relevant command spans.
poison = section(salchemist_c, "void do_mixpoison", "void do_mix(")
mix = section(salchemist_c, "void do_mix(", "bool is_neg_good")
encrust = section(salchemist_c, "void do_encrust", "int encrusted_eq_proc")
pvp = section(drannak_c, "int pvp_store", "// Proc for weapon")

for writer in (poison, mix, encrust, pvp):
    assert "item_movement_transaction_submit_craft" in writer

assert "extract_used_poison_ingredients" not in poison
assert "obj_to_char" not in poison
assert "extract_used_ingredients" not in mix
assert "obj_to_char" not in mix
assert "struct alchemy_bottle_candidate" in mix
assert "available_bottles.insert" in mix
assert "if (ingredients_consumed)" in mix
assert "obj_to_char(new_item, ch)" not in encrust
assert "chaos_material_pouch_record_generated" not in encrust
assert "Virtual Chaos-pouch encrust is temporarily unavailable" in encrust
assert "if (craft.kind >= 1 && craft.kind <= 3)" in salchemist_c
assert "const alchemy_craft_context context = { skill, 5, false };" in encrust
assert "const alchemy_craft_context context = { skill, 4, false };" in encrust
assert "vnum_from_inv" not in pvp
assert "obj_to_char(orb, pl)" not in pvp

# Zero-output failure crafts must be admitted without dereferencing outputs.
zero_output = section(
    movement_c,
    "bool item_movement_transaction_submit_craft",
    "bool item_creation_grant_submit_to_player",
)
assert "output_count && (!outputs" in zero_output
assert "outputs.empty() ? nullptr : outputs.data()" in mix
assert "nullptr, 0" in encrust

print("Issue 551 crafting conservation contract passed")
