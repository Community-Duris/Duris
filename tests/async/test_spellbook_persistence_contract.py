#!/usr/bin/env python3
"""Contracts for preserving native spellbook bitmaps at every save boundary."""

from _paths import source
from contract_text import contains, index


SQL = source("sql/sql_player.c").read_text()
FILES = source("core/files.c").read_text()
MEMORIZE = source("classes/memorize.c").read_text()
CAPTURE = source("player/player_snapshot_capture.c").read_text()
LOAD_ITEMS = source("player/player_load_items.c").read_text()
REWARD = source("account/account_reward_snapshot.c").read_text()
CODEC = source("sql/item_extra_descr_codec.c").read_text()
REPOSITORY = source("player/player_snapshot_repository.c").read_text()
LOCKER_ASYNC = source("persistence/locker_async.c").read_text()


def body(text, signature, next_signature):
    start = index(text, signature)
    end = index(text, next_signature, start + len(signature))
    return text[start:end]


def last_body(text, signature, next_signature):
    start = text.rindex(signature)
    end = index(text, next_signature, start + len(signature))
    return text[start:end]


private_loader = last_body(SQL, "void sql_load_private_chest_items(", "mysql_free_result(result);")
assert index(private_loader, "sql_load_item_affects_from_table(") < index(
	private_loader, 'sql_load_item_extra_descr_from_table(item_id, obj, "locker_item")'
)
assert index(private_loader, 'sql_load_item_extra_descr_from_table(item_id, obj, "locker_item")') < index(
	private_loader, "obj_to_obj(obj, chest_obj)"
)

filtered_loader = body(SQL, "static P_obj sql_load_locker_items_filtered(", "static P_obj sql_load_locker_items(")
assert contains(filtered_loader, 'sql_load_item_extra_descr_from_table(item_id, obj, "locker_item")')
assert contains(filtered_loader, "obj->db_item_id = item_id")
assert 'sql_load_item_extra_descr_from_table(item_id, obj, "locker_item_extra_descr")' not in SQL

saved_writer = body(SQL, "static int sql_save_saved_item_recursive(", "bool sql_save_saved_item(")
assert contains(saved_writer, 'sql_save_item_extra_descr(item_id, obj, "saved_item_extra_descr")')

saved_contents = body(SQL, "static P_obj sql_load_saved_item_contents(", "void sql_restore_saved_items(")
assert contains(saved_contents, 'sql_load_item_extra_descr_from_table(item_id, obj, "saved_item")')
saved_restore = last_body(SQL, "void sql_restore_saved_items(", "#define SHIP_SQL_BATCH_SIZE")
assert contains(saved_restore, 'sql_load_item_extra_descr_from_table(item_id, obj, "saved_item")')

shop_writer = body(SQL, "static int sql_save_shopkeeper_item(", "static bool sql_save_shopkeeper_affects(")
assert contains(shop_writer, 'sql_save_item_extra_descr(item_id, obj, "shopkeeper_item_extra_descr")')
shop_loader = body(SQL, "static void sql_load_all_shopkeeper_items(", "static bool sql_load_shopkeeper_affects(")
assert contains(shop_loader, 'sql_load_item_extra_descr_from_table(item_id, obj, "shopkeeper_item")')

extra_descr_save = body(SQL, "static bool sql_save_item_extra_descr(", "// save a single item")
assert contains(extra_descr_save, "spellbook_bits")
assert contains(extra_descr_save, "spellbook_emitted")
assert contains(extra_descr_save, "char query[32768]")
assert contains(extra_descr_save, "static_cast<size_t>(written) >= sizeof(query)")
assert contains(LOCKER_ASYNC, "spellbook_bits")
assert contains(LOCKER_ASYNC, "spellbook_emitted")
assert contains(LOCKER_ASYNC, "sql_encode_item_extra_descr(source_keyword, source_description")

assert contains(REPOSITORY, "canonicalize_snapshot_extra_description")
assert contains(REPOSITORY, 'sql_decode_stored_spellbook("SPELLBOOK"')
assert contains(REPOSITORY, 'description.spellbook != (description.keyword == "SPELLBOOK")')
assert contains(REPOSITORY, "description.spell_ids")
assert contains(REPOSITORY, "if (!description.spell_ids.empty())")

assert contains(FILES, "has_serializable_extra_description")
assert contains(FILES, "sql_item_extra_descr_is_spellbook_marker(ed->keyword)")
assert contains(FILES, "binary data and must never pass through ADD_STRING/GET_STRING")
assert contains(FILES, "ignored legacy flatfile spellbook marker")
assert contains(FILES, "memcpy(s, *buf, (size_t)len)")
assert FILES.count("oversized spellbook bitmap") == 2
assert contains(MEMORIZE, "spl < 0 || spl >= MAX_SKILLS")
assert contains(MEMORIZE, "strlen(tmp->keyword) == 3")
assert contains(MEMORIZE, "memset(tmp, 0, sizeof(*tmp))")
assert contains(MEMORIZE, "memset(tmp->description, 0, byte_count)")
assert contains(MEMORIZE, "static_cast<unsigned char>(tmp->description[spl / 8])")

assert contains(CAPTURE, "static_cast<unsigned char>(description->description[skill_id / 8])")
assert contains(LOAD_ITEMS, "static_cast<uint64_t>(MAX_SKILLS) - 1 - digit")

assert contains(REWARD, 'cJSON_AddStringToObject(entry, "keyword", "SPELLBOOK")')
assert contains(REWARD, 'sql_decode_stored_spellbook("SPELLBOOK"')
assert contains(REWARD, "legacy raw spellbook marker normalized to empty bitmap")
assert contains(REWARD, "REMOVE_BIT(obj->str_mask, STRUNG_EDESC)")

assert contains(CODEC, "std::array<bool, MAX_SKILLS> seen")
assert contains(CODEC, "sql_spellbook_decode_status::invalid")
assert contains(CODEC, 'duplicate_string("[]")')

print("spellbook persistence boundary contracts passed")
