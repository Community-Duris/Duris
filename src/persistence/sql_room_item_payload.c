#include "persistence/sql_room_item_payload.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <cctype>
#include <climits>
#include <cstring>
#include <memory>
#include <new>
#include <set>
#include <string>
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
			matches +=
				" AND (pi.item_properties IS NULL OR BINARY pi.item_properties=" +
				text_literal(properties) + ")";
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
	*present = count != 0;
	return true;
#endif
}
