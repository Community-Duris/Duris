#include "persistence/sql_room_item_payload.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include "world/vnum.obj.h"
#include "persistence/economic_sql_zone_reset_item_transaction.h"
#include "persistence/zone_reset_item_origin_sql.h"
#ifndef __NO_MYSQL__
#include "persistence/economic_accounting_repository.h"
#endif

#include <algorithm>
#include <array>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <climits>
#include <cstring>
#include <memory>
#include <map>
#include <new>
#include <openssl/evp.h>
#include <set>
#include <span>
#include <string>
#include <string_view>
#include <type_traits>
#include <unordered_map>

namespace
{
bool refuse(int error)
{
	errno = error;
	return false;
}

bool encode_one(player_item_snapshot item, std::vector<uint8_t> *bytes)
{
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	return player_item_snapshot_list_encode({ item }, bytes) ==
		       player_snapshot_codec_result::ok &&
	       !bytes->empty() && bytes->size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES;
}

bool exact_item(const player_item_snapshot &item)
{
	// Complete literal text, never reconstruction from mutable prototypes.
	return item.object_uid && item.vnum > 0 && item.type >= ITEM_LOWEST &&
	       item.type <= ITEM_LAST && item.type != ITEM_MONEY && item.type != ITEM_CORPSE &&
	       !(item.extra_flags & ITEM_ARTIFACT) &&
	       item.string_mask == (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) &&
	       item.equipment_slot == 0;
}

#ifndef __NO_MYSQL__
using rows_ptr = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;

rows_ptr query(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
		return { nullptr, mysql_free_result };
	}
	MYSQL_RES *rows = mysql_store_result(connection);
	if (!rows)
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return { rows, mysql_free_result };
}

rows_ptr stream(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
		return { nullptr, mysql_free_result };
	}
	MYSQL_RES *rows = mysql_use_result(connection);
	if (!rows)
		errno = mysql_errno(connection) ? static_cast<int>(mysql_errno(connection)) : EIO;
	return { rows, mysql_free_result };
}

bool number(const char *text, uint64_t *value)
{
	if (!text || !*text)
		return false;
	const char *end = text + std::strlen(text);
	const auto parsed = std::from_chars(text, end, *value);
	return parsed.ec == std::errc{} && parsed.ptr == end;
}

std::string hex(const uint8_t *bytes, size_t size)
{
	constexpr char digits[] = "0123456789abcdef";
	std::string result(size * 2, '0');
	for (size_t index = 0; index < size; ++index)
	{
		result[2 * index] = digits[bytes[index] >> 4];
		result[2 * index + 1] = digits[bytes[index] & 15];
	}
	return "X'" + result + "'";
}

std::string text_literal(const std::string &value)
{
	return hex(reinterpret_cast<const uint8_t *>(value.data()), value.size());
}

bool transaction(MYSQL *connection, unsigned long session = 0)
{
	if (!connection || !(connection->server_status & SERVER_STATUS_IN_TRANS) ||
	    (session && mysql_thread_id(connection) != session))
		return refuse(ENOTCONN);
	using client_flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	client_flag reconnect = false;
	return !mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect ?
		       true :
		       refuse(EPERM);
}

bool scalar(MYSQL *connection, const std::string &sql, uint64_t *value)
{
	auto rows = query(connection, sql);
	if (!rows)
		return false;
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	return mysql_num_rows(rows.get()) == 1 && row && number(row[0], value) ? true :
										 refuse(EILSEQ);
}

bool schema(MYSQL *connection)
{
	auto rows = query(
		connection,
		"SELECT CONCAT(c.column_name,':',c.data_type,IF(c.column_type LIKE '%unsigned%',"
		"' unsigned',''),':',c.is_nullable),t.engine FROM information_schema.columns c "
		"JOIN information_schema.tables t ON t.table_schema=c.table_schema AND "
		"t.table_name=c.table_name WHERE c.table_schema=DATABASE() AND "
		"c.table_name='sql_room_item_payload' ORDER BY c.ordinal_position");
	if (!rows)
		return false;
	const char *expected[] = {
		"item_uid:bigint unsigned:NO",		"item_revision:bigint unsigned:NO",
		"payload_version:smallint unsigned:NO", "operation_id:binary:NO",
		"season_epoch:bigint unsigned:NO",	"payload:mediumblob:NO"
	};
	if (mysql_num_rows(rows.get()) != std::size(expected))
		return refuse(EPROTONOSUPPORT);
	for (const char *field : expected)
	{
		MYSQL_ROW row = mysql_fetch_row(rows.get());
		if (!row || !row[0] || std::strcmp(field, row[0]) || !row[1] ||
		    std::strcmp(row[1], "InnoDB"))
			return refuse(EPROTONOSUPPORT);
	}
	uint64_t count = 0;
	auto indexes = query(
		connection,
		"SELECT CONCAT(index_name,':',non_unique,':',GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',')) "
		"FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='sql_room_item_payload' "
		"GROUP BY index_name,non_unique ORDER BY index_name");
	if (!indexes || mysql_num_rows(indexes.get()) != 2)
		return refuse(EPROTONOSUPPORT);
	std::set<std::string> expected_indexes = { "PRIMARY:0:item_uid,item_revision",
						   "idx_sql_room_item_operation:1:operation_id" };
	while (MYSQL_ROW index = mysql_fetch_row(indexes.get()))
		if (!index[0] || expected_indexes.erase(index[0]) != 1)
			return refuse(EPROTONOSUPPORT);
	auto checks = query(
		connection,
		"SELECT c.check_clause FROM information_schema.check_constraints c JOIN information_schema.table_constraints t "
		"ON t.constraint_schema=c.constraint_schema AND t.constraint_name=c.constraint_name WHERE t.table_schema=DATABASE() "
		"AND t.table_name='sql_room_item_payload' AND t.constraint_type='CHECK'");
	if (!checks || mysql_num_rows(checks.get()) != 1)
		return refuse(EPROTONOSUPPORT);
	MYSQL_ROW check = mysql_fetch_row(checks.get());
	if (!check || !check[0])
		return refuse(EPROTONOSUPPORT);
	std::string clause;
	for (const char *cursor = check[0]; *cursor; ++cursor)
		if (!std::isspace(static_cast<unsigned char>(*cursor)) && *cursor != '`' &&
		    *cursor != '(' && *cursor != ')')
			clause += static_cast<char>(
				std::tolower(static_cast<unsigned char>(*cursor)));
	// Both byte-length spellings are canonicalized differently by the engines.
	for (size_t at = clause.find("octet_length"); at != std::string::npos;
	     at = clause.find("octet_length", at + 6))
		clause.replace(at, 12, "length");
	if (clause !=
	    "item_uid>0anditem_revision>0andpayload_version=1andseason_epoch>0andlengthpayload>0andlengthpayload<=131072")
		return refuse(EPROTONOSUPPORT);
	return scalar(connection,
		      "SELECT COUNT(*) FROM information_schema.table_constraints WHERE table_schema=DATABASE() "
		      "AND table_name='sql_room_item_payload' AND constraint_type='FOREIGN KEY'",
		      &count) &&
			       count == 1 &&
			       scalar(connection,
				      "SELECT COUNT(*) FROM information_schema.columns WHERE table_schema=DATABASE() "
				      "AND table_name='sql_room_item_payload' AND column_name='operation_id' AND "
				      "character_octet_length=16",
				      &count) &&
			       count == 1 &&
			       scalar(connection,
				      "SELECT COUNT(*) FROM information_schema.statistics WHERE table_schema=DATABASE() "
				      "AND table_name='sql_room_item_payload' AND index_name='PRIMARY' AND non_unique=0 "
				      "AND ((seq_in_index=1 AND column_name='item_uid') OR "
				      "(seq_in_index=2 AND column_name='item_revision'))",
				      &count) &&
			       count == 2 &&
			       scalar(connection,
				      "SELECT COUNT(*) FROM information_schema.key_column_usage k JOIN "
				      "information_schema.referential_constraints r ON r.constraint_schema=k.constraint_schema "
				      "AND r.constraint_name=k.constraint_name WHERE k.table_schema=DATABASE() AND "
				      "k.table_name='sql_room_item_payload' AND k.column_name='operation_id' AND "
				      "k.referenced_table_name='critical_operation_inbox' AND k.referenced_column_name='operation_id' "
				      "AND r.update_rule='RESTRICT' AND r.delete_rule='RESTRICT'",
				      &count) &&
			       count == 1 ?
		       true :
		       refuse(EPROTONOSUPPORT);
}

bool native_metadata(MYSQL *connection, uint64_t item_id, const player_item_snapshot &item)
{
	std::set<std::pair<int16_t, int16_t>> expected_affects;
	for (const auto &affect : item.affects)
		if (affect[0] || affect[1])
			expected_affects.emplace(affect[0], affect[1]);
	auto affects = query(connection,
			     "SELECT location,modifier FROM player_item_affects WHERE item_id=" +
				     std::to_string(item_id) + " LIMIT 5 FOR UPDATE");
	if (!affects)
		return false;
	if (mysql_num_rows(affects.get()) != expected_affects.size())
		return refuse(ESTALE);
	while (MYSQL_ROW row = mysql_fetch_row(affects.get()))
	{
		bool matched = false;
		for (const auto &affect : expected_affects)
			if (row[0] && row[1] && std::to_string(affect.first) == row[0] &&
			    std::to_string(affect.second) == row[1])
			{
				expected_affects.erase(affect);
				matched = true;
				break;
			}
		if (!matched)
			return refuse(ESTALE);
	}
	std::set<std::pair<std::string, std::string>> expected_descriptions;
	for (const auto &description : item.extra_descriptions)
	{
		if (description.keyword.empty() ||
		    description.spellbook != (description.keyword == "SPELLBOOK"))
			return refuse(EBADMSG);
		std::string text = description.description;
		if (description.spellbook)
		{
			// Captured typed spellbooks have an exact compact SQL representation.
			if (!text.empty())
				return refuse(ENOTSUP);
			text = "[";
			std::set<int32_t> seen;
			for (int32_t id : description.spell_ids)
			{
				if (id < 0 || id >= MAX_SKILLS || !seen.insert(id).second)
					return refuse(EBADMSG);
				text += (text.size() == 1 ? "" : ",") + std::to_string(id);
			}
			text += ']';
		}
		else if (!description.spell_ids.empty())
			return refuse(EBADMSG);
		if (!expected_descriptions.emplace(description.keyword, text).second)
			return refuse(EBADMSG);
	}
	if (expected_descriptions.size() > PLAYER_LOAD_ITEM_DESCRIPTION_MAX)
		return refuse(E2BIG);
	auto descriptions = query(
		connection,
		"SELECT SUBSTRING(keyword,1,4097),SUBSTRING(description,1,4097),"
		"OCTET_LENGTH(keyword),OCTET_LENGTH(description) FROM player_item_extra_descr WHERE item_id=" +
			std::to_string(item_id) + " LIMIT 65 FOR UPDATE");
	if (!descriptions)
		return false;
	if (mysql_num_rows(descriptions.get()) != expected_descriptions.size())
		return refuse(ESTALE);
	while (MYSQL_ROW row = mysql_fetch_row(descriptions.get()))
	{
		uint64_t keyword_size = 0, text_size = 0;
		const auto *lengths = mysql_fetch_lengths(descriptions.get());
		if (!row[0] || !row[1] || !lengths || !number(row[2], &keyword_size) ||
		    !number(row[3], &text_size) ||
		    keyword_size > PLAYER_SNAPSHOT_MAX_STRING_BYTES ||
		    text_size > PLAYER_SNAPSHOT_MAX_STRING_BYTES || lengths[0] != keyword_size ||
		    lengths[1] != text_size ||
		    expected_descriptions.erase({ std::string(row[0], lengths[0]),
						  std::string(row[1], lengths[1]) }) != 1)
			return refuse(ESTALE);
	}
	return true;
}
#endif
} // namespace

bool sql_room_item_payload_capture(const item_transfer_payload &payload,
				   sql_room_item_payload_batch *batch)
try
{
	if (!batch || payload.reason != item_transfer_reason::player_drop ||
	    payload.reason_id != static_cast<int64_t>(payload.to_owner.id) ||
	    payload.from_owner.type != item_owner_type::player || !payload.from_owner.id ||
	    payload.from_owner.id > INT32_MAX || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::room || !payload.to_owner.id ||
	    payload.to_owner.id > INT32_MAX || payload.to_owner.context_id || payload.multi_root ||
	    !payload.selected_item_uid || payload.target_parent_item_uid ||
	    (payload.target_root_item_uid &&
	     payload.target_root_item_uid != payload.selected_item_uid) ||
	    payload.expected_target_parent_revision || payload.logical_source_id ||
	    payload.corpse.present || payload.collector.present ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !payload.continuation.data.empty() || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return refuse(ENOTSUP);
	sql_room_item_payload_batch candidate;
	if (player_item_snapshot_list_decode(payload.item_blob.data(), payload.item_blob_size,
					     &candidate.items) !=
		    player_snapshot_codec_result::ok ||
	    candidate.items.size() != payload.item_count)
		return refuse(EBADMSG);
	std::vector<uint8_t> canonical;
	if (player_item_snapshot_list_encode(candidate.items, &canonical) !=
		    player_snapshot_codec_result::ok ||
	    canonical.size() != payload.item_blob_size ||
	    !std::equal(canonical.begin(), canonical.end(), payload.item_blob.begin()))
		return refuse(EBADMSG);
	std::unordered_map<uint64_t, const item_transfer_entry *> entries;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		if (!entry.item_uid || entry.root_item_uid != payload.selected_item_uid ||
		    !entry.expected_item_revision || entry.expected_item_revision == UINT64_MAX ||
		    entry.expected_state != item_custody_state::active ||
		    !entries.emplace(entry.item_uid, &entry).second)
			return refuse(EBADMSG);
	}
	candidate.payloads.reserve(candidate.items.size());
	size_t total_bytes = 0;
	std::set<uint64_t> captured_uids;
	for (size_t index = 0; index < candidate.items.size(); ++index)
	{
		const auto &item = candidate.items[index];
		const auto found = entries.find(item.object_uid);
		if (!exact_item(item) || !captured_uids.insert(item.object_uid).second ||
		    found == entries.end() || found->second->vnum != item.vnum ||
		    (index == 0 ? (item.object_uid != payload.selected_item_uid ||
				   item.parent_index != -1) :
				  (item.parent_index < 0 ||
				   item.parent_index >= static_cast<int32_t>(index))) ||
		    found->second->parent_item_uid !=
			    (index == 0 ? 0 : candidate.items[item.parent_index].object_uid))
			return refuse(EBADMSG);
		std::vector<uint8_t> encoded;
		if (!encode_one(item, &encoded))
			return refuse(EBADMSG);
		if (total_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - encoded.size())
			return refuse(E2BIG);
		total_bytes += encoded.size();
		candidate.payloads.push_back(std::move(encoded));
	}
	*batch = std::move(candidate);
	return true;
}
catch (const std::bad_alloc &)
{
	return refuse(ENOMEM);
}

bool sql_room_item_payload_capture_creation(const zone_reset_item_image &image,
					    sql_room_item_payload_batch *batch)
try
{
	if (!batch || critical_operation_id_is_zero(image.operation_id) ||
	    !economic_source_event_valid(image.reset_source) ||
	    image.reset_source.kind != economic_source_kind::world_generation ||
	    image.reset_source.source.bytes != image.reset_source.generation.bytes ||
	    image.reset_source.source.bytes == image.operation_id.bytes ||
	    image.reset_source.sequence || image.zone_vnum < 0 || image.room_vnum <= 0 ||
	    !image.season_epoch || image.season_epoch == UINT64_MAX ||
	    image.expected_room_revision == UINT64_MAX || image.items.empty())
		return refuse(ENOTSUP);
	if (image.items.size() > ITEM_TRANSFER_MAX_ITEMS ||
	    image.items.size() > CRITICAL_COMMAND_MAX_KEYS - 2 ||
	    image.recipes.size() != image.items.size() || image.coins.size() > image.items.size())
		return refuse(E2BIG);
	std::set<uint64_t> uids;
	for (size_t index = 0; index < image.items.size(); ++index)
	{
		const auto &item = image.items[index];
		// Creation permits the codec's ordinary VNUM0; the old drop predicate
		// remains byte-for-byte unchanged. Artifacts still need their owner.
		if (!item.object_uid || item.object_uid == UINT64_MAX || item.vnum < 0 ||
		    item.type < ITEM_LOWEST || item.type > ITEM_LAST || item.type == ITEM_CORPSE ||
		    (item.extra_flags & ITEM_ARTIFACT) || item.equipment_slot ||
		    item.string_mask !=
			    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
		    !uids.insert(item.object_uid).second ||
		    (index == 0 ? item.parent_index != PLAYER_SNAPSHOT_NO_PARENT :
				  item.parent_index < 0 ||
					  item.parent_index >= static_cast<int32_t>(index)))
			return refuse(EBADMSG);
	}
	std::set<uint64_t> coin_uids;
	for (const auto &coin : image.coins)
	{
		if (!uids.count(coin.item_uid) || !coin_uids.insert(coin.item_uid).second)
			return refuse(EBADMSG);
		const auto item = std::find_if(image.items.begin(), image.items.end(),
					       [&](const auto &entry)
					       { return entry.object_uid == coin.item_uid; });
		if (item->type != ITEM_MONEY || item->vnum != VOBJ_COINS)
			return refuse(EBADMSG);
		for (size_t denomination = 0; denomination < coin.denominations.size();
		     ++denomination)
			if (item->values[denomination] < 0 ||
			    coin.denominations[denomination] != item->values[denomination])
				return refuse(EBADMSG);
		economic_coin_vector delta{};
		if (economic_coin_delta({}, coin.denominations, &delta) !=
		    economic_accounting_error::ok)
			return refuse(EBADMSG);
	}
	for (const auto &item : image.items)
		if ((item.type == ITEM_MONEY) != (item.vnum == VOBJ_COINS) ||
		    (item.type == ITEM_MONEY) != (coin_uids.count(item.object_uid) != 0))
			return refuse(EBADMSG);
	if (!native_mobile_birth_recipe_valid(image.items, image.recipes))
		return refuse(EBADMSG);
	// Validate borrowed strings/row/depth limits before encode_one copies any
	// item. Unlike drop capture, this input did not come from a bounded decoder.
	{
		std::vector<uint8_t> complete;
		const auto status = player_item_snapshot_list_encode(image.items, &complete);
		if (status != player_snapshot_codec_result::ok)
			return refuse(status == player_snapshot_codec_result::allocation_failure ?
					      ENOMEM :
					      EBADMSG);
		if (complete.size() > SQL_ROOM_ITEM_GRAPH_MAX_BYTES)
			return refuse(E2BIG);
	}
	sql_room_item_payload_batch candidate;
	candidate.payloads.reserve(image.items.size());
	size_t total = 0;
	for (const auto &item : image.items)
	{
		std::vector<uint8_t> bytes;
		if (!encode_one(item, &bytes))
			return refuse(EBADMSG);
		if (total > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - bytes.size())
			return refuse(E2BIG);
		total += bytes.size();
		candidate.payloads.push_back(std::move(bytes));
	}
	candidate.items = image.items;
	*batch = std::move(candidate);
	return true;
}
catch (const std::bad_alloc &)
{
	return refuse(ENOMEM);
}

namespace
{
#ifndef __NO_MYSQL__
bool room_creation_command(const critical_command &command, const zone_reset_item_image &image)
{
	zone_reset_item_image original;
	if (zone_reset_item_command_decode(command, &original) != economic_accounting_error::ok)
		return refuse(EILSEQ);
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> actual_source{}, expected_source{};
	std::vector<uint8_t> actual_items, expected_items, actual_recipes, expected_recipes;
	if (economic_source_event_encode(original.reset_source, &actual_source) !=
		    economic_accounting_error::ok ||
	    economic_source_event_encode(image.reset_source, &expected_source) !=
		    economic_accounting_error::ok ||
	    player_item_snapshot_list_encode(original.items, &actual_items) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(image.items, &expected_items) !=
		    player_snapshot_codec_result::ok ||
	    native_mobile_birth_recipe_encode(original.items, original.recipes, &actual_recipes) !=
		    economic_accounting_error::ok ||
	    native_mobile_birth_recipe_encode(image.items, image.recipes, &expected_recipes) !=
		    economic_accounting_error::ok ||
	    original.operation_id.bytes != image.operation_id.bytes ||
	    actual_source != expected_source || original.zone_vnum != image.zone_vnum ||
	    original.room_vnum != image.room_vnum || original.season_epoch != image.season_epoch ||
	    original.expected_room_revision != image.expected_room_revision ||
	    actual_items != expected_items || actual_recipes != expected_recipes ||
	    original.coins.size() != image.coins.size() ||
	    !std::equal(original.coins.begin(), original.coins.end(), image.coins.begin(),
			[](const auto &a, const auto &b)
			{ return a.item_uid == b.item_uid && a.denominations == b.denominations; }))
		return refuse(EILSEQ);
	return true;
}
#endif
}

bool sql_room_item_payload_prepare_creation(MYSQL *connection, const zone_reset_item_image &image,
					    sql_room_item_payload_batch *batch)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)image;
	(void)batch;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!batch)
			return refuse(EINVAL);
		if (!transaction(connection) || !schema(connection))
			return false;
		const unsigned long session = mysql_thread_id(connection);
		sql_room_item_payload_batch candidate;
		if (!sql_room_item_payload_capture_creation(image, &candidate))
			return false;
		// Season exclusion precedes room and item locks. Never manufacture a row
		// or treat an absent room counter as an authoritative zero.
		if (!sql_room_item_payload_lock_season(connection, &candidate.season_epoch))
			return false;
		if (candidate.season_epoch != image.season_epoch)
			return refuse(ESTALE);
		uint64_t revision = 0;
		if (!scalar(connection,
			    "SELECT revision FROM item_owner_revision WHERE owner_type=3 AND owner_id=" +
				    std::to_string(image.room_vnum) +
				    " AND owner_context_id=0 FOR UPDATE",
			    &revision))
			return false;
		if (revision != image.expected_room_revision)
			return refuse(ESTALE);
		for (const auto &item : candidate.items)
		{
			const std::string uid = std::to_string(item.object_uid);
			const std::pair<const char *, const char *> projections[] = {
				{ "item_current_owner",
				  "item_uid=%s OR root_item_uid=%s OR parent_item_uid=%s" },
				{ "item_ownership_ledger",
				  "item_uid=%s OR root_item_uid=%s OR parent_item_uid=%s" },
				{ "sql_room_item_payload", "item_uid=%s" },
				{ "economic_accounting_item_reference", "item_uid=%s" },
				{ "player_items", "obj_uid=%s" },
				{ "saved_items", "obj_uid=%s" },
				{ "player_pet_items", "obj_uid=%s" },
				{ "shopkeeper_items", "obj_uid=%s" },
				{ "siege_items", "obj_uid=%s" },
				{ "locker_items", "obj_uid=%s" },
				{ "account_locker_items", "obj_uid=%s" },
				{ "corpse_items", "obj_uid=%s" },
				{ "auction_item_custody", "item_uid=%s" },
				{ "collector_listings", "item_uid=%s" },
				{ "collector_ledger", "item_uid=%s" },
				{ "player_death_custody",
				  "item_uid=%s OR root_item_uid=%s OR parent_item_uid=%s" },
				{ "player_death_restitution_item",
				  "item_uid=%s OR source_root_item_uid=%s OR source_parent_item_uid=%s OR delivered_root_item_uid=%s OR delivered_parent_item_uid=%s" },
				{ "player_death_restitution_delivery", "item_uid=%s" },
				{ "player_death_restitution_runtime", "item_uid=%s" },
				{ "artifact_mana", "item_uid=%s" },
				{ "item_ownership_quarantine", "item_uid=%s" },
				{ "auction_reconciliation_quarantine", "item_uid=%s" },
				{ "collector_reconciliation_quarantine", "item_uid=%s" }
			};
			for (const auto &[table, predicate] : projections)
			{
				std::string where = predicate;
				for (size_t at = where.find("%s"); at != std::string::npos;
				     at = where.find("%s", at + uid.size()))
					where.replace(at, 2, uid);
				uint64_t count = 0;
				if (!scalar(connection,
					    "SELECT COUNT(*) FROM " + std::string(table) +
						    " WHERE " + where + " FOR UPDATE",
					    &count))
					return false;
				if (count)
					return refuse(EEXIST);
			}
		}
		if (!transaction(connection, session))
			return false;
		// Global reservation/source/operation exclusion belongs to the admitted
		// root. This participant retains only its original locked observations.
		candidate.session_id = session;
		*batch = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_record_creation(MYSQL *connection, const critical_command &command,
					   const zone_reset_item_image &image,
					   sql_room_item_payload_batch *batch)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)image;
	(void)batch;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!batch || !batch->session_id)
			return refuse(EINVAL);
		if (!transaction(connection, batch->session_id))
			return false;
		sql_room_item_payload_batch expected;
		if (!sql_room_item_payload_capture_creation(image, &expected) ||
		    !room_creation_command(command, image))
			return false;
		if (expected.payloads != batch->payloads ||
		    batch->season_epoch != image.season_epoch)
			return refuse(EILSEQ);
		uint64_t epoch = 0, count = 0;
		if (!schema(connection) || !sql_room_item_payload_lock_season(connection, &epoch))
			return false;
		if (epoch != image.season_epoch)
			return refuse(ESTALE);
		const std::string operation =
			hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
		const std::string room = std::to_string(image.room_vnum);
		const std::string root = std::to_string(image.items.front().object_uid);
		const std::string revision = std::to_string(image.expected_room_revision + 1);
		if (!scalar(connection,
			    "SELECT COUNT(*) FROM item_owner_revision WHERE owner_type=3 AND owner_id=" +
				    room + " AND owner_context_id=0 AND revision=" + revision +
				    " FOR UPDATE",
			    &count) ||
		    count != 1 ||
		    !scalar(connection,
			    "SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" + root +
				    " FOR UPDATE",
			    &count) ||
		    count != expected.items.size() ||
		    !scalar(connection,
			    "SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
				    operation + " FOR UPDATE",
			    &count) ||
		    count != expected.items.size())
			return refuse(ESTALE);
		for (size_t index = 0; index < expected.items.size(); ++index)
		{
			const auto &item = expected.items[index];
			const std::string uid = std::to_string(item.object_uid);
			const std::string parent = std::to_string(
				index ? image.items[item.parent_index].object_uid : 0);
			const std::string coin =
				item.type == ITEM_MONEY ?
					" AND coin_payload=" +
						hex(expected.payloads[index].data(),
						    expected.payloads[index].size()) :
					" AND coin_payload IS NULL";
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
					    uid + " AND root_item_uid=" + root +
					    " AND COALESCE(parent_item_uid,0)=" + parent +
					    " AND owner_type=3 AND owner_id=" + room +
					    " AND owner_context_id=0 AND item_revision=1 AND state=1 AND equipment_slot=0 AND vnum=" +
					    std::to_string(item.vnum) + coin + " FOR UPDATE",
				    &count) ||
			    count != 1 ||
			    !scalar(connection,
				    "SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
					    operation +
					    " AND event_index=" + std::to_string(index) +
					    " AND item_uid=" + uid + " AND root_item_uid=" + root +
					    " AND COALESCE(parent_item_uid,0)=" + parent +
					    " AND from_owner_type=0 AND from_owner_id=0 AND from_owner_context_id=0 "
					    "AND to_owner_type=3 AND to_owner_id=" +
					    room +
					    " AND to_owner_context_id=0 AND item_revision=1 AND from_owner_revision=0 AND to_owner_revision=" +
					    revision +
					    " AND reason_type=2 AND reason_id=0 AND source_site=" +
					    std::to_string(static_cast<unsigned>(
						    critical_source_site::zone_event)) +
					    " AND from_equipment_slot=0 AND to_equipment_slot=0 FOR UPDATE",
				    &count) ||
			    count != 1)
				return refuse(ESTALE);
			const std::string key = "item_uid=" + uid + " AND item_revision=1";
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM sql_room_item_payload WHERE " + key +
					    " FOR UPDATE",
				    &count))
				return false;
			const std::string bytes = hex(expected.payloads[index].data(),
						      expected.payloads[index].size());
			if (count)
			{
				if (count != 1 ||
				    !scalar(connection,
					    "SELECT COUNT(*) FROM sql_room_item_payload WHERE " +
						    key +
						    " AND payload_version=1 AND operation_id=" +
						    operation + " AND season_epoch=" +
						    std::to_string(epoch) + " AND payload=" + bytes,
					    &count) ||
				    count != 1)
					return refuse(EEXIST);
			}
			else
			{
				const std::string insert =
					"INSERT INTO sql_room_item_payload(item_uid,item_revision,payload_version,operation_id,season_epoch,payload) VALUES(" +
					uid + ",1,1," + operation + "," + std::to_string(epoch) +
					"," + bytes + ")";
				if (mysql_real_query(connection, insert.data(), insert.size()))
					return refuse(
						mysql_errno(connection) ?
							static_cast<int>(mysql_errno(connection)) :
							EIO);
			}
		}
		return transaction(connection, batch->session_id);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_verify_creation_retained(MYSQL *connection,
						    const critical_command &command,
						    const item_transfer_result &result)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!transaction(connection))
			return false;
		const unsigned long session = mysql_thread_id(connection);
		zone_reset_item_image image;
		sql_room_item_payload_batch expected;
		if (zone_reset_item_command_decode(command, &image) !=
			    economic_accounting_error::ok ||
		    !sql_room_item_payload_capture_creation(image, &expected) ||
		    result.root_item_uid != image.items.front().object_uid ||
		    result.item_count != image.items.size() || result.from_owner_revision ||
		    result.to_owner_revision != image.expected_room_revision + 1 ||
		    result.max_item_revision != 1 || result.corpse_revision ||
		    result.collector_catalog_changed)
			return refuse(EILSEQ);
		std::array<uint8_t, ITEM_TRANSFER_RESULT_BYTES> result_bytes{};
		if (!item_transfer_command_encode_result(result, &result_bytes))
			return refuse(EILSEQ);
		const unsigned int root_status = economic_sql_zone_reset_item_verify_retained(
			connection, command, 0, result_bytes);
		if (root_status)
			return refuse(static_cast<int>(root_status));
		if (!schema(connection))
			return false;
		const std::string operation =
			hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
		uint64_t count = 0;
		if (!scalar(connection,
			    "SELECT COUNT(*) FROM sql_room_item_payload WHERE operation_id=" +
				    operation,
			    &count) ||
		    count != image.items.size() ||
		    !scalar(connection,
			    "SELECT COUNT(*) FROM item_ownership_ledger WHERE operation_id=" +
				    operation,
			    &count) ||
		    count != image.items.size() ||
		    !scalar(connection,
			    "SELECT COUNT(*) FROM economic_accounting_item_reference WHERE operation_id=" +
				    operation,
			    &count) ||
		    count != image.items.size())
			return refuse(EILSEQ);
		for (size_t index = 0; index < image.items.size(); ++index)
		{
			const auto &item = image.items[index];
			const std::string uid = std::to_string(item.object_uid),
					  event = std::to_string(index);
			const std::string parent = std::to_string(
				index ? image.items[item.parent_index].object_uid : 0);
			const std::string bytes = hex(expected.payloads[index].data(),
						      expected.payloads[index].size());
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM sql_room_item_payload WHERE item_uid=" +
					    uid +
					    " AND item_revision=1 AND payload_version=1 AND operation_id=" +
					    operation + " AND season_epoch=" +
					    std::to_string(image.season_epoch) +
					    " AND payload=" + bytes,
				    &count) ||
			    count != 1 ||
			    !scalar(connection,
				    "SELECT COUNT(*) FROM item_ownership_ledger l JOIN economic_accounting_item_reference r "
				    "ON r.legacy_operation_id=l.operation_id AND r.legacy_event_index=l.event_index "
				    "WHERE l.operation_id=" +
					    operation + " AND l.event_index=" + event +
					    " AND l.item_uid=" + uid + " AND l.root_item_uid=" +
					    std::to_string(result.root_item_uid) +
					    " AND COALESCE(l.parent_item_uid,0)=" + parent +
					    " AND l.from_owner_type=0 AND l.from_owner_id=0 AND l.from_owner_context_id=0 "
					    "AND l.to_owner_type=3 AND l.to_owner_id=" +
					    std::to_string(image.room_vnum) +
					    " AND l.to_owner_context_id=0 AND l.item_revision=1 AND l.from_owner_revision=0 AND l.to_owner_revision=" +
					    std::to_string(result.to_owner_revision) +
					    " AND l.reason_type=2 AND l.reason_id=0 AND l.source_site=" +
					    std::to_string(static_cast<unsigned>(
						    critical_source_site::zone_event)) +
					    " AND l.from_equipment_slot=0 AND l.to_equipment_slot=0 AND r.operation_id=" +
					    operation + " AND r.line_index=" + event +
					    " AND r.event_index=" + event +
					    " AND r.child_index=0 AND r.item_uid=" + uid +
					    " AND r.before_revision=0 AND r.after_revision=1",
				    &count) ||
			    count != 1)
				return refuse(EILSEQ);
		}
		return transaction(connection, session);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

namespace
{
#ifndef __NO_MYSQL__
bool room_origin_available(MYSQL *connection, bool *available)
{
	uint64_t count = 0;
	if (!scalar(connection,
		    "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
		    "AND table_name='zone_reset_item_birth_origin' AND table_type='BASE TABLE'",
		    &count))
		return false;
	*available = count != 0;
	return true;
}

// Classify typed22 evidence before checking creation details: malformed/lost
// ledger rows must refuse, never become permission for legacy fallback.
bool room_creation_origin(MYSQL *connection, uint64_t uid, bool *recognized,
			  zone_reset_item_retained_origin *origin)
{
	if (!connection || !uid || !recognized || !origin)
		return refuse(EINVAL);
	bool carrier = false;
	if (!room_origin_available(connection, &carrier))
		return false;
	const std::string item = std::to_string(uid);
	std::string selection =
		"SELECT l.operation_id FROM item_ownership_ledger l JOIN critical_operation_inbox inbox "
		"ON inbox.operation_id=l.operation_id WHERE inbox.command_type=22 AND (l.item_uid=" +
		item + " OR l.root_item_uid=" + item + " OR l.parent_item_uid=" + item +
		") UNION "
		"SELECT r.operation_id FROM economic_accounting_item_reference r JOIN critical_operation_inbox inbox "
		"ON inbox.operation_id=r.operation_id WHERE inbox.command_type=22 AND r.item_uid=" +
		item +
		" UNION "
		"SELECT p.operation_id FROM sql_room_item_payload p JOIN critical_operation_inbox inbox "
		"ON inbox.operation_id=p.operation_id WHERE inbox.command_type=22 AND p.item_uid=" +
		item;
	if (carrier)
		selection +=
			" UNION SELECT birth_operation FROM zone_reset_item_birth_origin WHERE root_item_uid=" +
			item +
			" UNION SELECT o.birth_operation FROM zone_reset_item_birth_origin o JOIN item_current_owner own "
			"ON own.root_item_uid=o.root_item_uid WHERE own.item_uid=" +
			item;
	auto rows = query(connection, selection + " ORDER BY 1 LIMIT 2");
	if (!rows)
		return false;
	if (!mysql_num_rows(rows.get()))
	{
		*recognized = false;
		return true;
	}
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	const auto *lengths = row ? mysql_fetch_lengths(rows.get()) : nullptr;
	critical_operation_id operation{};
	if (mysql_num_rows(rows.get()) != 1 || !row || !lengths || !row[0] ||
	    lengths[0] != operation.bytes.size())
		return refuse(EILSEQ);
	std::memcpy(operation.bytes.data(), row[0], operation.bytes.size());
	rows.reset();
	if (!transaction(connection))
		return false;
	zone_reset_item_retained_origin candidate;
	const int status = zone_reset_item_origin_sql_lock(connection, operation, &candidate);
	if (status)
		return refuse(status);
	item_transfer_result result{};
	if (!candidate.present ||
	    !item_transfer_command_decode_result(candidate.result.data(), candidate.result.size(),
						 &result) ||
	    !sql_room_item_payload_verify_creation_retained(connection, candidate.original, result))
		return refuse(EILSEQ);
	zone_reset_item_image image;
	if (zone_reset_item_command_decode(candidate.original, &image) !=
		    economic_accounting_error::ok ||
	    std::none_of(image.items.begin(), image.items.end(),
			 [&](const auto &entry) { return entry.object_uid == uid; }))
		return refuse(EILSEQ);
	*origin = std::move(candidate);
	*recognized = true;
	return true;
}

bool room_read_creation(MYSQL *connection, uint64_t root_uid, bool *recognized,
			sql_room_item_graph *graph)
{
	uint64_t current_revision = 0;
	if (!scalar(connection,
		    "SELECT item_revision FROM item_current_owner WHERE item_uid=" +
			    std::to_string(root_uid),
		    &current_revision))
		return false;
	if (current_revision != 1)
	{
		*recognized = false; // Later player drops retain their existing reader.
		return true;
	}
	zone_reset_item_retained_origin origin;
	bool found = false;
	if (!room_creation_origin(connection, root_uid, &found, &origin))
		return false;
	if (!found)
	{
		*recognized = false;
		return true;
	}
	zone_reset_item_image image;
	if (zone_reset_item_command_decode(origin.original, &image) !=
		    economic_accounting_error::ok ||
	    image.items.front().object_uid != root_uid)
		return refuse(EILSEQ);
	const unsigned long session = mysql_thread_id(connection);
	std::optional<economic_sql_authority_snapshot> money_book;
	if (!image.coins.empty())
	{
		// This nonlocking locator is only a hint. After the custody locks, any
		// unexpectedly present coin refuses unless the actual book was locked.
		std::string coins;
		for (const auto &coin : image.coins)
			coins += (coins.empty() ? "" : ",") + std::to_string(coin.item_uid);
		uint64_t observed = 0;
		if (!scalar(connection,
			    "SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				    std::to_string(root_uid) + " AND item_uid IN(" + coins + ")",
			    &observed))
			return false;
		if (observed)
		{
			economic_frozen_intent intent;
			if (economic_intent_decode(origin.original.accounting_intent, &intent) !=
			    economic_accounting_error::ok)
				return refuse(EILSEQ);
			const auto &lineage = intent.admission.metadata.lineage;
			// Read the actual current epoch; the birth's retained epoch is history.
			auto epochs = query(
				connection,
				"SELECT active_epoch FROM economic_lineage_state WHERE lineage=" +
					hex(lineage.bytes.data(), lineage.bytes.size()) +
					" LIMIT 2");
			if (!epochs)
				return false;
			MYSQL_ROW row = mysql_fetch_row(epochs.get());
			const auto *lengths = row ? mysql_fetch_lengths(epochs.get()) : nullptr;
			critical_operation_id current_epoch{};
			if (mysql_num_rows(epochs.get()) != 1 || !row || !lengths || !row[0] ||
			    lengths[0] != current_epoch.bytes.size())
				return refuse(ESTALE);
			std::memcpy(current_epoch.bytes.data(), row[0], current_epoch.bytes.size());
			epochs.reset();
			if (critical_operation_id_is_zero(current_epoch))
				return refuse(ESTALE);
			economic_sql_authority_snapshot authority;
			const unsigned int status = economic_sql_lock_authority(
				connection, lineage, current_epoch, {}, &authority);
			if (status)
				return refuse(static_cast<int>(status));
			if (!transaction(connection, session))
				return false;
			money_book = std::move(authority);
		}
	}
	uint64_t epoch = 0, owner_revision = 0;
	if (!sql_room_item_payload_lock_season(connection, &epoch))
		return false;
	if (epoch != image.season_epoch ||
	    !scalar(connection,
		    "SELECT revision FROM item_owner_revision WHERE owner_type=3 AND owner_id=" +
			    std::to_string(image.room_vnum) + " AND owner_context_id=0 FOR UPDATE",
		    &owner_revision) ||
	    owner_revision < image.expected_room_revision + 1)
		return refuse(ESTALE);
	std::unordered_map<uint64_t, size_t> indices;
	for (size_t index = 0; index < image.items.size(); ++index)
		indices.emplace(image.items[index].object_uid, index);
	std::set<uint64_t> present;
	uint64_t coin_bytes = 0;
	if (!scalar(connection,
		    "SELECT COALESCE(SUM(OCTET_LENGTH(coin_payload)),0) FROM item_current_owner WHERE root_item_uid=" +
			    std::to_string(root_uid) + " FOR UPDATE",
		    &coin_bytes))
		return false;
	if (coin_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES)
		return refuse(E2BIG);
	auto rows = query(
		connection,
		"SELECT item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot,"
		"OCTET_LENGTH(coin_payload),SUBSTRING(coin_payload,1,131073) "
		"FROM item_current_owner WHERE root_item_uid=" +
			std::to_string(root_uid) + " ORDER BY item_uid LIMIT " +
			std::to_string(ITEM_TRANSFER_MAX_ITEMS + 1) + " FOR UPDATE");
	if (!rows)
		return false;
	if (!mysql_num_rows(rows.get()) || mysql_num_rows(rows.get()) > image.items.size())
		return refuse(EILSEQ);
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		uint64_t uid = 0;
		if (!number(row[0], &uid) || !indices.count(uid) || !present.insert(uid).second)
			return refuse(EILSEQ);
		const auto &item = image.items[indices.at(uid)];
		const auto *lengths = mysql_fetch_lengths(rows.get());
		if (!lengths)
			return refuse(EILSEQ);
		if (item.type == ITEM_MONEY)
		{
			std::vector<uint8_t> expected_coin;
			uint64_t bytes = 0;
			if (!encode_one(item, &expected_coin) || !number(row[9], &bytes) ||
			    bytes != expected_coin.size() || !row[10] || lengths[10] != bytes ||
			    std::memcmp(row[10], expected_coin.data(), expected_coin.size()))
				return refuse(EILSEQ);
		}
		else if (row[9] || row[10])
			return refuse(EILSEQ); // Ordinary creation never owns a coin payload.
		const std::array<uint64_t, 8> expected = {
			item.parent_index < 0 ? 0 : image.items[item.parent_index].object_uid,
			3,
			static_cast<uint64_t>(image.room_vnum),
			0,
			1,
			static_cast<uint64_t>(item.vnum),
			1,
			0
		};
		for (size_t index = 0; index < expected.size(); ++index)
		{
			uint64_t value = 0;
			if (!number(row[index + 1], &value) || value != expected[index])
				return refuse(EILSEQ);
		}
	}
	rows.reset();
	if (!present.count(root_uid))
		return refuse(EILSEQ);
	for (const auto &coin : image.coins)
	{
		if (!present.count(coin.item_uid))
			continue; // Removed descendants do not need their original birth head.
		if (!money_book)
			return refuse(ESTALE);
		economic_account_key key{ money_book->lineage, economic_account_kind::pile,
					  coin.item_uid, 0 };
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
		if (economic_account_key_encode(key, &encoded) != economic_accounting_error::ok)
			return refuse(EILSEQ);
		const std::string lineage =
			hex(money_book->lineage.bytes.data(), money_book->lineage.bytes.size());
		const std::string current_epoch =
			hex(money_book->epoch.bytes.data(), money_book->epoch.bytes.size());
		// A birth head at revision1 has exactly one effect in the current book.
		// Include malformed/orphan effects instead of filtering them out as if
		// they did not exist. A progressed/opening head requires its own reader.
		auto heads = query(
			connection,
			"SELECT effect.operation_id,effect.after_revision,op.lineage,op.epoch,op.outcome,op.result_code "
			"FROM economic_accounting_account_effect effect LEFT JOIN economic_accounting_operation op "
			"ON op.operation_id=effect.operation_id LEFT JOIN economic_epoch book "
			"ON book.lineage=op.lineage AND book.epoch=op.epoch WHERE effect.account_key=" +
				hex(encoded.data(), encoded.size()) +
				" AND (op.operation_id IS NULL OR book.epoch IS NULL "
				"OR op.lineage<>" +
				lineage + " OR op.outcome<>1 OR op.result_code<>0 OR op.epoch=" +
				current_epoch +
				") ORDER BY effect.after_revision DESC,effect.operation_id,effect.account_index LIMIT 2 LOCK IN SHARE MODE");
		if (!heads)
			return false;
		MYSQL_ROW row = mysql_fetch_row(heads.get());
		const auto *lengths = row ? mysql_fetch_lengths(heads.get()) : nullptr;
		uint64_t revision = 0, outcome = 0, result_code = 0;
		if (mysql_num_rows(heads.get()) != 1 || !row || !lengths || !row[0] ||
		    lengths[0] != origin.original.operation_id.bytes.size() ||
		    std::memcmp(row[0], origin.original.operation_id.bytes.data(), lengths[0]) ||
		    !number(row[1], &revision) || revision != 1 || !row[2] ||
		    lengths[2] != money_book->lineage.bytes.size() ||
		    std::memcmp(row[2], money_book->lineage.bytes.data(), lengths[2]) || !row[3] ||
		    lengths[3] != money_book->epoch.bytes.size() ||
		    std::memcmp(row[3], money_book->epoch.bytes.data(), lengths[3]) ||
		    !number(row[4], &outcome) || outcome != 1 || !number(row[5], &result_code) ||
		    result_code)
			return refuse(ESTALE);
	}
	sql_room_item_graph candidate;
	candidate.owner = { item_owner_type::room, static_cast<uint64_t>(image.room_vnum), 0 };
	candidate.owner_revision = owner_revision;
	std::unordered_map<uint64_t, size_t> positions;
	for (const auto &original : image.items)
	{
		if (!present.count(original.object_uid))
			continue; // A later move can remove complete original descendants.
		const uint64_t parent = original.parent_index < 0 ?
						0 :
						image.items[original.parent_index].object_uid;
		if (parent && !positions.count(parent))
			return refuse(EILSEQ);
		uint64_t count = 0;
		const size_t children =
			std::count_if(image.items.begin(), image.items.end(),
				      [&](const auto &child)
				      {
					      return present.count(child.object_uid) &&
						     child.parent_index >= 0 &&
						     image.items[child.parent_index].object_uid ==
							     original.object_uid;
				      });
		if (!scalar(connection,
			    "SELECT COUNT(*) FROM item_current_owner WHERE parent_item_uid=" +
				    std::to_string(original.object_uid) + " FOR UPDATE",
			    &count) ||
		    count != children)
			return refuse(EILSEQ);
		for (const auto *table :
		     { "saved_items", "player_items", "player_pet_items", "shopkeeper_items",
		       "siege_items", "locker_items", "account_locker_items", "corpse_items" })
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM " + std::string(table) +
					    " WHERE obj_uid=" +
					    std::to_string(original.object_uid) + " FOR UPDATE",
				    &count) ||
			    count)
				return refuse(EEXIST);
		const std::string uid = std::to_string(original.object_uid);
		for (const auto &projection :
		     { "SELECT COUNT(*) FROM auction_item_custody WHERE item_uid=" + uid +
			       " AND claimed_at IS NULL FOR UPDATE",
		       "SELECT COUNT(*) FROM collector_listings WHERE item_uid=" + uid +
			       " AND status IN(2,3) FOR UPDATE",
		       "SELECT COUNT(*) FROM player_death_restitution_runtime WHERE item_uid=" +
			       uid + " FOR UPDATE" })
			if (!scalar(connection, projection, &count) || count)
				return refuse(EEXIST);
		player_item_snapshot item = original;
		item.equipment_slot = -1;
		item.parent_index = parent ? static_cast<int32_t>(positions.at(parent)) :
					     PLAYER_SNAPSHOT_NO_PARENT;
		player_load_item_identity identity;
		identity.database_id = candidate.items.size() + 1;
		identity.serialized_parent_id =
			parent ? static_cast<uint64_t>(item.parent_index) + 1 : 0;
		identity.quantity = 1;
		identity.override_mask = PLAYER_LOAD_ITEM_OVERRIDE_ALL |
					 PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME;
		identity.item_uid = original.object_uid;
		identity.root_item_uid = root_uid;
		identity.parent_item_uid = parent;
		identity.owner = candidate.owner;
		identity.item_revision = 1;
		identity.owner_revision = owner_revision;
		identity.state = item_custody_state::active;
		positions.emplace(original.object_uid, candidate.items.size());
		candidate.items.push_back(std::move(item));
		candidate.identities.push_back(identity);
	}
	std::vector<uint8_t> complete;
	if (player_item_snapshot_list_encode(candidate.items, &complete) !=
		    player_snapshot_codec_result::ok ||
	    !transaction(connection, session))
		return refuse(EILSEQ);
	candidate.creation_origin.emplace(std::move(origin));
	*graph = std::move(candidate);
	*recognized = true;
	return true;
}
#endif
}

bool sql_room_item_payload_prepare(MYSQL *connection, const item_transfer_payload &payload,
				   sql_room_item_payload_batch *batch)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)payload;
	(void)batch;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!transaction(connection) || !schema(connection))
			return false;
		sql_room_item_payload_batch candidate;
		if (!sql_room_item_payload_capture(payload, &candidate))
			return false;
		if (!sql_room_item_payload_lock_season(connection, &candidate.season_epoch))
			return false;
		uint64_t native_count = 0;
		if (!scalar(connection,
			    "SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				    std::to_string(payload.selected_item_uid) + " FOR UPDATE",
			    &native_count) ||
		    native_count != payload.item_count)
			return refuse(ESTALE);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &entry = payload.items[index];
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM item_current_owner WHERE item_uid=" +
					    std::to_string(entry.item_uid) + " AND root_item_uid=" +
					    std::to_string(entry.root_item_uid) +
					    " AND COALESCE(parent_item_uid,0)=" +
					    std::to_string(entry.parent_item_uid) +
					    " AND owner_type=1 AND owner_id=" +
					    std::to_string(payload.from_owner.id) +
					    " AND owner_context_id=0 AND item_revision=" +
					    std::to_string(entry.expected_item_revision) +
					    " AND vnum=" + std::to_string(entry.vnum) +
					    " AND state=1 AND equipment_slot=0 FOR UPDATE",
				    &native_count) ||
			    native_count != 1)
				return refuse(ESTALE);
		}
		std::unordered_map<uint64_t, uint64_t> physical;
		for (size_t index = 0; index < candidate.items.size(); ++index)
		{
			const auto &item = candidate.items[index];
			std::string matches =
				"pi.pid=" + std::to_string(payload.from_owner.id) +
				" AND pi.vnum=" + std::to_string(item.vnum) +
				" AND pi.equip_slot=0 AND pi.quantity=1" +
				" AND pi.weight=" + std::to_string(item.weight) +
				" AND pi.cost=" + std::to_string(item.cost) +
				" AND pi.timer=" + std::to_string(item.timers[0]) +
				" AND pi.extra_flags=" + std::to_string(item.extra_flags) +
				" AND pi.wear_flags=" + std::to_string(item.wear_flags) +
				" AND pi.item_type=" + std::to_string(item.type) +
				" AND pi.item_material=" + std::to_string(item.material) +
				" AND pi.item_condition=" + std::to_string(item.condition);
			for (size_t field = 0; field < item.values.size(); ++field)
				matches += " AND pi.value" + std::to_string(field) + "=" +
					   std::to_string(item.values[field]);
			for (size_t field = 0; field < item.bitvectors.size(); ++field)
				matches += " AND pi.bitvector" + std::to_string(field + 1) + "=" +
					   std::to_string(item.bitvectors[field]);
			matches += " AND BINARY pi.name=" + text_literal(item.name) +
				   " AND BINARY pi.short_descr=" +
				   text_literal(item.short_description) +
				   " AND BINARY pi.description=" + text_literal(item.description) +
				   " AND BINARY pi.action_descr=" +
				   text_literal(item.action_description);
			std::string properties;
			if (player_item_properties_encode(item.extra2_flags, item.dynamic_affects,
							  &properties) !=
			    player_snapshot_codec_result::ok)
				return refuse(EBADMSG);
			matches += " AND BINARY pi.item_properties=" + text_literal(properties);
			auto rows = query(
				connection,
				"SELECT pi.id,COALESCE(pi.container_id,0),IF(" + matches +
					",1,0),OCTET_LENGTH(runtime.payload),SUBSTRING(runtime.payload,1,131073) "
					"FROM player_items pi LEFT JOIN player_item_runtime_state runtime ON runtime.item_id=pi.id "
					"WHERE pi.obj_uid=" +
					std::to_string(item.object_uid) + " LIMIT 2 FOR UPDATE");
			if (!rows)
				return false;
			MYSQL_ROW row = mysql_fetch_row(rows.get());
			const auto *lengths = row ? mysql_fetch_lengths(rows.get()) : nullptr;
			uint64_t id = 0, parent = 0, bytes = 0;
			if (mysql_num_rows(rows.get()) != 1 || !row || !lengths ||
			    !number(row[0], &id) || !id || !number(row[1], &parent) || !row[2] ||
			    std::strcmp(row[2], "1") || !number(row[3], &bytes) ||
			    bytes != candidate.payloads[index].size() || !row[4] ||
			    lengths[4] != bytes ||
			    std::memcmp(row[4], candidate.payloads[index].data(), bytes) ||
			    parent != (index == 0 ? 0 :
						    physical.at(candidate.items[item.parent_index]
									.object_uid)))
				return refuse(ESTALE);
			physical.emplace(item.object_uid, id);
			if (!native_metadata(connection, id, item))
				return false;
		}
		for (const auto &item : candidate.items)
		{
			uint64_t children = 0;
			const size_t expected = std::count_if(
				candidate.items.begin(), candidate.items.end(),
				[&](const auto &child)
				{
					return child.parent_index >= 0 &&
					       candidate.items[child.parent_index].object_uid ==
						       item.object_uid;
				});
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM player_items WHERE container_id=" +
					    std::to_string(physical.at(item.object_uid)) +
					    " FOR UPDATE",
				    &children) ||
			    children != expected)
				return refuse(ESTALE);
			uint64_t duplicates = 0;
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM saved_items WHERE obj_uid=" +
					    std::to_string(item.object_uid) + " FOR UPDATE",
				    &duplicates) ||
			    duplicates)
				return refuse(EEXIST);
		}
		candidate.session_id = mysql_thread_id(connection);
		*batch = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_record(MYSQL *connection, const critical_command &command,
				  const item_transfer_payload &payload,
				  const sql_room_item_payload_batch &batch)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)payload;
	(void)batch;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!transaction(connection, batch.session_id) || !batch.session_id ||
		    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    command.type != critical_command_type::item_transfer)
			return refuse(EINVAL);
		sql_room_item_payload_batch expected;
		if (!sql_room_item_payload_capture(payload, &expected) ||
		    expected.payloads != batch.payloads)
			return refuse(EBADMSG);
		const std::string operation =
			hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
		uint64_t epoch = 0;
		if (!batch.season_epoch || !sql_room_item_payload_lock_season(connection, &epoch) ||
		    epoch != batch.season_epoch)
			return refuse(ESTALE);
		for (size_t index = 0; index < expected.items.size(); ++index)
		{
			const auto &item = expected.items[index];
			const auto found = std::find_if(
				payload.items.begin(), payload.items.begin() + payload.item_count,
				[&](const auto &entry)
				{ return entry.item_uid == item.object_uid; });
			const uint64_t revision = found->expected_item_revision + 1;
			const std::string key = "item_uid=" + std::to_string(item.object_uid) +
						" AND item_revision=" + std::to_string(revision);
			uint64_t count = 0;
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM item_current_owner WHERE " + key +
					    " AND owner_type=3 AND owner_id=" +
					    std::to_string(payload.to_owner.id) +
					    " AND owner_context_id=0 AND state=1 AND equipment_slot=0 AND vnum=" +
					    std::to_string(item.vnum) + " AND root_item_uid=" +
					    std::to_string(payload.selected_item_uid) +
					    " AND COALESCE(parent_item_uid,0)=" +
					    std::to_string(found->parent_item_uid) + " FOR UPDATE",
				    &count) ||
			    count != 1)
				return refuse(ESTALE);
			// An immutable revision row can only be repeated byte-for-byte; never upsert.
			if (!scalar(connection,
				    "SELECT COUNT(*) FROM sql_room_item_payload WHERE " + key +
					    " FOR UPDATE",
				    &count))
				return false;
			const std::string bytes = hex(expected.payloads[index].data(),
						      expected.payloads[index].size());
			if (count)
			{
				if (count != 1 ||
				    !scalar(connection,
					    "SELECT COUNT(*) FROM sql_room_item_payload WHERE " +
						    key +
						    " AND payload_version=1 AND operation_id=" +
						    operation + " AND season_epoch=" +
						    std::to_string(epoch) + " AND payload=" + bytes,
					    &count) ||
				    count != 1)
					return refuse(EEXIST);
			}
			else
			{
				const std::string insert =
					"INSERT INTO sql_room_item_payload(item_uid,item_revision,payload_version,operation_id,season_epoch,payload) VALUES(" +
					std::to_string(item.object_uid) + "," +
					std::to_string(revision) + ",1," + operation + "," +
					std::to_string(epoch) + "," + bytes + ")";
				if (mysql_real_query(connection, insert.data(), insert.size()))
					return refuse(
						mysql_errno(connection) ?
							static_cast<int>(mysql_errno(connection)) :
							EIO);
			}
		}
		return transaction(connection, batch.session_id);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_available(MYSQL *connection, bool *available)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)available;
	return refuse(ENOTSUP);
#else
	if (!connection || !available)
		return refuse(EINVAL);
	uint64_t count = 0;
	if (!scalar(connection,
		    "SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
		    "AND table_name='sql_room_item_payload'",
		    &count) ||
	    count > 1)
		return false;
	if (count && !schema(connection))
		return false;
	*available = count == 1;
	return true;
#endif
}

// Caller owns a successful schema-2 accounting receipt and its transaction.
// Check immutable original-operation evidence without consulting later custody.
bool sql_room_item_payload_verify_retained(MYSQL *connection, const critical_command &command,
					   const item_transfer_result &result)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)command;
	(void)result;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!transaction(connection))
			return false;
		if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
		    !critical_command_envelope_valid(command))
			return refuse(EINVAL);
		const unsigned long session = mysql_thread_id(connection);
		item_transfer_payload payload = {};
		sql_room_item_payload_batch expected;
		if (!item_transfer_command_decode_payload(command, &payload))
			return refuse(EILSEQ);
		if (!sql_room_item_payload_capture(payload, &expected))
			return false;
		if (payload.expected_from_revision == UINT64_MAX ||
		    payload.expected_to_revision == UINT64_MAX ||
		    result.root_item_uid != payload.selected_item_uid ||
		    result.item_count != payload.item_count ||
		    result.from_owner_revision != payload.expected_from_revision + 1 ||
		    result.to_owner_revision != payload.expected_to_revision + 1 ||
		    result.corpse_revision || result.collector_catalog_changed)
			return refuse(EILSEQ);
		if (!schema(connection))
			return false;
		std::unordered_map<uint64_t, size_t> indices;
		std::unordered_map<uint64_t, uint64_t> revisions;
		indices.reserve(expected.items.size());
		revisions.reserve(payload.item_count);
		uint64_t max_revision = 0;
		for (size_t index = 0; index < expected.items.size(); ++index)
			indices.emplace(expected.items[index].object_uid, index);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const auto &item = payload.items[index];
			revisions.emplace(item.item_uid, item.expected_item_revision + 1);
			max_revision = std::max(max_revision, item.expected_item_revision + 1);
		}
		if (result.max_item_revision != max_revision)
			return refuse(EILSEQ);
		const std::string operation =
			hex(command.operation_id.bytes.data(), command.operation_id.bytes.size());
		const std::string limit = std::to_string(payload.item_count + 1);
		uint64_t season = 0;
		size_t count = 0, total_bytes = 0;
		std::set<uint64_t> seen;
		// Lock metadata before fetching any blobs. Early stream disposal may
		// drain unread rows, so establish the aggregate byte bound first.
		{
			auto rows = stream(
				connection,
				"SELECT item_uid,item_revision,payload_version,season_epoch,OCTET_LENGTH(payload) "
				"FROM sql_room_item_payload WHERE operation_id=" +
					operation + " ORDER BY item_uid,item_revision LIMIT " +
					limit + " LOCK IN SHARE MODE");
			if (!rows)
				return false;
			while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
			{
				uint64_t uid = 0, revision = 0, version = 0, row_season = 0,
					 bytes = 0;
				if (++count > payload.item_count || !number(row[0], &uid) ||
				    !number(row[1], &revision) || !number(row[2], &version) ||
				    version != 1 || !number(row[3], &row_season) || !row_season ||
				    (season && season != row_season) || !number(row[4], &bytes) ||
				    !bytes || bytes > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
				    total_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - bytes ||
				    !seen.insert(uid).second)
					return refuse(EILSEQ);
				const auto found = indices.find(uid);
				const auto expected_revision = revisions.find(uid);
				if (found == indices.end() ||
				    expected_revision == revisions.end() ||
				    revision != expected_revision->second ||
				    expected.payloads[found->second].size() != bytes)
					return refuse(EILSEQ);
				season = row_season;
				total_bytes += bytes;
			}
			if (mysql_errno(connection))
				return refuse(static_cast<int>(mysql_errno(connection)));
		}
		if (count != payload.item_count || !transaction(connection, session))
			return count != payload.item_count ? refuse(EILSEQ) : false;
		count = total_bytes = 0;
		seen.clear();
		{
			auto rows = stream(
				connection,
				"SELECT item_uid,item_revision,payload_version,season_epoch,OCTET_LENGTH(payload),"
				"SUBSTRING(payload,1,131073) FROM sql_room_item_payload WHERE operation_id=" +
					operation + " ORDER BY item_uid,item_revision LIMIT " +
					limit + " LOCK IN SHARE MODE");
			if (!rows)
				return false;
			while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
			{
				unsigned long *lengths = mysql_fetch_lengths(rows.get());
				uint64_t uid = 0, revision = 0, version = 0, row_season = 0,
					 bytes = 0;
				if (++count > payload.item_count || !lengths ||
				    !number(row[0], &uid) || !number(row[1], &revision) ||
				    !number(row[2], &version) || version != 1 ||
				    !number(row[3], &row_season) || !row_season ||
				    (season && season != row_season) || !number(row[4], &bytes) ||
				    !bytes || bytes > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
				    !row[5] || lengths[5] != bytes ||
				    total_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - bytes ||
				    !seen.insert(uid).second)
					return refuse(EILSEQ);
				const auto found = indices.find(uid);
				if (found == indices.end())
					return refuse(EILSEQ);
				const auto entry = revisions.find(uid);
				const auto &expected_bytes = expected.payloads[found->second];
				if (entry == revisions.end() || revision != entry->second ||
				    expected_bytes.size() != bytes ||
				    std::memcmp(row[5], expected_bytes.data(),
						expected_bytes.size()))
					return refuse(EILSEQ);
				season = row_season;
				total_bytes += bytes;
			}
			if (mysql_errno(connection))
				return refuse(static_cast<int>(mysql_errno(connection)));
		}
		if (count != payload.item_count)
			return refuse(EILSEQ);
		{
			auto rows = stream(
				connection,
				"SELECT l.event_index,l.item_uid,l.root_item_uid,COALESCE(l.parent_item_uid,0),"
				"l.from_owner_type,l.from_owner_id,l.from_owner_context_id,l.to_owner_type,l.to_owner_id,"
				"l.to_owner_context_id,l.item_revision,l.from_owner_revision,l.to_owner_revision,"
				"l.reason_type,l.reason_id,l.source_site,l.from_equipment_slot,l.to_equipment_slot,"
				"r.operation_id,r.line_index,r.event_index,r.child_index,r.item_uid,r.before_revision,r.after_revision "
				"FROM item_ownership_ledger l LEFT JOIN economic_accounting_item_reference r "
				"ON r.legacy_operation_id=l.operation_id AND r.legacy_event_index=l.event_index "
				"WHERE l.operation_id=" +
					operation + " ORDER BY l.event_index LIMIT " + limit);
			if (!rows)
				return false;
			count = 0;
			while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
			{
				unsigned long *lengths = mysql_fetch_lengths(rows.get());
				if (count >= payload.item_count || !lengths || !row[18] ||
				    lengths[18] != command.operation_id.bytes.size() ||
				    std::memcmp(row[18], command.operation_id.bytes.data(),
						lengths[18]))
					return refuse(EILSEQ);
				const auto &item = payload.items[count];
				const std::array<uint64_t, 24> values = {
					count,
					item.item_uid,
					payload.selected_item_uid,
					item.parent_item_uid,
					static_cast<uint64_t>(payload.from_owner.type),
					payload.from_owner.id,
					0,
					static_cast<uint64_t>(payload.to_owner.type),
					payload.to_owner.id,
					0,
					item.expected_item_revision + 1,
					result.from_owner_revision,
					result.to_owner_revision,
					static_cast<uint64_t>(payload.reason),
					static_cast<uint64_t>(payload.reason_id),
					static_cast<uint64_t>(command.source_site),
					0,
					0,
					count,
					count,
					0,
					item.item_uid,
					item.expected_item_revision,
					item.expected_item_revision + 1
				};
				for (size_t index = 0; index < values.size(); ++index)
				{
					uint64_t actual = 0;
					if (!number(row[index < 18 ? index : index + 1], &actual) ||
					    actual != values[index])
						return refuse(EILSEQ);
				}
				++count;
			}
			if (mysql_errno(connection))
				return refuse(static_cast<int>(mysql_errno(connection)));
		}
		if (count != payload.item_count)
			return refuse(EILSEQ);
		return transaction(connection, session);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_roots(MYSQL *connection, std::vector<uint64_t> *roots)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)roots;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!roots || !transaction(connection) || !schema(connection))
			return false;
		const auto session = mysql_thread_id(connection);
		uint64_t epoch = 0;
		if (!sql_room_item_payload_lock_season(connection, &epoch))
			return false;
		auto rows = query(
			connection,
			"SELECT DISTINCT own.root_item_uid FROM item_current_owner own "
			"WHERE own.owner_type=3 AND own.state=1 AND (EXISTS(SELECT 1 FROM sql_room_item_payload payload "
			"WHERE payload.item_uid=own.item_uid AND payload.season_epoch=" +
				std::to_string(epoch) +
				") OR (NOT EXISTS(SELECT 1 FROM sql_room_item_payload history WHERE history.item_uid=own.item_uid) "
				"AND EXISTS(SELECT 1 FROM item_ownership_ledger ledger JOIN critical_operation_inbox inbox "
				"ON inbox.operation_id=ledger.operation_id JOIN economic_accounting_operation operation "
				"ON operation.operation_id=ledger.operation_id WHERE ledger.item_uid=own.item_uid AND "
				"ledger.root_item_uid=own.root_item_uid AND ledger.to_owner_type=3 AND ledger.to_owner_id=own.owner_id "
				"AND ledger.to_owner_context_id=0 AND ledger.from_owner_type=1 AND ledger.from_owner_id>0 "
				"AND ledger.from_owner_context_id=0 AND ledger.reason_type=" +
				std::to_string(
					static_cast<unsigned>(item_transfer_reason::player_drop)) +
				" AND ledger.reason_id=own.owner_id AND inbox.command_type=5 AND inbox.schema_version=2 "
				"AND inbox.status=1 AND inbox.result_code=0 AND inbox.failure_stage=0 AND operation.outcome=1 "
				"AND operation.result_code=0))) ORDER BY own.root_item_uid LIMIT " +
				std::to_string(SQL_ROOM_ITEM_ROOT_MAX + 1));
		if (!rows)
			return false;
		if (mysql_num_rows(rows.get()) > SQL_ROOM_ITEM_ROOT_MAX)
			return refuse(E2BIG);
		std::vector<uint64_t> candidate;
		while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
		{
			uint64_t uid = 0;
			if (!number(row[0], &uid) || !uid)
				return refuse(EILSEQ);
			candidate.push_back(uid);
		}
		rows.reset();
		// A required creation with a missing literal must remain in recovery's
		// inventory and fail its full typed reader, not disappear into fallback.
		bool carrier = false;
		if (!room_origin_available(connection, &carrier))
			return false;
		std::string selection =
			"SELECT DISTINCT own.root_item_uid FROM item_current_owner own "
			"WHERE own.owner_type=3 AND own.state=1 AND ("
			"EXISTS(SELECT 1 FROM item_ownership_ledger l JOIN critical_operation_inbox inbox "
			"ON inbox.operation_id=l.operation_id WHERE inbox.command_type=22 "
			"AND (l.item_uid=own.item_uid OR l.root_item_uid=own.item_uid OR l.parent_item_uid=own.item_uid)) OR "
			"EXISTS(SELECT 1 FROM economic_accounting_item_reference r JOIN critical_operation_inbox inbox "
			"ON inbox.operation_id=r.operation_id WHERE inbox.command_type=22 AND r.item_uid=own.item_uid) OR "
			"EXISTS(SELECT 1 FROM sql_room_item_payload p JOIN critical_operation_inbox inbox "
			"ON inbox.operation_id=p.operation_id WHERE inbox.command_type=22 AND p.item_uid=own.item_uid)";
		if (carrier)
			selection +=
				" OR EXISTS(SELECT 1 FROM zone_reset_item_birth_origin o "
				"WHERE o.root_item_uid=own.item_uid OR o.root_item_uid=own.root_item_uid)";
		selection += ')';
		if (carrier)
			selection +=
				" UNION SELECT o.root_item_uid FROM zone_reset_item_birth_origin o "
				"JOIN item_current_owner own ON own.item_uid=o.root_item_uid "
				"WHERE own.owner_type=3 AND own.state=1";
		auto created =
			query(connection, selection + " ORDER BY 1 LIMIT " +
						  std::to_string(SQL_ROOM_ITEM_ROOT_MAX + 1));
		if (!created)
			return false;
		if (mysql_num_rows(created.get()) > SQL_ROOM_ITEM_ROOT_MAX)
			return refuse(E2BIG);
		while (MYSQL_ROW row = mysql_fetch_row(created.get()))
		{
			uint64_t uid = 0;
			if (!number(row[0], &uid) || !uid)
				return refuse(EILSEQ);
			candidate.push_back(uid);
		}
		std::sort(candidate.begin(), candidate.end());
		candidate.erase(std::unique(candidate.begin(), candidate.end()), candidate.end());
		if (candidate.size() > SQL_ROOM_ITEM_ROOT_MAX)
			return refuse(E2BIG);
		if (!transaction(connection, session))
			return false;
		*roots = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_classify_creation(MYSQL *connection, uint64_t root_uid,
					     bool *may_need_owner) noexcept
{
	if (may_need_owner)
		*may_need_owner = true;
#ifdef __NO_MYSQL__
	(void)connection;
	(void)root_uid;
	return refuse(ENOTSUP);
#else
	if (!may_need_owner || !root_uid)
		return refuse(EINVAL);
	if (!transaction(connection))
		return false;
	try
	{
		const unsigned long session = mysql_thread_id(connection);
		if (!session || !schema(connection) || !transaction(connection, session))
			return false;
		uint64_t current_revision = 0;
		if (!scalar(connection,
			    "SELECT item_revision FROM item_current_owner WHERE item_uid=" +
				    std::to_string(root_uid),
			    &current_revision))
			return false;
		if (!current_revision)
			return refuse(EILSEQ);
		if (!transaction(connection, session))
			return false;
		if (current_revision != 1)
		{
			*may_need_owner = false; // Later player drops keep their original reader.
			return true;
		}
		bool recognized = true;
		zone_reset_item_retained_origin origin;
		if (!room_creation_origin(connection, root_uid, &recognized, &origin) ||
		    !transaction(connection, session))
			return false;
		if (!recognized)
		{
			*may_need_owner =
				false; // Only complete successful absence selects fallback.
			return true;
		}
		zone_reset_item_image image;
		if (!origin.present ||
		    zone_reset_item_command_decode(origin.original, &image) !=
			    economic_accounting_error::ok ||
		    image.items.empty() || image.items.front().object_uid != root_uid)
			return refuse(EILSEQ);
		// The existing helper authenticates complete immutable root/literal/
		// ledger evidence. This classification is still not current placement
		// or terminal factory-service publication authority.
		return transaction(connection, session);
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
	catch (...)
	{
		return refuse(EIO);
	}
#endif
}

bool sql_room_item_payload_read(MYSQL *connection, uint64_t root_uid, sql_room_item_graph *graph)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)root_uid;
	(void)graph;
	return refuse(ENOTSUP);
#else
	try
	{
		if (!root_uid || !graph || !transaction(connection) || !schema(connection))
			return false;
		bool creation = false;
		sql_room_item_graph created;
		if (!room_read_creation(connection, root_uid, &creation, &created))
			return false;
		if (creation)
		{
			*graph = std::move(created);
			return true;
		}
		const auto session = mysql_thread_id(connection);
		uint64_t epoch = 0;
		if (!sql_room_item_payload_lock_season(connection, &epoch))
			return false;
		// Lock the room owner before custody, matching transfer lock order. Then
		// Stream bounded rows: aggregate preflights may use an older read view.
		uint64_t room = 0, owner_revision = 0, count = 0, budget = 0;
		if (!scalar(connection,
			    "SELECT owner_id FROM item_current_owner WHERE item_uid=" +
				    std::to_string(root_uid) +
				    " AND owner_type=3 AND owner_context_id=0 AND state=1",
			    &room) ||
		    !room || room > INT32_MAX ||
		    !scalar(connection,
			    "SELECT revision FROM item_owner_revision WHERE owner_type=3 AND owner_id=" +
				    std::to_string(room) + " AND owner_context_id=0 FOR UPDATE",
			    &owner_revision) ||
		    !owner_revision ||
		    !scalar(connection,
			    "SELECT COUNT(*) FROM item_current_owner WHERE root_item_uid=" +
				    std::to_string(root_uid),
			    &count) ||
		    !count || count > ITEM_TRANSFER_MAX_ITEMS ||
		    !scalar(connection,
			    "SELECT COALESCE(SUM(OCTET_LENGTH(payload.payload)),0) FROM item_current_owner own "
			    "LEFT JOIN sql_room_item_payload payload ON payload.item_uid=own.item_uid AND "
			    "payload.item_revision=own.item_revision AND payload.season_epoch=" +
				    std::to_string(epoch) +
				    " WHERE own.root_item_uid=" + std::to_string(root_uid),
			    &budget) ||
		    !budget || budget > SQL_ROOM_ITEM_GRAPH_MAX_BYTES)
			return refuse(EILSEQ);
		// LEFT JOIN means a missing descendant payload remains visible and refuses
		// the entire tree. No filtering can silently publish a partial graph.
		auto rows = stream(
			connection,
			"SELECT own.item_uid,own.root_item_uid,COALESCE(own.parent_item_uid,0),"
			"own.owner_type,own.owner_id,own.owner_context_id,own.item_revision,own.vnum,own.state,"
			"own.equipment_slot,revision.revision,payload.payload_version,OCTET_LENGTH(payload.payload),"
			"SUBSTRING(payload.payload,1,131073),"
			"(SELECT COUNT(*) FROM economic_accounting_item_reference ref JOIN item_ownership_ledger ledger "
			"ON ledger.operation_id=ref.legacy_operation_id AND ledger.event_index=ref.legacy_event_index "
			"JOIN critical_operation_inbox inbox ON inbox.operation_id=payload.operation_id "
			"JOIN economic_accounting_operation operation ON operation.operation_id=payload.operation_id "
			"WHERE ref.operation_id=payload.operation_id AND ref.item_uid=own.item_uid AND "
			"inbox.status=1 AND inbox.result_code=0 AND inbox.failure_stage=0 AND "
			"inbox.command_type=5 AND inbox.schema_version=2 AND operation.outcome=1 AND operation.result_code=0 AND "
			"ref.legacy_operation_id=payload.operation_id AND ref.before_revision+1=ref.after_revision AND "
			"ledger.from_owner_type=1 AND ledger.from_owner_id>0 AND ledger.from_owner_context_id=0 AND "
			"ledger.reason_type=" +
				std::to_string(
					static_cast<unsigned>(item_transfer_reason::player_drop)) +
				" AND ledger.reason_id=own.owner_id AND " +
				"ref.after_revision=own.item_revision AND ref.child_index=0 AND ledger.item_uid=own.item_uid AND "
				"ledger.item_revision=own.item_revision AND ledger.root_item_uid=own.root_item_uid AND "
				"COALESCE(ledger.parent_item_uid,0)=COALESCE(own.parent_item_uid,0) AND "
				"ledger.to_owner_type=own.owner_type AND ledger.to_owner_id=own.owner_id AND "
				"ledger.to_owner_context_id=own.owner_context_id),"
				"(SELECT COUNT(*) FROM saved_items saved WHERE saved.obj_uid=own.item_uid) "
				"FROM item_current_owner own LEFT JOIN sql_room_item_payload payload ON "
				"payload.item_uid=own.item_uid AND payload.item_revision=own.item_revision AND payload.season_epoch=" +
				std::to_string(epoch) + " " +
				"LEFT JOIN item_owner_revision revision ON revision.owner_type=own.owner_type AND "
				"revision.owner_id=own.owner_id AND revision.owner_context_id=own.owner_context_id "
				"WHERE own.root_item_uid=" +
				std::to_string(root_uid) + " ORDER BY own.item_uid LIMIT " +
				std::to_string(ITEM_TRANSFER_MAX_ITEMS + 1) + " FOR UPDATE");
		if (!rows)
			return false;
		std::vector<player_item_snapshot> unordered;
		std::vector<player_load_item_identity> identities;
		std::unordered_map<uint64_t, size_t> indices;
		sql_room_item_graph candidate;
		size_t total_bytes = 0;
		while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
		{
			if (unordered.size() >= ITEM_TRANSFER_MAX_ITEMS)
				return refuse(E2BIG);
			const auto *lengths = mysql_fetch_lengths(rows.get());
			player_load_item_identity identity;
			uint64_t owner_type = 0, state = 0, slot = 0, version = 0, bytes = 0,
				 proof = 0, duplicates = 0;
			if (!lengths || !number(row[0], &identity.item_uid) || !identity.item_uid ||
			    !number(row[1], &identity.root_item_uid) ||
			    identity.root_item_uid != root_uid ||
			    !number(row[2], &identity.parent_item_uid) ||
			    !number(row[3], &owner_type) || owner_type != 3 ||
			    !number(row[4], &identity.owner.id) || !identity.owner.id ||
			    identity.owner.id > INT32_MAX ||
			    !number(row[5], &identity.owner.context_id) ||
			    identity.owner.context_id || !number(row[6], &identity.item_revision) ||
			    !identity.item_revision || !number(row[8], &state) || state != 1 ||
			    !number(row[9], &slot) || slot ||
			    !number(row[10], &identity.owner_revision) ||
			    !identity.owner_revision || !number(row[11], &version) ||
			    version != SQL_ROOM_ITEM_PAYLOAD_VERSION || !number(row[12], &bytes) ||
			    !bytes || bytes > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES ||
			    total_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - bytes || !row[13] ||
			    lengths[13] != bytes || !number(row[14], &proof) || proof != 1 ||
			    !number(row[15], &duplicates) || duplicates)
				return refuse(EILSEQ);
			total_bytes += bytes;
			std::vector<player_item_snapshot> decoded;
			if (player_item_snapshot_list_decode(
				    reinterpret_cast<const uint8_t *>(row[13]), bytes, &decoded) !=
				    player_snapshot_codec_result::ok ||
			    decoded.size() != 1 || !exact_item(decoded[0]) ||
			    decoded[0].parent_index != -1 ||
			    decoded[0].object_uid != identity.item_uid || !row[7] ||
			    std::to_string(decoded[0].vnum) != row[7])
				return refuse(EILSEQ);
			std::vector<uint8_t> canonical;
			if (!encode_one(decoded[0], &canonical) || canonical.size() != bytes ||
			    std::memcmp(canonical.data(), row[13], bytes))
				return refuse(EILSEQ);
			identity.owner.type = item_owner_type::room;
			identity.state = item_custody_state::active;
			identity.quantity = 1;
			identity.override_mask = PLAYER_LOAD_ITEM_OVERRIDE_ALL |
						 PLAYER_LOAD_ITEM_OVERRIDE_RUNTIME;
			if (unordered.empty())
			{
				candidate.owner = identity.owner;
				candidate.owner_revision = identity.owner_revision;
			}
			if (!item_owner_identity_equal(candidate.owner, identity.owner) ||
			    candidate.owner_revision != identity.owner_revision ||
			    candidate.owner.id != room ||
			    candidate.owner_revision != owner_revision ||
			    !indices.emplace(identity.item_uid, unordered.size()).second)
				return refuse(EILSEQ);
			unordered.push_back(std::move(decoded[0]));
			identities.push_back(identity);
		}
		if (mysql_errno(connection))
			return refuse(static_cast<int>(mysql_errno(connection)));
		rows.reset();
		const auto root = indices.find(root_uid);
		if (root == indices.end() || identities[root->second].parent_item_uid)
			return refuse(EILSEQ);
		std::vector<std::vector<size_t>> children(unordered.size());
		for (size_t index = 0; index < identities.size(); ++index)
		{
			if (index == root->second)
				continue;
			const auto parent = indices.find(identities[index].parent_item_uid);
			if (parent == indices.end() || parent->second == index)
				return refuse(EILSEQ);
			children[parent->second].push_back(index);
		}
		std::vector<size_t> order = { root->second };
		std::vector<int32_t> positions(unordered.size(), -1);
		std::vector<size_t> depths(unordered.size(), 0);
		depths[root->second] = 1;
		for (size_t cursor = 0; cursor < order.size(); ++cursor)
		{
			const size_t index = order[cursor];
			if (positions[index] != -1 || depths[index] > PLAYER_SNAPSHOT_MAX_DEPTH)
				return refuse(EILSEQ);
			positions[index] = static_cast<int32_t>(cursor);
			auto item = std::move(unordered[index]);
			auto identity = identities[index];
			identity.database_id =
				cursor + 1; // ephemeral materializer witness, never SQL identity
			item.equipment_slot = -1;
			if (identity.parent_item_uid)
			{
				item.parent_index = positions[indices.at(identity.parent_item_uid)];
				if (item.parent_index < 0)
					return refuse(EILSEQ);
				identity.serialized_parent_id =
					static_cast<uint64_t>(item.parent_index) + 1;
			}
			candidate.items.push_back(std::move(item));
			candidate.identities.push_back(identity);
			for (size_t child : children[index])
			{
				depths[child] = depths[index] + 1;
				order.push_back(child);
			}
		}
		if (order.size() != identities.size())
			return refuse(EILSEQ);
		// The complete graph must satisfy the codec's shared row/depth budget too.
		std::vector<uint8_t> complete;
		if (player_item_snapshot_list_encode(candidate.items, &complete) !=
			    player_snapshot_codec_result::ok ||
		    !transaction(connection, session))
			return refuse(EILSEQ);
		*graph = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return refuse(ENOMEM);
	}
#endif
}

bool sql_room_item_payload_lock_season(MYSQL *connection, uint64_t *epoch)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)epoch;
	return refuse(ENOTSUP);
#else
	if (!epoch || !transaction(connection))
		return refuse(EINVAL);
	return scalar(connection,
		      "SELECT season_epoch FROM season_reset_state WHERE state_id=1 "
		      "AND reset_status='active' LOCK IN SHARE MODE",
		      epoch) && *epoch ?
		       true :
		       refuse(ESTALE);
#endif
}

bool sql_room_item_payload_present(MYSQL *connection, uint64_t uid, bool *present)
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)uid;
	(void)present;
	return refuse(ENOTSUP);
#else
	if (!connection || !uid || !present)
		return refuse(EINVAL);
	uint64_t count = 0;
	if (!scalar(connection,
		    "SELECT (EXISTS(SELECT 1 FROM sql_room_item_payload WHERE item_uid=" +
			    std::to_string(uid) +
			    ") OR EXISTS(SELECT 1 FROM item_ownership_ledger ledger "
			    "JOIN critical_operation_inbox inbox ON inbox.operation_id=ledger.operation_id "
			    "JOIN economic_accounting_operation operation ON operation.operation_id=ledger.operation_id "
			    "WHERE ledger.item_uid=" +
			    std::to_string(uid) +
			    " AND ledger.from_owner_type=1 AND "
			    "ledger.from_owner_id>0 AND ledger.from_owner_context_id=0 AND ledger.to_owner_type=3 "
			    "AND ledger.to_owner_context_id=0 AND ledger.reason_type=" +
			    std::to_string(
				    static_cast<unsigned>(item_transfer_reason::player_drop)) +
			    " AND ledger.reason_id=ledger.to_owner_id AND inbox.command_type=5 AND inbox.schema_version=2 "
			    "AND inbox.status=1 AND inbox.result_code=0 AND inbox.failure_stage=0 "
			    "AND operation.outcome=1 AND operation.result_code=0))",
		    &count))
		return false;
	if (!count)
	{
		try
		{
			bool creation = false;
			zone_reset_item_retained_origin origin;
			if (!room_creation_origin(connection, uid, &creation, &origin))
				return false;
			if (creation)
				count = 1; // Full original root and immutable literal proof succeeded.
		}
		catch (const std::bad_alloc &)
		{
			return refuse(ENOMEM);
		}
	}
	*present = count != 0;
	return true;
#endif
}

namespace
{
struct room_source_failure
{
	unsigned int code;
};
void room_require(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw room_source_failure{ code };
}
struct room_source_spec
{
	const char *name, *columns, *order;
};
constexpr room_source_spec room_sources[] = {
	{ "sql_room_item_payload",
	  "item_uid,item_revision,payload_version,operation_id,season_epoch,payload",
	  "item_uid,item_revision" },
	{ "season_reset_state", "state_id,season_epoch,reset_status", "state_id" },
	{ "item_ownership_ledger",
	  "operation_id,event_index,item_uid,root_item_uid,parent_item_uid,from_owner_type,from_owner_id,from_owner_context_id,to_owner_type,to_owner_id,to_owner_context_id,item_revision,from_owner_revision,to_owner_revision,reason_type,reason_id,source_site,from_equipment_slot,to_equipment_slot",
	  "operation_id,event_index" },
	{ "economic_accounting_item_reference",
	  "operation_id,line_index,event_index,child_index,item_uid,before_revision,after_revision,legacy_operation_id,legacy_event_index",
	  "operation_id,line_index" },
	{ "economic_accounting_operation", "operation_id,outcome,result_code", "operation_id" }
};
class room_hash
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{ EVP_MD_CTX_new(),
									 EVP_MD_CTX_free };

    public:
	room_hash()
	{
		room_require(bool(context), ENOMEM);
		room_require(EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) == 1, EIO);
	}
	void bytes(std::span<const uint8_t> value)
	{
		room_require(EVP_DigestUpdate(context.get(), value.data(), value.size()) == 1, EIO);
	}
	void number(uint64_t value)
	{
		std::array<uint8_t, 8> bytes_{};
		for (size_t index = 0; index < bytes_.size(); ++index)
			bytes_[index] = static_cast<uint8_t>(value >> (8 * index));
		bytes(bytes_);
	}
	void text(std::string_view value)
	{
		number(value.size());
		bytes({ reinterpret_cast<const uint8_t *>(value.data()), value.size() });
	}
	economic_sql_source_digest finish()
	{
		economic_sql_source_digest value{};
		unsigned int length = 0;
		room_require(EVP_DigestFinal_ex(context.get(), value.data(), &length) == 1 &&
				     length == value.size(),
			     EIO);
		return value;
	}
};
void room_add(uint64_t &used, uint64_t extra, uint64_t maximum)
{
	room_require(used <= maximum && extra <= maximum - used, E2BIG);
	used += extra;
}
std::vector<std::string> room_columns(std::string_view input)
{
	std::vector<std::string> output;
	while (!input.empty())
	{
		const size_t end = input.find(',');
		output.emplace_back(input.substr(0, end));
		if (end == std::string_view::npos)
			break;
		input.remove_prefix(end + 1);
	}
	return output;
}
economic_sql_source_digest room_definition(const room_source_spec &spec)
{
	room_hash hash;
	hash.text("ESD1");
	hash.text(spec.name);
	hash.text(spec.order);
	const auto columns = room_columns(spec.columns);
	hash.number(columns.size());
	for (const auto &column : columns)
		hash.text(column);
	return hash.finish();
}
economic_sql_source_digest room_row_digest(const economic_sql_source_table &table,
					   const economic_sql_source_row &row)
{
	room_hash hash;
	hash.text("ESR1");
	hash.bytes(table.definition_digest);
	for (const auto &cell : row.cells)
	{
		hash.number(cell ? 1 : 0);
		if (cell)
			hash.text(*cell);
	}
	return hash.finish();
}
struct room_budget
{
	uint64_t rows, cells, bytes;
};
room_budget room_validate(const economic_sql_physical_source_snapshot &base,
			  const std::vector<economic_sql_source_table> &tables,
			  const economic_sql_source_limits &limits, size_t diagnostics)
{
	room_require(diagnostics && diagnostics <= 512, EINVAL);
	const auto code = economic_sql_validate_physical_sources(base, limits);
	room_require(!code, code);
	room_require(tables.size() == std::size(room_sources));
	room_budget budget{ base.rows, base.cells, base.cell_bytes };
	for (size_t index = 0; index < tables.size(); ++index)
	{
		const auto &table = tables[index];
		const auto &spec = room_sources[index];
		room_require(table.name == spec.name &&
			     table.columns == room_columns(spec.columns) &&
			     table.definition_digest == room_definition(spec));
		room_hash content;
		content.text("EST1");
		content.bytes(table.definition_digest);
		content.number(table.rows.size());
		room_add(budget.rows, table.rows.size(), limits.maximum_rows);
		for (const auto &row : table.rows)
		{
			room_require(row.cells.size() == table.columns.size());
			room_add(budget.cells, row.cells.size(), limits.maximum_cells);
			for (const auto &cell : row.cells)
				if (cell)
				{
					room_require(cell->size() <=
							     limits.maximum_single_cell_bytes,
						     E2BIG);
					room_add(budget.bytes, cell->size(),
						 limits.maximum_cell_bytes);
				}
			room_require(row.digest == room_row_digest(table, row));
			content.bytes(row.digest);
		}
		room_require(table.content_digest == content.finish());
	}
	return budget;
}
const economic_sql_source_table &
room_base_table(const std::vector<economic_sql_source_table> &tables, const char *name)
{
	const auto found = std::find_if(tables.begin(), tables.end(),
					[&](const auto &table) { return table.name == name; });
	room_require(found != tables.end());
	return *found;
}
bool room_number(const economic_sql_source_row &row, size_t index, uint64_t &value,
		 bool nullable_zero = false)
{
	if (index >= row.cells.size() || !row.cells[index])
	{
		value = 0;
		return nullable_zero && index < row.cells.size();
	}
	const auto &text = *row.cells[index];
	const auto parsed = std::from_chars(text.data(), text.data() + text.size(), value);
	return !text.empty() && parsed.ec == std::errc{} && parsed.ptr == text.data() + text.size();
}
bool room_equal_number(const economic_sql_source_row &row, size_t index, uint64_t value,
		       bool nullable_zero = false)
{
	uint64_t number_ = 0;
	return room_number(row, index, number_, nullable_zero) && number_ == value;
}
using room_key = std::pair<uint64_t, uint64_t>;
using room_operation_key = std::pair<std::string, uint64_t>;
using room_index = std::map<room_key, std::vector<size_t>>;
using room_operations = std::map<std::string, std::vector<size_t>>;
room_operations room_operation_index(const economic_sql_source_table &table)
{
	room_operations output;
	for (size_t index = 0; index < table.rows.size(); ++index)
		if (!table.rows[index].cells.empty() && table.rows[index].cells[0] &&
		    table.rows[index].cells[0]->size() == 16)
			output[*table.rows[index].cells[0]].push_back(index);
	return output;
}
bool room_success(const std::string &operation, const economic_sql_source_table &inbox,
		  const room_operations &inboxes, const economic_sql_source_table &accounting,
		  const room_operations &operations)
{
	const auto native = inboxes.find(operation), economic = operations.find(operation);
	if (native == inboxes.end() || economic == operations.end() || native->second.size() != 1 ||
	    economic->second.size() != 1)
		return false;
	const auto &receipt = inbox.rows[native->second[0]],
		   &account = accounting.rows[economic->second[0]];
	return room_equal_number(receipt, 3, 5) && room_equal_number(receipt, 4, 2) &&
	       room_equal_number(receipt, 6, 1) && room_equal_number(receipt, 7, 0) &&
	       room_equal_number(receipt, 8, 0) && room_equal_number(account, 1, 1) &&
	       room_equal_number(account, 2, 0);
}
bool room_drop(const economic_sql_source_row &ledger, uint64_t uid, uint64_t root, uint64_t room)
{
	uint64_t player = 0;
	return ledger.cells[0] && ledger.cells[0]->size() == 16 &&
	       room_equal_number(ledger, 2, uid) && room_equal_number(ledger, 3, root) &&
	       room_equal_number(ledger, 5, 1) && room_number(ledger, 6, player) && player &&
	       room_equal_number(ledger, 7, 0) && room_equal_number(ledger, 8, 3) &&
	       room_equal_number(ledger, 9, room) && room_equal_number(ledger, 10, 0) &&
	       room_equal_number(ledger, 14,
				 static_cast<uint64_t>(item_transfer_reason::player_drop)) &&
	       room_equal_number(ledger, 15, room);
}
void room_inspect(const economic_sql_physical_source_snapshot &base,
		  const std::vector<economic_sql_source_table> &tables, size_t maximum_diagnostics,
		  sql_room_item_source_evidence &evidence)
{
	const auto &payloads = tables[0], &season = tables[1], &ledger = tables[2],
		   &references = tables[3], &accounting = tables[4];
	const auto &custody = room_base_table(base.source2.tables, "item_current_owner");
	const auto &equipment = base.source2.item_equipment_sources[0];
	const auto &owners = room_base_table(base.source2.tables, "item_owner_revision");
	const auto &inbox = room_base_table(base.source2.tables, "critical_operation_inbox");
	const auto &saved = room_base_table(base.physical_sources, "saved_items");
	const auto inboxes = room_operation_index(inbox),
		   operations = room_operation_index(accounting);
	std::map<uint64_t, std::vector<size_t>> custody_by_uid, payload_by_uid, ledger_by_uid;
	room_index payload_by_revision, ledger_by_revision;
	std::map<room_operation_key, std::vector<size_t>> references_by_event;
	std::map<room_key, std::vector<size_t>> owner_by_identity;
	std::map<uint64_t, size_t> saved_by_uid;
	auto finding = [&](size_t index, uint32_t flags)
	{
		if (!flags)
			return;
		if (evidence.diagnostics.size() < maximum_diagnostics)
			evidence.diagnostics.push_back({ flags, index });
		else
			evidence.diagnostics_truncated = true;
	};
	if (season.rows.size() == 1 && room_equal_number(season.rows[0], 0, 1) &&
	    room_number(season.rows[0], 1, evidence.current_season) && evidence.current_season &&
	    season.rows[0].cells[2] && *season.rows[0].cells[2] == "active")
		evidence.season_active = true;
	else
		finding(SIZE_MAX, SQL_ROOM_SOURCE_MALFORMED_IDENTITY);
	for (size_t index = 0; index < custody.rows.size(); ++index)
	{
		uint64_t uid = 0;
		if (room_number(custody.rows[index], 0, uid) && uid)
			custody_by_uid[uid].push_back(index);
	}
	for (size_t index = 0; index < owners.rows.size(); ++index)
	{
		uint64_t room = 0;
		if (room_equal_number(owners.rows[index], 0, 3) &&
		    room_number(owners.rows[index], 1, room) &&
		    room_equal_number(owners.rows[index], 2, 0))
			owner_by_identity[{ room, 0 }].push_back(index);
	}
	for (const auto &row : saved.rows)
	{
		uint64_t uid = 0;
		if (room_number(row, 4, uid) && uid)
			++saved_by_uid[uid];
	}
	for (size_t index = 0; index < ledger.rows.size(); ++index)
	{
		uint64_t uid = 0, revision = 0;
		if (room_number(ledger.rows[index], 2, uid) && uid &&
		    room_number(ledger.rows[index], 11, revision))
		{
			ledger_by_uid[uid].push_back(index);
			ledger_by_revision[{ uid, revision }].push_back(index);
		}
	}
	for (size_t index = 0; index < references.rows.size(); ++index)
	{
		const auto &row = references.rows[index];
		uint64_t event = 0;
		if (row.cells[7] && row.cells[7]->size() == 16 && room_number(row, 8, event))
			references_by_event[{ *row.cells[7], event }].push_back(index);
	}
	for (size_t index = 0; index < payloads.rows.size(); ++index)
	{
		uint64_t uid = 0, revision = 0;
		if (room_number(payloads.rows[index], 0, uid) && uid &&
		    room_number(payloads.rows[index], 1, revision) && revision)
		{
			payload_by_uid[uid].push_back(index);
			payload_by_revision[{ uid, revision }].push_back(index);
		}
	}
	auto proof = [&](sql_room_item_source_witness &witness, const std::string &operation,
			 bool current)
	{
		size_t count = 0;
		const auto entries =
			ledger_by_revision.find({ witness.item_uid, witness.item_revision });
		if (entries == ledger_by_revision.end())
			return count;
		for (const size_t ledger_index : entries->second)
		{
			const auto &entry = ledger.rows[ledger_index];
			uint64_t root = 0, room = 0, event = 0, parent = 0;
			if (!entry.cells[0] || *entry.cells[0] != operation ||
			    !room_number(entry, 1, event) || !room_number(entry, 3, root) ||
			    !root || !room_number(entry, 9, room) || !room ||
			    !room_number(entry, 4, parent, true) ||
			    !room_drop(entry, witness.item_uid, root, room) ||
			    !room_success(operation, inbox, inboxes, accounting, operations) ||
			    (current &&
			     (root != witness.root_item_uid || parent != witness.parent_item_uid ||
			      room != witness.room)))
				continue;
			const auto bindings = references_by_event.find({ operation, event });
			if (bindings == references_by_event.end())
				continue;
			for (const size_t reference_index : bindings->second)
			{
				const auto &binding = references.rows[reference_index];
				uint64_t before = 0;
				if (!binding.cells[0] || *binding.cells[0] != operation ||
				    !room_equal_number(binding, 3, 0) ||
				    !room_equal_number(binding, 4, witness.item_uid) ||
				    !room_number(binding, 5, before) || before == UINT64_MAX ||
				    !room_equal_number(binding, 6, before + 1) ||
				    before + 1 != witness.item_revision)
					continue;
				++count;
				witness.ledger_row = ledger_index;
				witness.reference_row = reference_index;
				if (!current)
				{
					witness.root_item_uid = root;
					witness.parent_item_uid = parent;
					witness.room = room;
				}
			}
		}
		return count;
	};
	std::map<uint64_t, size_t> current_witness;
	auto inspect_current = [&](sql_room_item_source_witness &witness,
				   const economic_sql_source_row &owner, size_t custody_index)
	{
		witness.custody_row = custody_index;
		uint64_t revision = 0;
		if (!room_number(owner, 1, witness.root_item_uid) || !witness.root_item_uid ||
		    !room_number(owner, 2, witness.parent_item_uid, true) ||
		    !room_equal_number(owner, 3, 3) || !room_number(owner, 4, witness.room) ||
		    !witness.room || witness.room > INT32_MAX || !room_equal_number(owner, 5, 0) ||
		    !room_number(owner, 6, revision) || !revision ||
		    !room_equal_number(owner, 8, 1) ||
		    !room_equal_number(equipment.rows[custody_index], 1, 0))
			witness.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
		const auto owner_revision = owner_by_identity.find({ witness.room, 0 });
		if (owner_revision == owner_by_identity.end() ||
		    owner_revision->second.size() != 1 ||
		    !room_number(owners.rows[owner_revision->second[0]], 3,
				 witness.owner_revision) ||
		    !witness.owner_revision)
			witness.flags |= SQL_ROOM_SOURCE_MISSING_OWNER_REVISION;
		if (saved_by_uid.count(witness.item_uid))
			witness.flags |= SQL_ROOM_SOURCE_COMPETING_LEGACY;
		if (witness.literal && (revision != witness.item_revision || !owner.cells[7] ||
					*owner.cells[7] != std::to_string(witness.literal->vnum)))
			witness.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
	};
	for (size_t index = 0; index < payloads.rows.size(); ++index)
	{
		const auto &row = payloads.rows[index];
		sql_room_item_source_witness witness;
		witness.payload_row = index;
		uint64_t version = 0;
		if (!room_number(row, 0, witness.item_uid) || !witness.item_uid ||
		    !room_number(row, 1, witness.item_revision) || !witness.item_revision ||
		    !room_number(row, 2, version) || version != SQL_ROOM_ITEM_PAYLOAD_VERSION ||
		    !room_number(row, 4, witness.season_epoch) || !witness.season_epoch ||
		    !row.cells[3] || row.cells[3]->size() != 16)
			witness.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
		if (!row.cells[5] || row.cells[5]->empty() ||
		    row.cells[5]->size() > ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES)
			witness.flags |= SQL_ROOM_SOURCE_MALFORMED_LITERAL;
		else
		{
			std::vector<player_item_snapshot> decoded;
			std::vector<uint8_t> canonical;
			const auto &bytes = *row.cells[5];
			const auto decoding = player_item_snapshot_list_decode(
				reinterpret_cast<const uint8_t *>(bytes.data()), bytes.size(),
				&decoded);
			if (decoding == player_snapshot_codec_result::allocation_failure)
				throw room_source_failure{ ENOMEM };
			bool literal = decoding == player_snapshot_codec_result::ok &&
				       decoded.size() == 1 && exact_item(decoded[0]) &&
				       decoded[0].parent_index == -1 &&
				       decoded[0].object_uid == witness.item_uid;
			if (literal)
			{
				const auto encoding =
					player_item_snapshot_list_encode(decoded, &canonical);
				if (encoding == player_snapshot_codec_result::allocation_failure)
					throw room_source_failure{ ENOMEM };
				literal =
					encoding == player_snapshot_codec_result::ok &&
					!canonical.empty() &&
					canonical.size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES &&
					canonical.size() == bytes.size() &&
					!std::memcmp(canonical.data(), bytes.data(), bytes.size());
			}
			if (!literal)
				witness.flags |= SQL_ROOM_SOURCE_MALFORMED_LITERAL;
			else
				witness.literal = std::move(decoded[0]);
		}
		if (witness.season_epoch != evidence.current_season)
			witness.flags |= SQL_ROOM_SOURCE_OLD_SEASON;
		const auto owned = custody_by_uid.find(witness.item_uid);
		bool current = false;
		if (owned == custody_by_uid.end())
			witness.flags |= SQL_ROOM_SOURCE_MISSING_CUSTODY;
		else if (owned->second.size() != 1)
			witness.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
		else
		{
			const size_t custody_index = owned->second[0];
			const auto &owner = custody.rows[custody_index];
			witness.custody_row = custody_index;
			current = evidence.season_active && room_equal_number(owner, 3, 3) &&
				  room_equal_number(owner, 8, 1) &&
				  room_equal_number(owner, 6, witness.item_revision) &&
				  witness.season_epoch == evidence.current_season;
			if (current)
				inspect_current(witness, owner, custody_index);
			else if (!room_equal_number(owner, 3, 3))
				witness.flags |= SQL_ROOM_SOURCE_MOVED_FROM_ROOM;
		}
		witness.flags |= current ? SQL_ROOM_SOURCE_CURRENT : SQL_ROOM_SOURCE_HISTORY;
		if (payload_by_revision[{ witness.item_uid, witness.item_revision }].size() > 1)
			witness.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
		const size_t proofs = row.cells[3] && row.cells[3]->size() == 16 ?
					      proof(witness, *row.cells[3], current) :
					      0;
		if (proofs != 1)
			witness.flags |= proofs ? SQL_ROOM_SOURCE_AMBIGUOUS_PROOF :
						  SQL_ROOM_SOURCE_MISSING_PROOF;
		if (current)
			current_witness[witness.item_uid] = evidence.witnesses.size();
		finding(evidence.witnesses.size(),
			witness.flags & ~(SQL_ROOM_SOURCE_CURRENT | SQL_ROOM_SOURCE_HISTORY));
		evidence.witnesses.push_back(std::move(witness));
	}
	// Scan native provenance independently: all literal payloads may be gone.
	// The fallback intentionally does not bind ledger revision; exact current
	// correspondence remains a separate finding, as in roots()/read().
	std::set<uint64_t> family_roots;
	for (size_t index = 0; index < custody.rows.size(); ++index)
	{
		const auto &owner = custody.rows[index];
		uint64_t uid = 0, root = 0, room = 0, revision = 0;
		if (!room_number(owner, 0, uid) || !uid || !room_number(owner, 1, root) || !root ||
		    !room_equal_number(owner, 3, 3) || !room_number(owner, 4, room) || !room ||
		    !room_equal_number(owner, 5, 0) || !room_equal_number(owner, 8, 1) ||
		    !room_number(owner, 6, revision))
			continue;
		bool family = payload_by_uid.count(uid) != 0;
		const auto entries = ledger_by_uid.find(uid);
		if (!family && entries != ledger_by_uid.end())
			for (const size_t ledger_index : entries->second)
			{
				const auto &entry = ledger.rows[ledger_index];
				if (room_drop(entry, uid, root, room) &&
				    room_success(*entry.cells[0], inbox, inboxes, accounting,
						 operations))
				{
					family = true;
					break;
				}
			}
		if (!family)
			continue; // unrelated legitimate legacy baseline family
		family_roots.insert(root);
	}
	std::map<uint64_t, std::vector<size_t>> graph_members;
	for (size_t index = 0; index < custody.rows.size(); ++index)
	{
		const auto &owner = custody.rows[index];
		uint64_t uid = 0, root = 0, revision = 0;
		if (!room_number(owner, 1, root) || !family_roots.count(root))
			continue;
		if (!room_number(owner, 0, uid) || !uid || !room_number(owner, 6, revision) ||
		    !revision)
		{
			sql_room_item_source_witness malformed;
			malformed.item_uid = uid;
			malformed.item_revision = revision;
			malformed.flags = SQL_ROOM_SOURCE_CURRENT |
					  SQL_ROOM_SOURCE_MALFORMED_IDENTITY |
					  SQL_ROOM_SOURCE_MISSING_LITERAL;
			inspect_current(malformed, owner, index);
			finding(evidence.witnesses.size(),
				malformed.flags & ~SQL_ROOM_SOURCE_CURRENT);
			graph_members[root].push_back(evidence.witnesses.size());
			evidence.witnesses.push_back(std::move(malformed));
			continue;
		}
		// ALL custody descendants participate, even if their literal and
		// provenance are missing or their placement conflicts with the root.
		auto represented = current_witness.find(uid);
		if (represented == current_witness.end())
		{
			sql_room_item_source_witness missing;
			missing.item_uid = uid;
			missing.item_revision = revision;
			missing.flags = SQL_ROOM_SOURCE_CURRENT | SQL_ROOM_SOURCE_MISSING_LITERAL;
			inspect_current(missing, owner, index);
			const auto entries = ledger_by_uid.find(uid);
			size_t proofs = 0;
			if (entries != ledger_by_uid.end())
				for (const size_t ledger_index : entries->second)
				{
					const auto &entry = ledger.rows[ledger_index];
					if (!room_drop(entry, uid, root, missing.room) ||
					    !room_success(*entry.cells[0], inbox, inboxes,
							  accounting, operations))
						continue;
					if (!room_equal_number(entry, 11, revision))
						missing.flags |= SQL_ROOM_SOURCE_STALE_PROVENANCE;
					else
						proofs += proof(missing, *entry.cells[0], true);
				}
			if (proofs != 1)
				missing.flags |= proofs ? SQL_ROOM_SOURCE_AMBIGUOUS_PROOF :
							  SQL_ROOM_SOURCE_MISSING_PROOF;
			finding(evidence.witnesses.size(),
				missing.flags & ~SQL_ROOM_SOURCE_CURRENT);
			const size_t witness_index = evidence.witnesses.size();
			evidence.witnesses.push_back(std::move(missing));
			current_witness[uid] = witness_index;
			represented = current_witness.find(uid);
		}
		graph_members[root].push_back(represented->second);
	}
	// Native successful-drop provenance is also retained when both the literal
	// and current custody have disappeared, or custody has moved to a player.
	// These witnesses are history/gaps, never current room occupancy.
	std::map<room_key, size_t> missing_history;
	for (size_t index = 0; index < ledger.rows.size(); ++index)
	{
		const auto &entry = ledger.rows[index];
		uint64_t uid = 0, root = 0, room = 0, revision = 0;
		if (!room_number(entry, 2, uid) || !uid || !room_number(entry, 3, root) || !root ||
		    !room_number(entry, 9, room) || !room || !room_number(entry, 11, revision) ||
		    !revision || !room_drop(entry, uid, root, room) ||
		    !room_success(*entry.cells[0], inbox, inboxes, accounting, operations))
			continue;
		const auto retained = payload_by_revision.find({ uid, revision });
		if (retained != payload_by_revision.end() &&
		    std::any_of(retained->second.begin(), retained->second.end(), [&](size_t row)
				{ return payloads.rows[row].cells[3] == entry.cells[0]; }))
			continue;
		const auto current = current_witness.find(uid);
		if (current != current_witness.end() &&
		    evidence.witnesses[current->second].item_revision == revision &&
		    (evidence.witnesses[current->second].flags & SQL_ROOM_SOURCE_MISSING_LITERAL))
			continue;
		const auto duplicate = missing_history.find({ uid, revision });
		if (duplicate != missing_history.end())
		{
			evidence.witnesses[duplicate->second].flags |=
				SQL_ROOM_SOURCE_AMBIGUOUS_PROOF;
			finding(duplicate->second, SQL_ROOM_SOURCE_AMBIGUOUS_PROOF);
			continue;
		}
		sql_room_item_source_witness missing;
		missing.item_uid = uid;
		missing.item_revision = revision;
		missing.root_item_uid = root;
		missing.room = room;
		missing.ledger_row = index;
		(void)room_number(entry, 4, missing.parent_item_uid, true);
		missing.flags = SQL_ROOM_SOURCE_HISTORY | SQL_ROOM_SOURCE_MISSING_LITERAL;
		const size_t proofs = proof(missing, *entry.cells[0], false);
		if (proofs != 1)
			missing.flags |= proofs ? SQL_ROOM_SOURCE_AMBIGUOUS_PROOF :
						  SQL_ROOM_SOURCE_MISSING_PROOF;
		const auto owned = custody_by_uid.find(uid);
		if (owned == custody_by_uid.end())
			missing.flags |= SQL_ROOM_SOURCE_MISSING_CUSTODY;
		else if (owned->second.size() != 1)
			missing.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
		else
		{
			missing.custody_row = owned->second[0];
			if (!room_equal_number(custody.rows[missing.custody_row], 3, 3))
				missing.flags |= SQL_ROOM_SOURCE_MOVED_FROM_ROOM;
		}
		finding(evidence.witnesses.size(), missing.flags & ~SQL_ROOM_SOURCE_HISTORY);
		missing_history[{ uid, revision }] = evidence.witnesses.size();
		evidence.witnesses.push_back(std::move(missing));
	}
	if (graph_members.size() > SQL_ROOM_ITEM_ROOT_MAX)
	{
		evidence.native_root_limit_exceeded = true;
		finding(SIZE_MAX, SQL_ROOM_SOURCE_NATIVE_ROOT_LIMIT);
	}
	for (const auto &[root, members] : graph_members)
	{
		sql_room_item_source_graph graph;
		graph.root_item_uid = root;
		if (members.size() > ITEM_TRANSFER_MAX_ITEMS)
		{
			graph.witness_indices = members;
			for (const size_t member : members)
			{
				evidence.witnesses[member].flags |= SQL_ROOM_SOURCE_BAD_GRAPH;
				finding(member, SQL_ROOM_SOURCE_BAD_GRAPH);
			}
			evidence.graphs.push_back(std::move(graph));
			continue;
		}
		bool valid = evidence.season_active && members.size() <= ITEM_TRANSFER_MAX_ITEMS;
		std::map<uint64_t, size_t> positions;
		std::vector<std::vector<size_t>> children(members.size());
		std::vector<size_t> depths(members.size(), 0), order;
		uint64_t total_bytes = 0;
		for (size_t index = 0; index < members.size(); ++index)
		{
			const auto &witness = evidence.witnesses[members[index]];
			valid &= positions.emplace(witness.item_uid, index).second;
			valid &= witness.flags == SQL_ROOM_SOURCE_CURRENT &&
				 witness.literal.has_value();
			if (witness.payload_row != SIZE_MAX &&
			    payloads.rows[witness.payload_row].cells[5])
			{
				const size_t bytes =
					payloads.rows[witness.payload_row].cells[5]->size();
				if (bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES ||
				    total_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - bytes)
					valid = false;
				else
					total_bytes += bytes;
			}
			if (!index)
			{
				graph.room = witness.room;
				graph.owner_revision = witness.owner_revision;
			}
			valid &= graph.room == witness.room &&
				 graph.owner_revision == witness.owner_revision;
		}
		const auto root_position = positions.find(root);
		valid &= root_position != positions.end();
		if (root_position != positions.end())
		{
			valid &= evidence.witnesses[members[root_position->second]]
					 .parent_item_uid == 0;
			order.push_back(root_position->second);
			depths[root_position->second] = 1;
		}
		for (size_t index = 0; index < members.size(); ++index)
		{
			if (root_position != positions.end() && index == root_position->second)
				continue;
			const auto parent =
				positions.find(evidence.witnesses[members[index]].parent_item_uid);
			if (parent == positions.end() || parent->second == index)
				valid = false;
			else
				children[parent->second].push_back(index);
		}
		std::vector<int32_t> native_positions(members.size(), -1);
		std::vector<player_item_snapshot> complete;
		for (size_t cursor = 0; cursor < order.size() && cursor < members.size(); ++cursor)
		{
			const size_t index = order[cursor];
			if (native_positions[index] != -1 ||
			    depths[index] > PLAYER_SNAPSHOT_MAX_DEPTH)
			{
				valid = false;
				break;
			}
			native_positions[index] = static_cast<int32_t>(cursor);
			const auto &witness = evidence.witnesses[members[index]];
			graph.witness_indices.push_back(members[index]);
			if (witness.literal)
			{
				auto item = *witness.literal;
				item.equipment_slot = -1;
				item.parent_index = cursor ? native_positions[positions.at(
								     witness.parent_item_uid)] :
							     -1;
				complete.push_back(std::move(item));
			}
			for (const size_t child : children[index])
			{
				depths[child] = depths[index] + 1;
				order.push_back(child);
			}
		}
		valid &= order.size() == members.size() && complete.size() == members.size();
		std::vector<uint8_t> encoded;
		if (valid)
		{
			const auto encoding = player_item_snapshot_list_encode(complete, &encoded);
			if (encoding == player_snapshot_codec_result::allocation_failure)
				throw room_source_failure{ ENOMEM };
			valid = encoding == player_snapshot_codec_result::ok;
		}
		graph.valid = valid;
		if (!valid)
			for (const size_t member : members)
			{
				evidence.witnesses[member].flags |= SQL_ROOM_SOURCE_BAD_GRAPH;
				finding(member, SQL_ROOM_SOURCE_BAD_GRAPH);
			}
		evidence.graphs.push_back(std::move(graph));
	}
}
#ifndef __NO_MYSQL__
void room_execute(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
		throw room_source_failure{ mysql_errno(connection) ? mysql_errno(connection) :
								     EIO };
}
void room_active(MYSQL *connection, unsigned long session)
{
	room_require(mysql_thread_id(connection) == session &&
			     (connection->server_status & SERVER_STATUS_IN_TRANS) &&
			     (connection->server_status & SERVER_STATUS_AUTOCOMMIT),
		     ENOTCONN);
}
std::vector<uint64_t> room_counts(MYSQL *connection, const std::string &sql, size_t fields)
{
	room_execute(connection, sql);
	rows_ptr rows(mysql_store_result(connection), mysql_free_result);
	room_require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	room_require(mysql_num_rows(rows.get()) == 1 && mysql_num_fields(rows.get()) == fields);
	MYSQL_ROW row = mysql_fetch_row(rows.get());
	const auto lengths = mysql_fetch_lengths(rows.get());
	room_require(row && lengths);
	std::vector<uint64_t> output(fields);
	for (size_t index = 0; index < fields; ++index)
	{
		room_require(row[index] && lengths[index]);
		const auto parsed =
			std::from_chars(row[index], row[index] + lengths[index], output[index]);
		room_require(parsed.ec == std::errc{} && parsed.ptr == row[index] + lengths[index]);
	}
	return output;
}
void room_metadata(MYSQL *connection, const room_source_spec &spec, size_t table_index)
{
	// Verify the selected native fields, permitting unrelated additive columns.
	// Binary operation identity must retain its actual 16-byte SQL type.
	const auto columns = room_columns(spec.columns);
	std::string names;
	for (const auto &column : columns)
	{
		if (!names.empty())
			names += ',';
		names += "'" + column + "'";
	}
	auto metadata = query(
		connection,
		"SELECT column_name,data_type,column_type,is_nullable,COALESCE(character_octet_length,0) FROM information_schema.columns WHERE table_schema=DATABASE() AND table_name='" +
			std::string(spec.name) + "' AND column_name IN (" + names +
			") ORDER BY column_name");
	room_require(bool(metadata), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	room_require(mysql_num_rows(metadata.get()) == columns.size(), EPROTONOSUPPORT);
	std::set<std::string> seen;
	while (MYSQL_ROW row = mysql_fetch_row(metadata.get()))
	{
		room_require(row[0] && row[1] && row[2] && row[3] && row[4], EPROTONOSUPPORT);
		const auto position = std::find(columns.begin(), columns.end(), row[0]);
		room_require(position != columns.end() && seen.insert(row[0]).second,
			     EPROTONOSUPPORT);
		const size_t field = static_cast<size_t>(position - columns.begin());
		const char *type = "bigint";
		bool unsigned_ = true, nullable = false;
		uint64_t octets = 0;
		if ((table_index == 0 && field == 3) || (table_index == 2 && field == 0) ||
		    (table_index == 3 && (field == 0 || field == 7)) ||
		    (table_index == 4 && field == 0))
		{
			type = "binary";
			unsigned_ = false;
			octets = 16;
		}
		else if (table_index == 0 && field == 2)
			type = "smallint";
		else if (table_index == 0 && field == 5)
		{
			type = "mediumblob";
			unsigned_ = false;
		}
		else if (table_index == 1 && field == 0)
			type = "tinyint";
		else if (table_index == 1 && field == 2)
		{
			type = "enum";
			unsigned_ = false;
		}
		else if (table_index == 2)
		{
			if (field == 1 || field == 14 || field == 16 || field == 17 || field == 18)
				type = "smallint";
			else if (field == 5 || field == 8)
				type = "tinyint";
			else if (field == 15)
				unsigned_ = false;
			nullable = field == 4;
		}
		else if (table_index == 3 && (field == 1 || field == 3 || field == 8))
			type = "smallint";
		else if (table_index == 3 && field == 2)
			type = "int";
		else if (table_index == 4 && field == 1)
			type = "tinyint";
		else if (table_index == 4 && field == 2)
			type = "int";
		room_require(!std::strcmp(row[1], type) &&
				     (std::strstr(row[2], "unsigned") != nullptr) == unsigned_ &&
				     !std::strcmp(row[3], nullable ? "YES" : "NO"),
			     EPROTONOSUPPORT);
		if (octets)
		{
			uint64_t actual = 0;
			room_require(number(row[4], &actual) && actual == octets, EPROTONOSUPPORT);
		}
	}
}
void room_capture_table(MYSQL *connection, unsigned long session, const room_source_spec &spec,
			const economic_sql_source_limits &limits, room_budget &budget,
			std::vector<economic_sql_source_table> &destination)
{
	economic_sql_source_table table;
	table.name = spec.name;
	table.columns = room_columns(spec.columns);
	table.definition_digest = room_definition(spec);
	std::string selected, total, largest;
	for (const auto &column : table.columns)
	{
		const auto expression = "CAST((" + column + ") AS BINARY)";
		const auto length = "COALESCE(OCTET_LENGTH(" + expression + "),0)";
		if (!selected.empty())
		{
			selected += ',';
			total += '+';
			largest += ',';
		}
		selected += expression;
		total += length;
		largest += length;
	}
	const auto from = " FROM `" + table.name + "`";
	const auto expected =
		room_counts(connection,
			    "SELECT COUNT(*),COALESCE(SUM(" + total +
				    "),0),COALESCE(MAX(GREATEST(" + largest + ")),0)" + from,
			    3);
	room_active(connection, session);
	room_add(budget.rows, expected[0], limits.maximum_rows);
	room_require(expected[0] <= limits.maximum_cells / table.columns.size(), E2BIG);
	room_add(budget.cells, expected[0] * table.columns.size(), limits.maximum_cells);
	room_add(budget.bytes, expected[1], limits.maximum_cell_bytes);
	room_require(expected[2] <= limits.maximum_single_cell_bytes, E2BIG);
	table.rows.reserve(expected[0]);
	room_execute(connection, "SELECT " + selected + from + " ORDER BY " + spec.order);
	rows_ptr rows(mysql_use_result(connection), mysql_free_result);
	room_require(bool(rows), mysql_errno(connection) ? mysql_errno(connection) : EIO);
	room_require(mysql_num_fields(rows.get()) == table.columns.size());
	uint64_t bytes = 0;
	while (MYSQL_ROW row = mysql_fetch_row(rows.get()))
	{
		room_require(table.rows.size() < expected[0]);
		const auto lengths = mysql_fetch_lengths(rows.get());
		room_require(lengths);
		economic_sql_source_row retained;
		retained.cells.reserve(table.columns.size());
		for (size_t index = 0; index < table.columns.size(); ++index)
		{
			if (row[index])
			{
				room_require(lengths[index] <= limits.maximum_single_cell_bytes,
					     E2BIG);
				room_add(bytes, lengths[index], expected[1]);
				retained.cells.emplace_back(
					std::string(row[index], lengths[index]));
			}
			else
				retained.cells.emplace_back(std::nullopt);
		}
		retained.digest = room_row_digest(table, retained);
		table.rows.push_back(std::move(retained));
	}
	room_require(!mysql_errno(connection), mysql_errno(connection));
	room_require(table.rows.size() == expected[0] && bytes == expected[1]);
	rows.reset();
	room_active(connection, session);
	room_hash content;
	content.text("EST1");
	content.bytes(table.definition_digest);
	content.number(table.rows.size());
	for (const auto &row : table.rows)
		content.bytes(row.digest);
	table.content_digest = content.finish();
	destination.push_back(std::move(table));
}
#endif
}

unsigned int
sql_room_item_payload_inspect_sources(const economic_sql_physical_source_snapshot &base,
				      const std::vector<economic_sql_source_table> &tables,
				      const economic_sql_source_limits &limits,
				      size_t maximum_diagnostics,
				      sql_room_item_source_evidence *output) noexcept
{
	try
	{
		room_require(output, EINVAL);
		(void)room_validate(base, tables, limits, maximum_diagnostics);
		sql_room_item_source_evidence evidence;
		room_inspect(base, tables, maximum_diagnostics, evidence);
		*output = std::move(evidence);
		return 0;
	}
	catch (const room_source_failure &failure)
	{
		return failure.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
}

unsigned int sql_room_item_payload_capture_sources_in_transaction(
	MYSQL *connection, const economic_sql_source_limits &limits,
	const economic_sql_physical_source_snapshot &base, sql_room_item_source_snapshot *output,
	size_t maximum_diagnostics) noexcept
{
#ifdef __NO_MYSQL__
	(void)connection;
	(void)limits;
	(void)base;
	(void)output;
	(void)maximum_diagnostics;
	return ENOTSUP;
#else
	try
	{
		room_require(connection && output && maximum_diagnostics &&
				     maximum_diagnostics <= 512,
			     EINVAL);
		const auto base_code = economic_sql_validate_physical_sources(base, limits);
		room_require(!base_code, base_code);
		if (!transaction(connection))
			throw room_source_failure{ errno ? static_cast<unsigned int>(errno) :
							   EPERM };
		const unsigned long session = mysql_thread_id(connection);
		room_active(connection, session);
		const char *server = mysql_get_server_info(connection);
		room_require(server, ENOTSUP);
		const bool maria = std::strstr(server, "MariaDB") != nullptr;
		if (maria && !std::strncmp(server, "5.5.5-", 6))
			server += 6;
		room_require((maria && !std::strncmp(server, "10.11.", 6)) ||
				     (!maria && !std::strncmp(server, "8.0.", 4)),
			     ENOTSUP);
		// RR/same-cut is the caller's precondition. A session default can be
		// READ COMMITTED while SET TRANSACTION selected RR for this cut; the
		// default variable cannot prove the active transaction's isolation.
		std::string names;
		for (size_t index = 0; index < std::size(room_sources); ++index)
		{
			const auto &spec = room_sources[index];
			auto metadata =
				query(connection, std::string("SELECT ") + spec.columns +
							  " FROM `" + spec.name + "` LIMIT 0");
			room_require(bool(metadata),
				     mysql_errno(connection) ? mysql_errno(connection) : EIO);
			room_require(mysql_num_fields(metadata.get()) ==
					     room_columns(spec.columns).size(),
				     EPROTONOSUPPORT);
			room_active(connection, session);
			if (!names.empty())
				names += ',';
			names += "'" + std::string(spec.name) + "'";
			auto primary = query(
				connection,
				std::string(
					"SELECT GROUP_CONCAT(column_name ORDER BY seq_in_index SEPARATOR ',') FROM information_schema.statistics WHERE table_schema=DATABASE() AND table_name='") +
					spec.name + "' AND index_name='PRIMARY' AND non_unique=0");
			room_require(bool(primary),
				     mysql_errno(connection) ? mysql_errno(connection) : EIO);
			MYSQL_ROW key = mysql_fetch_row(primary.get());
			room_require(mysql_num_rows(primary.get()) == 1 && key && key[0] &&
					     !std::strcmp(key[0], spec.order),
				     EPROTONOSUPPORT);
			primary.reset();
			room_metadata(connection, spec, index);
		}
		const auto engines = room_counts(
			connection,
			"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() AND table_type='BASE TABLE' AND engine='InnoDB' AND table_name IN (" +
				names + ")",
			1);
		room_require(engines[0] == std::size(room_sources), ENOTSUP);
		if (!schema(connection))
			throw room_source_failure{ errno ? static_cast<unsigned int>(errno) :
							   EPROTONOSUPPORT };
		sql_room_item_source_snapshot captured;
		captured.physical_digest = base.digest;
		room_budget budget{ base.rows, base.cells, base.cell_bytes };
		for (const auto &spec : room_sources)
			room_capture_table(connection, session, spec, limits, budget,
					   captured.tables);
		captured.rows = budget.rows;
		captured.cells = budget.cells;
		captured.cell_bytes = budget.bytes;
		room_hash binding;
		binding.text("sql_room_item_payload persisted evidence");
		binding.bytes(base.digest);
		binding.number(captured.tables.size());
		for (const auto &table : captured.tables)
			binding.bytes(table.content_digest);
		captured.digest = binding.finish();
		(void)room_validate(base, captured.tables, limits, maximum_diagnostics);
		room_inspect(base, captured.tables, maximum_diagnostics, captured.evidence);
		room_active(connection, session);
		*output = std::move(captured);
		return 0;
	}
	catch (const room_source_failure &failure)
	{
		return failure.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EIO;
	}
#endif
}

namespace
{
bool sql_drop_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
bool sql_drop_policy_supported() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
class sql_drop_entry_table final
	: public std::__umap_hashtable<uint64_t, const item_transfer_entry *>
{
	using table_type = std::__umap_hashtable<uint64_t, const item_transfer_entry *>;
	using node_type_actual =
		std::__detail::_Hash_node<typename table_type::value_type,
					  std::__cache_default<uint64_t, std::hash<uint64_t>>::value>;

    public:
	using table_type::table_type;
	bool current_heap(size_t *out) const noexcept
	{
		if (!out || !this->bucket_count())
			return false;
		size_t bytes = 0;
		if (this->bucket_count() > 1 &&
		    (this->bucket_count() > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		     !sql_drop_add(bytes, this->bucket_count() *
						  sizeof(std::__detail::_Hash_node_base *))))
			return false;
		if (this->size() > SIZE_MAX / sizeof(node_type_actual) ||
		    !sql_drop_add(bytes, this->size() * sizeof(node_type_actual)))
			return false;
		*out = bytes;
		return true;
	}
	bool next_insert_request(size_t *out) const noexcept
	{
		if (!out || this->size() == this->max_size())
			return false;
		size_t bytes = sizeof(node_type_actual);
		auto policy = this->__rehash_policy();
		const auto next = policy._M_need_rehash(this->bucket_count(), this->size(), 1);
		if (next.first && next.second > 1 &&
		    (next.second > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *) ||
		     !sql_drop_add(bytes, next.second * sizeof(std::__detail::_Hash_node_base *))))
			return false;
		*out = bytes;
		return true;
	}
};
static_assert(sizeof(sql_drop_entry_table) ==
	      sizeof(std::unordered_map<uint64_t, const item_transfer_entry *>));
static_assert(alignof(sql_drop_entry_table) ==
	      alignof(std::unordered_map<uint64_t, const item_transfer_entry *>));
static_assert(std::is_same_v<sql_drop_entry_table::iterator,
			     std::unordered_map<uint64_t, const item_transfer_entry *>::iterator>);
#else
using sql_drop_entry_table = std::unordered_map<uint64_t, const item_transfer_entry *>;
#endif
bool sql_room_item_payload_batch_heap(const sql_room_item_payload_batch &value,
				      size_t &bytes) noexcept
{
	if (value.items.capacity() > SIZE_MAX / sizeof(player_item_snapshot) ||
	    value.payloads.capacity() > SIZE_MAX / sizeof(std::vector<uint8_t>) ||
	    !sql_drop_add(bytes, value.items.capacity() * sizeof(player_item_snapshot)) ||
	    !sql_drop_add(bytes, value.payloads.capacity() * sizeof(std::vector<uint8_t>)))
		return false;
	for (const auto &item : value.items)
	{
		size_t heap = 0;
		if (!player_item_snapshot_current_heap_bytes(item, &heap) ||
		    !sql_drop_add(bytes, heap))
			return false;
	}
	for (const auto &encoded : value.payloads)
		if (!sql_drop_add(bytes, encoded.capacity()))
			return false;
	return true;
}
// Complete original singleton helper: original by-value row, normalized fields,
// initializer-list copied row and vector copy all coexist during the full codec.
bool sql_drop_encode_one_bounded(const player_item_snapshot &input, std::vector<uint8_t> *bytes,
				 bool (*reserve)(size_t, void *) noexcept, void *context,
				 size_t outer) noexcept
{
	try
	{
		size_t prefix = outer, heap = 0;
		const size_t frames = 4 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
				      3 * sizeof(player_item_snapshot) +
				      sizeof(std::vector<player_item_snapshot>) +
				      sizeof(std::initializer_list<player_item_snapshot>) +
				      player_item_snapshot_copy_frame_bytes();
		if (!sql_drop_add(prefix, frames) || !reserve || !reserve(prefix, context))
			return false;
		player_item_snapshot item;
		if (player_item_snapshot_clone_bounded(input, &item, reserve, context, prefix) !=
		    player_snapshot_codec_result::ok)
			return false;
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		item.equipment_slot = 0;
		if (!player_item_snapshot_current_heap_bytes(item, &heap) ||
		    !sql_drop_add(prefix, heap))
			return false;
		player_item_snapshot singleton;
		if (player_item_snapshot_clone_bounded(item, &singleton, reserve, context,
						       prefix) != player_snapshot_codec_result::ok)
			return false;
		if (!player_item_snapshot_current_heap_bytes(singleton, &heap) ||
		    !sql_drop_add(prefix, heap) ||
		    !sql_drop_add(prefix, sizeof(player_item_snapshot)) ||
		    !reserve(prefix, context))
			return false;
		std::vector<player_item_snapshot> rows;
		rows.reserve(1);
		player_item_snapshot copied;
		if (player_item_snapshot_clone_bounded(singleton, &copied, reserve, context,
						       prefix) != player_snapshot_codec_result::ok)
			return false;
		rows.push_back(std::move(copied));
		if (!player_item_snapshot_current_heap_bytes(rows.front(), &heap) ||
		    !sql_drop_add(prefix, heap))
			return false;
		return player_item_snapshot_list_encode_bounded(rows, bytes, reserve, context,
								prefix) ==
			       player_snapshot_codec_result::ok &&
		       !bytes->empty() && bytes->size() <= ITEM_TRANSFER_ITEM_BLOB_MAX_BYTES;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
struct sql_drop_capture_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, nested = 0;
	const sql_room_item_payload_batch *candidate = nullptr;
	const std::vector<uint8_t> *canonical = nullptr, *encoded = nullptr;
	const sql_drop_entry_table *entries = nullptr;
	const std::set<uint64_t> *captured = nullptr;
	mutable bool denied = false;
	static bool forward(size_t bytes, void *context) noexcept
	{
		auto *owner = static_cast<sql_drop_capture_budget *>(context);
		if (!owner || owner->denied || !owner->reserve)
			return false;
		if (!owner->reserve(bytes, owner->context))
		{
			owner->denied = true;
			return false;
		}
		return true;
	}
	bool refuse_after(int semantic_error) const noexcept
	{
		// Only an actual refused reservation sets denied. Stale errno,
		// malformed literals and pure profile/limit errors cannot set it.
		return refuse(denied ? ENOBUFS : semantic_error);
	}
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		// Full capture parameters, loop locals, lookup iterators, references,
		// return/closure carriers and original exact_item validation predicates.
		// The genuine bool latch lives in sizeof(*this), including real padding.
		// Relay owns bytes/context parameters, typed owner pointer and bool
		// return; refuse_after owns this/error parameters and bool return plus
		// original refuse's int argument and bool result. These two paths do
		// not coexist. Admit their exact larger source carrier sum prospectively.
		constexpr size_t relay_frames = sizeof(size_t) + 2 * sizeof(void *) + sizeof(bool);
		constexpr size_t refusal_frames =
			sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(bool);
		constexpr size_t forwarding_frames = relay_frames > refusal_frames ? relay_frames :
										     refusal_frames;
		// The three named constexpr size_t profile locals also own real source
		// carriers during prefix; no emitted optimization erasure is assumed.
		constexpr size_t frames =
			forwarding_frames + 3 * sizeof(size_t) + 14 * sizeof(void *) +
			12 * sizeof(size_t) + 7 * sizeof(bool) + 4 * sizeof(std::vector<uint8_t>) +
			sizeof(sql_room_item_payload_batch) + sizeof(sql_drop_entry_table) +
			sizeof(std::set<uint64_t>) + player_item_snapshot_copy_frame_bytes();
		size_t bytes = outer, heap = 0;
		if (!sql_drop_add(bytes, sizeof(*this)) || !sql_drop_add(bytes, frames) ||
		    (candidate && !sql_room_item_payload_batch_heap(*candidate, bytes)) ||
		    (canonical && !sql_drop_add(bytes, canonical->capacity())) ||
		    (encoded && !sql_drop_add(bytes, encoded->capacity())))
			return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (entries && (!entries->current_heap(&heap) || !sql_drop_add(bytes, heap)))
			return false;
		if (captured &&
		    (captured->size() > SIZE_MAX / sizeof(std::_Rb_tree_node<uint64_t>) ||
		     !sql_drop_add(bytes, captured->size() * sizeof(std::_Rb_tree_node<uint64_t>))))
			return false;
#else
		return false;
#endif
		if (!sql_drop_add(bytes, extra))
			return false;
		result = bytes;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t bytes = 0;
		return prefix(bytes, extra) &&
		       forward(bytes, const_cast<sql_drop_capture_budget *>(this));
	}
	bool entry_insert(uint64_t key) const noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		const size_t frames =
			sizeof(std::__detail::_Prime_rehash_policy) +
			2 * sizeof(std::pair<bool, size_t>) +
			sizeof(std::__detail::_Prime_rehash_policy::_State) + 3 * sizeof(void *) +
			3 * sizeof(size_t) + sizeof(bool) +
			sizeof(std::pair<sql_drop_entry_table::iterator, bool>) +
			// Genuine emplace allocation: _Scoped_node, actual node/pair/forward,
			// node and bucket allocator and original rollback/destruction scopes.
			4 * (3 * sizeof(void *) + sizeof(size_t)) +
			sizeof(std::allocator<std::__detail::_Hash_node_base *>);
		if (!entries || !peak(frames))
			return false;
		size_t request = 0;
		// Original emplace eagerly owns a node even for duplicates, unlike try_emplace.
		if (!entries->next_insert_request(&request))
			return false;
		(void)key;
		return sql_drop_add(request, frames) && peak(request);
#else
		(void)key;
		return false;
#endif
	}
	bool uid_insert(uint64_t key) const noexcept
	{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
		if (!captured)
			return false;
		// Genuine set unique-insert compares before allocating. Existing UID
		// consumes no node; fresh UID uses one actual original red-black node.
		const size_t request = captured->find(key) == captured->end() ?
					       sizeof(std::_Rb_tree_node<uint64_t>) :
					       0;
		const size_t frames =
			5 * sizeof(void *) + 2 * sizeof(uint64_t) + 3 * sizeof(bool) +
			sizeof(std::pair<std::set<uint64_t>::iterator, bool>) +
			2 * sizeof(std::pair<std::_Rb_tree_node_base *, std::_Rb_tree_node_base *>) +
			4 * (3 * sizeof(void *) + sizeof(size_t));
		return peak(request + frames);
#else
		(void)key;
		return false;
#endif
	}
	bool payload_reserve(size_t count) const noexcept
	{
		const size_t frames = 5 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool) +
				      4 * (3 * sizeof(void *) + sizeof(size_t));
		return count <= SIZE_MAX / sizeof(std::vector<uint8_t>) &&
		       peak(count * sizeof(std::vector<uint8_t>) + frames);
	}
};
} // namespace

bool sql_room_item_payload_batch_current_heap_bytes(const sql_room_item_payload_batch &value,
						    size_t *output) noexcept
{
	if (!output || !sql_drop_policy_supported())
		return false;
	size_t bytes = 0;
	if (!sql_room_item_payload_batch_heap(value, bytes))
		return false;
	*output = bytes;
	return true;
}

bool sql_room_item_payload_capture_bounded(const item_transfer_payload &payload,
					   sql_room_item_payload_batch *batch,
					   bool (*reserve)(size_t, void *) noexcept, void *context,
					   size_t outer_live) noexcept
try
{
	if (!batch || !reserve || !sql_drop_policy_supported() ||
	    payload.reason != item_transfer_reason::player_drop ||
	    payload.reason_id != static_cast<int64_t>(payload.to_owner.id) ||
	    payload.from_owner.type != item_owner_type::player || !payload.from_owner.id ||
	    payload.from_owner.id > INT32_MAX || payload.from_owner.context_id ||
	    payload.to_owner.type != item_owner_type::room || !payload.to_owner.id ||
	    payload.to_owner.id > INT32_MAX || payload.to_owner.context_id || payload.multi_root ||
	    !payload.selected_item_uid || payload.target_parent_item_uid ||
	    (payload.target_root_item_uid &&
	     payload.target_root_item_uid != payload.selected_item_uid) ||
	    payload.expected_target_parent_revision || payload.logical_source_id ||
	    payload.corpse.present || payload.collector.present ||
	    payload.continuation.kind != item_transfer_continuation_kind::none ||
	    !payload.continuation.data.empty() || !payload.item_count ||
	    payload.item_count > ITEM_TRANSFER_MAX_ITEMS || !payload.item_blob_size ||
	    payload.item_blob_size > payload.item_blob.size())
		return refuse(ENOTSUP);
	// Admit actual default object/frame construction before private owners exist.
	sql_drop_capture_budget budget{ reserve, context, outer_live };
	if (!budget.peak(sizeof(sql_room_item_payload_batch) + sizeof(std::vector<uint8_t>) +
			 sizeof(sql_drop_entry_table) + sizeof(std::set<uint64_t>)))
		return refuse(ENOBUFS);
	sql_room_item_payload_batch candidate;
	budget.candidate = &candidate;
	if ((!budget.prefix(budget.nested) ?
		     player_snapshot_codec_result::overflow :
		     player_item_snapshot_list_decode_bounded(
			     payload.item_blob.data(), payload.item_blob_size, &candidate.items,
			     sql_drop_capture_budget::forward, &budget, budget.nested)) !=
		    player_snapshot_codec_result::ok ||
	    candidate.items.size() != payload.item_count)
		return budget.refuse_after(EBADMSG);
	std::vector<uint8_t> canonical;
	budget.canonical = &canonical;
	if ((!budget.prefix(budget.nested) ?
		     player_snapshot_codec_result::overflow :
		     player_item_snapshot_list_encode_bounded(
			     candidate.items, &canonical, sql_drop_capture_budget::forward, &budget,
			     budget.nested)) != player_snapshot_codec_result::ok ||
	    canonical.size() != payload.item_blob_size ||
	    !std::equal(canonical.begin(), canonical.end(), payload.item_blob.begin()))
		return budget.refuse_after(EBADMSG);
	sql_drop_entry_table entries;
	budget.entries = &entries;
	for (size_t index = 0; index < payload.item_count; ++index)
	{
		const auto &entry = payload.items[index];
		if (!entry.item_uid || entry.root_item_uid != payload.selected_item_uid ||
		    !entry.expected_item_revision || entry.expected_item_revision == UINT64_MAX ||
		    entry.expected_state != item_custody_state::active ||
		    (!budget.entry_insert(entry.item_uid) ||
		     !entries.emplace(entry.item_uid, &entry).second))
			return budget.refuse_after(EBADMSG);
	}
	if (!budget.payload_reserve(candidate.items.size()))
		return refuse(ENOBUFS);
	candidate.payloads.reserve(candidate.items.size());
	size_t total_bytes = 0;
	std::set<uint64_t> captured_uids;
	budget.captured = &captured_uids;
	for (size_t index = 0; index < candidate.items.size(); ++index)
	{
		const auto &item = candidate.items[index];
		const auto found = entries.find(item.object_uid);
		if (!exact_item(item) ||
		    (!budget.uid_insert(item.object_uid) ||
		     !captured_uids.insert(item.object_uid).second) ||
		    found == entries.end() || found->second->vnum != item.vnum ||
		    (index == 0 ? (item.object_uid != payload.selected_item_uid ||
				   item.parent_index != -1) :
				  (item.parent_index < 0 ||
				   item.parent_index >= static_cast<int32_t>(index))) ||
		    found->second->parent_item_uid !=
			    (index == 0 ? 0 : candidate.items[item.parent_index].object_uid))
			return budget.refuse_after(EBADMSG);
		std::vector<uint8_t> encoded;
		budget.encoded = &encoded;
		if (!budget.prefix(budget.nested) ||
		    !sql_drop_encode_one_bounded(item, &encoded, sql_drop_capture_budget::forward,
						 &budget, budget.nested))
			return budget.refuse_after(EBADMSG);
		if (total_bytes > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - encoded.size())
			return refuse(E2BIG);
		total_bytes += encoded.size();
		// The original reserve owns every descriptor; this nonallocating move
		// keeps the original payload insertion order and literal bytes.
		candidate.payloads.push_back(std::move(encoded));
		budget.encoded = nullptr;
	}
	if (!budget.peak())
		return refuse(ENOBUFS);
	*batch = std::move(candidate);
	return true;
}
catch (const std::bad_alloc &)
{
	return refuse(ENOMEM);
}
