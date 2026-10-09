#include "persistence/shop_item_runtime_payload.h"
#include "player/player_snapshot_codec.h"
#include "player/player_snapshot_capture.h"
#include "core/structs.h"
#include <set>
#include <utility>

bool shop_item_runtime_capture_keeper_literal(char_data *keeper,
					      std::vector<player_item_snapshot> *items_out) noexcept
{
	if (!keeper || !items_out)
		return false;
	try
	{
		std::vector<player_item_snapshot> candidate;
		std::set<const obj_data *> objects;
		std::set<uint64_t> uids;
		size_t estimated_bytes = sizeof(player_snapshot);
		// Validate the whole physical forest once, including roots appearing in
		// multiple slots/lists and shared descendants across captured trees.
		const auto inspect = [&](auto &&self, const obj_data *object,
					 const obj_data *parent, size_t depth) -> bool
		{
			if (!object || depth > PLAYER_SNAPSHOT_MAX_DEPTH ||
			    objects.size() >= PLAYER_SNAPSHOT_MAX_OBJECTS || !object->obj_uid ||
			    object->obj_uid == UINT64_MAX || !objects.insert(object).second ||
			    !uids.insert(object->obj_uid).second)
				return false;
			if (parent && (object->loc_p != LOC_INSIDE || object->loc.inside != parent))
				return false;
			for (const obj_data *child = object->contains; child;
			     child = child->next_content)
				if (!self(self, child, object, depth + 1))
					return false;
			return true;
		};
		const auto capture_root = [&](obj_data *root, int slot) -> bool
		{
			if (!root)
				return true;
			if (slot < 0 || slot > MAX_WEAR ||
			    (slot && (root->loc_p != LOC_WORN || root->loc.wearing != keeper)) ||
			    (!slot &&
			     (root->loc_p != LOC_CARRIED || root->loc.carrying != keeper)) ||
			    !inspect(inspect, root, nullptr, 1))
				return false;
			std::vector<player_item_snapshot> tree;
			size_t tree_bytes = 0;
			if (player_item_snapshot_tree_capture_literal(root, &tree, &tree_bytes) !=
				    player_snapshot_capture_result::ok ||
			    tree.empty() ||
			    tree.size() > PLAYER_SNAPSHOT_MAX_OBJECTS - candidate.size() ||
			    tree_bytes < sizeof(player_snapshot) ||
			    tree_bytes - sizeof(player_snapshot) >
				    PLAYER_SNAPSHOT_MAX_BYTES - estimated_bytes)
				return false;
			// Each tree capture includes the same snapshot header overhead;
			// charge it only once for this complete forest's memory budget.
			estimated_bytes += tree_bytes - sizeof(player_snapshot);
			const size_t offset = candidate.size();
			for (size_t index = 0; index < tree.size(); ++index)
			{
				auto &item = tree[index];
				if (index == 0)
				{
					if (item.parent_index != PLAYER_SNAPSHOT_NO_PARENT ||
					    item.object_uid != root->obj_uid ||
					    item.equipment_slot != 0)
						return false;
					item.equipment_slot = static_cast<int16_t>(slot);
				}
				else
				{
					if (item.parent_index < 0 ||
					    static_cast<size_t>(item.parent_index) >= index ||
					    item.equipment_slot != 0)
						return false;
					item.parent_index += static_cast<int32_t>(offset);
				}
				candidate.push_back(std::move(item));
			}
			return true;
		};
		for (int slot = 0; slot < MAX_WEAR; ++slot)
			if (!capture_root(keeper->equipment[slot], slot + 1))
				return false;
		for (obj_data *root = keeper->carrying; root; root = root->next_content)
			if (!capture_root(root, 0))
				return false;
		if (candidate.size() != objects.size())
			return false;
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_encode(candidate, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() > PLAYER_SNAPSHOT_MAX_BYTES)
			return false;
		*items_out = std::move(candidate);
		return true;
	}
	catch (...)
	{
		return false;
	}
}

#ifndef __NO_MYSQL__
#include "player/player_sql_transaction_cleanup.h"
#include "item/item_transfer_command.h"
#include "item/item_ownership_runtime.h"
#include "economy/shop_trade_command.h"
#include "world/object_template.h"
#include "core/prototypes.h"
#include "core/mm.h"
#include "core/utils.h"
#include <algorithm>
#include <charconv>
#include <cerrno>
#include <cstring>
#include <memory>
#include <sstream>
#include <tuple>

extern P_index obj_index;

namespace
{
struct refusal
{
	int error;
};
void need(bool okay, int error = EILSEQ)
{
	if (!okay)
		throw refusal{ error };
}
using rows = std::unique_ptr<MYSQL_RES, decltype(&mysql_free_result)>;
void run(MYSQL *connection, const std::string &sql)
{
	if (mysql_real_query(connection, sql.data(), sql.size()))
	{
		const unsigned code = mysql_errno(connection);
		throw refusal{ static_cast<int>(code ? code : EIO) };
	}
}
rows read(MYSQL *connection, const std::string &sql)
{
	run(connection, sql);
	rows result(mysql_store_result(connection), mysql_free_result);
	need(result != nullptr, EIO);
	return result;
}
template <class T> T number(const char *text)
{
	need(text != nullptr);
	T value{};
	const auto end = text + strlen(text);
	const auto parsed = std::from_chars(text, end, value);
	need(parsed.ec == std::errc{} && parsed.ptr == end);
	return value;
}
std::string quote(MYSQL *connection, const std::string &text)
{
	std::string result(text.size() * 2 + 1, '\0');
	result.resize(
		mysql_real_escape_string(connection, result.data(), text.data(), text.size()));
	return "'" + result + "'";
}
const char *table(bool keeper)
{
	return keeper ? "shopkeeper_items" : "player_items";
}
const char *runtime(bool keeper)
{
	return keeper ? "shopkeeper_item_runtime_state" : "player_item_runtime_state";
}
const char *affects(bool keeper)
{
	return keeper ? "shopkeeper_item_affects" : "player_item_affects";
}
const char *descriptions(bool keeper)
{
	return keeper ? "shopkeeper_item_extra_descr" : "player_item_extra_descr";
}
void transaction(MYSQL *connection)
{
	need(connection && (connection->server_status & SERVER_STATUS_IN_TRANS), EBUSY);
	using flag = std::remove_pointer_t<decltype(MYSQL_BIND{}.is_null)>;
	flag reconnect = false;
	need(!mysql_get_option(connection, MYSQL_OPT_RECONNECT, &reconnect) && !reconnect, EINVAL);
}
std::string encode(const player_item_snapshot &source)
{
	auto item = source;
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	std::vector<uint8_t> bytes;
	const auto code = player_item_snapshot_list_encode({ item }, &bytes);
	need(code == player_snapshot_codec_result::ok,
	     code == player_snapshot_codec_result::allocation_failure ? ENOMEM : EILSEQ);
	need(!bytes.empty() && bytes.size() <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
	return std::string(reinterpret_cast<const char *>(bytes.data()), bytes.size());
}
std::string properties(const player_item_snapshot &item)
{
	std::string text;
	const auto code =
		player_item_properties_encode(item.extra2_flags, item.dynamic_affects, &text);
	need(code == player_snapshot_codec_result::ok,
	     code == player_snapshot_codec_result::allocation_failure ? ENOMEM : EILSEQ);
	return text;
}
std::string description_text(const player_item_extra_description_snapshot &description)
{
	if (!description.spellbook)
		return description.description;
	std::ostringstream text;
	text.exceptions(std::ios::badbit | std::ios::failbit);
	text << '[';
	for (size_t index = 0; index < description.spell_ids.size(); ++index)
		text << (index ? "," : "") << description.spell_ids[index];
	text << ']';
	return text.str();
}
std::string strings(MYSQL *connection, const player_item_snapshot &item, uint8_t mask,
		    const std::string &value)
{
	return item.string_mask & mask ? quote(connection, value) : "NULL";
}
void read_payload(MYSQL *connection, bool keeper, uint64_t id, player_item_snapshot *output,
		  bool *present)
{
	auto result = read(connection, "SELECT OCTET_LENGTH(payload),SUBSTRING(payload,1," +
					       std::to_string(PLAYER_SNAPSHOT_MAX_BYTES + 1) +
					       ") FROM " + runtime(keeper) + " WHERE item_id=" +
					       std::to_string(id) + " LIMIT 2 FOR UPDATE");
	need(mysql_num_rows(result.get()) <= 1);
	MYSQL_ROW row = mysql_fetch_row(result.get());
	if (!row)
	{
		*present = false;
		return;
	}
	const auto *lengths = mysql_fetch_lengths(result.get());
	const auto count = number<uint64_t>(row[0]);
	need(count && count <= PLAYER_SNAPSHOT_MAX_BYTES && lengths && row[1] &&
		     lengths[1] == count,
	     E2BIG);
	std::vector<player_item_snapshot> decoded;
	const auto code = player_item_snapshot_list_decode(
		reinterpret_cast<const uint8_t *>(row[1]), lengths[1], &decoded);
	need(code == player_snapshot_codec_result::ok,
	     code == player_snapshot_codec_result::allocation_failure ? ENOMEM : EILSEQ);
	need(decoded.size() == 1 && decoded.front().parent_index == PLAYER_SNAPSHOT_NO_PARENT);
	need(encode(decoded.front()) == std::string(row[1], lengths[1]));
	*output = std::move(decoded.front());
	*present = true;
}
void metadata(MYSQL *connection, bool keeper, uint64_t id, const player_item_snapshot &item)
{
	std::set<std::pair<int16_t, int16_t>> expected;
	for (const auto &affect : item.affects)
		if (affect[0] || affect[1])
			expected.emplace(affect[0], affect[1]);
	auto result =
		read(connection, "SELECT location,modifier FROM " + std::string(affects(keeper)) +
					 " WHERE item_id=" + std::to_string(id) + " LIMIT " +
					 std::to_string(expected.size() + 1) + " FOR UPDATE");
	need(mysql_num_rows(result.get()) == expected.size());
	std::set<std::pair<int16_t, int16_t>> actual;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result.get())))
		need(actual.emplace(number<int16_t>(row[0]), number<int16_t>(row[1])).second);
	need(actual == expected);
	// Scalar equality keeps even corrupted oversized text off the client.
	auto count =
		read(connection, "SELECT COUNT(*) FROM " + std::string(descriptions(keeper)) +
					 " WHERE item_id=" + std::to_string(id) + " FOR UPDATE");
	row = mysql_fetch_row(count.get());
	need(row && number<uint64_t>(row[0]) == item.extra_descriptions.size());
	std::set<std::pair<std::string, std::string>> unique;
	for (const auto &description : item.extra_descriptions)
	{
		need(!description.keyword.empty());
		const auto text = description_text(description);
		need(unique.emplace(description.keyword, text).second);
		auto match = read(connection, "SELECT COUNT(*) FROM " +
						      std::string(descriptions(keeper)) +
						      " WHERE item_id=" + std::to_string(id) +
						      " AND keyword <=> BINARY " +
						      quote(connection, description.keyword) +
						      " AND description <=> BINARY " +
						      quote(connection, text) + " FOR UPDATE");
		row = mysql_fetch_row(match.get());
		need(row && number<uint64_t>(row[0]) == 1);
	}
}
enum class physical_proof_policy
{
	strict_payload,
	legacy_checkpoint,
};
void checkpoint_legacy_metadata(MYSQL *connection, uint64_t id, const player_item_snapshot &item,
				const object_template &prototype)
{
	// Legacy metadata represents values/multiplicity, not their native order.
	std::multiset<std::pair<int16_t, int16_t>> expected_affects, actual_affects;
	for (const auto &affect : item.affects)
		expected_affects.emplace(affect[0], affect[1]);
	auto result = read(connection,
			   "SELECT location,modifier FROM shopkeeper_item_affects WHERE item_id=" +
				   std::to_string(id) + " LIMIT " +
				   std::to_string(item.affects.size() + 1) + " FOR UPDATE");
	need(mysql_num_rows(result.get()) <= item.affects.size(), E2BIG);
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(result.get())))
	{
		need(row[0] && row[1], EILSEQ);
		actual_affects.emplace(number<int16_t>(row[0]), number<int16_t>(row[1]));
	}
	if (actual_affects.empty())
		for (const auto &affect : prototype.affected)
			actual_affects.emplace(affect.location, affect.modifier);
	else
		while (actual_affects.size() < item.affects.size())
			actual_affects.emplace(0, 0);
	need(actual_affects == expected_affects, ESTALE);

	auto count =
		read(connection, "SELECT COUNT(*) FROM shopkeeper_item_extra_descr WHERE item_id=" +
					 std::to_string(id) + " FOR UPDATE");
	row = mysql_fetch_row(count.get());
	need(row, EIO);
	const auto native_count = number<uint64_t>(row[0]);
	need(native_count <= PLAYER_SNAPSHOT_MAX_ROWS, E2BIG);
	std::map<std::pair<std::string, std::string>, size_t> expected;
	for (const auto &description : item.extra_descriptions)
		++expected[{ description.keyword, description_text(description) }];
	// A legacy load prepends stored rows to native prototype descriptions.
	// Accept that independently sourced addition, without inventing row order.
	if (native_count != item.extra_descriptions.size())
	{
		for (const auto &description : prototype.descriptions)
		{
			const auto found =
				expected.find({ description.keyword, description.description });
			need(found != expected.end() && found->second, ESTALE);
			if (!--found->second)
				expected.erase(found);
		}
	}
	size_t expected_count = 0;
	for (const auto &[description, multiplicity] : expected)
	{
		expected_count += multiplicity;
		auto match =
			read(connection,
			     "SELECT COUNT(*) FROM shopkeeper_item_extra_descr WHERE item_id=" +
				     std::to_string(id) + " AND keyword <=> BINARY " +
				     quote(connection, description.first) +
				     " AND COALESCE(description,'') <=> BINARY " +
				     quote(connection, description.second) + " FOR UPDATE");
		row = mysql_fetch_row(match.get());
		need(row && number<uint64_t>(row[0]) == multiplicity, ESTALE);
	}
	need(expected_count == native_count, ESTALE);
}
void verify(MYSQL *connection, bool keeper, uint64_t id, uint64_t owner, uint64_t parent,
	    const player_item_snapshot &item, bool exclusive = true,
	    physical_proof_policy policy = physical_proof_policy::strict_payload)
{
	const auto *prototype = find_recovery_object_template(item.vnum);
	need(prototype, ENODATA);
	std::ostringstream predicate;
	predicate.exceptions(std::ios::badbit | std::ios::failbit);
	const bool legacy = policy == physical_proof_policy::legacy_checkpoint;
	need(!legacy || keeper, EINVAL);
	predicate << (keeper ? "shopkeeper_id" : "pid") << '=' << owner << " AND vnum=" << item.vnum
		  << " AND equip_slot=" << item.equipment_slot
		  << " AND COALESCE(container_id,0)=" << parent;
	if (legacy)
		predicate << " AND COALESCE(quantity,1)=1 AND COALESCE(weight," << prototype->weight
			  << ")=" << item.weight << " AND COALESCE(cost," << prototype->cost
			  << ")=" << item.cost << " AND COALESCE(timer,0)=" << item.timers[0]
			  << " AND COALESCE(extra_flags," << prototype->extra_flags
			  << ")=" << item.extra_flags;
	else
		predicate << " AND quantity=1 AND weight=" << item.weight
			  << " AND cost=" << item.cost << " AND timer=" << item.timers[0]
			  << " AND extra_flags=" << item.extra_flags;
	// Legacy physical NULLs mean the trusted prototype, never the incoming item.
	predicate << " AND COALESCE(wear_flags," << prototype->wear_flags << ")=" << item.wear_flags
		  << " AND COALESCE(item_type," << static_cast<int>(prototype->type)
		  << ")=" << static_cast<int>(item.type) << " AND COALESCE(item_material,"
		  << static_cast<int>(prototype->material)
		  << ")=" << static_cast<int>(item.material);
	if (legacy)
		predicate << " AND COALESCE(item_condition," << prototype->condition
			  << ")=" << item.condition;
	else
		predicate << " AND item_condition=" << item.condition;
	for (size_t i = 0; i < item.values.size(); ++i)
		if (legacy)
			predicate << " AND COALESCE(value" << i << ",0)=" << item.values[i];
		else
			predicate << " AND value" << i << '=' << item.values[i];
	if (legacy)
	{
		for (const auto &[column, value, fallback] :
		     { std::tuple{ "name", &item.name, &prototype->name },
		       std::tuple{ "short_descr", &item.short_description,
				   &prototype->short_description },
		       std::tuple{ "description", &item.description, &prototype->description },
		       std::tuple{ "action_descr", &item.action_description,
				   &prototype->action_description } })
			predicate << " AND (CASE WHEN " << column << " IS NULL OR OCTET_LENGTH("
				  << column << ")=0 THEN BINARY " << quote(connection, *fallback)
				  << " ELSE BINARY " << column << " END) <=> BINARY "
				  << quote(connection, *value);
	}
	else
		for (const auto &[column, mask, value] :
		     { std::tuple{ "name", 1, &item.name },
		       std::tuple{ "short_descr", 4, &item.short_description },
		       std::tuple{ "description", 2, &item.description },
		       std::tuple{ "action_descr", 8, &item.action_description } })
			predicate << " AND " << column << " <=> "
				  << (item.string_mask & mask ? "BINARY " : "")
				  << strings(connection, item, static_cast<uint8_t>(mask), *value);
	const uint64_t prototype_bits[] = { prototype->bitvector, prototype->bitvector2,
					    prototype->bitvector3, prototype->bitvector4,
					    prototype->bitvector5 };
	for (size_t i = 0; i < item.bitvectors.size(); ++i)
		predicate << " AND COALESCE(bitvector" << i + 1 << ',' << prototype_bits[i]
			  << ")=" << item.bitvectors[i];
	const auto property = properties(item);
	if (!legacy)
		predicate << " AND OCTET_LENGTH(item_properties)=" << property.size()
			  << " AND item_properties <=> BINARY " << quote(connection, property);
	auto result = read(connection, "SELECT id,(" + predicate.str() + ") FROM " + table(keeper) +
					       " WHERE obj_uid=" + std::to_string(item.object_uid) +
					       " LIMIT 2 FOR UPDATE");
	MYSQL_ROW row = mysql_fetch_row(result.get());
	need(row && mysql_num_rows(result.get()) == 1 && number<uint64_t>(row[0]) == id && row[1] &&
		     !strcmp(row[1], "1"),
	     ESTALE);
	if (legacy)
	{
		auto property_row =
			read(connection,
			     "SELECT OCTET_LENGTH(item_properties),SUBSTRING(item_properties,1," +
				     std::to_string(PLAYER_ITEM_PROPERTIES_MAX_HEX_BYTES + 1) +
				     ") FROM shopkeeper_items WHERE id=" + std::to_string(id) +
				     " LIMIT 2 FOR UPDATE");
		row = mysql_fetch_row(property_row.get());
		need(row && mysql_num_rows(property_row.get()) == 1, ESTALE);
		if (row[0])
		{
			const auto *lengths = mysql_fetch_lengths(property_row.get());
			const auto count = number<uint64_t>(row[0]);
			need(count && count <= PLAYER_ITEM_PROPERTIES_MAX_HEX_BYTES && lengths &&
				     row[1] && lengths[1] == count,
			     E2BIG);
			player_item_snapshot represented{};
			const auto decoded = player_item_properties_decode(
				std::string(row[1], lengths[1]), &represented.extra2_flags,
				&represented.dynamic_affects);
			need(decoded == player_snapshot_codec_result::ok,
			     decoded == player_snapshot_codec_result::allocation_failure ? ENOMEM :
											   EILSEQ);
			need(properties(represented) == property, ESTALE);
		}
		// NULL stores no current flags/affects: the sealed keeper body, not
		// an invented prototype/zero equality, must supply these runtime facts.
		checkpoint_legacy_metadata(connection, id, item, *prototype);
	}
	else
	{
		const auto payload = encode(item);
		result = read(connection,
			      "SELECT (OCTET_LENGTH(payload)=" + std::to_string(payload.size()) +
				      " AND payload <=> BINARY " + quote(connection, payload) +
				      ") FROM " + runtime(keeper) + " WHERE item_id=" +
				      std::to_string(id) + " LIMIT 2 FOR UPDATE");
		row = mysql_fetch_row(result.get());
		need(row && mysql_num_rows(result.get()) == 1 && row[0] && !strcmp(row[0], "1"),
		     ESTALE);
		metadata(connection, keeper, id, item);
	}
	if (exclusive)
	{
		auto other = read(connection,
				  "SELECT id FROM " + std::string(table(!keeper)) +
					  " WHERE obj_uid=" + std::to_string(item.object_uid) +
					  " LIMIT 1 FOR UPDATE");
		need(!mysql_num_rows(other.get()), ESTALE);
	}
}
void write_payload(MYSQL *connection, bool keeper, uint64_t id, const player_item_snapshot &item)
{
	const auto payload = encode(item), property = properties(item);
	// The only changed physical policy field is in the frozen after image. Write
	// every base value explicitly for enrolled rows so later prototype changes
	// cannot silently reinterpret an accounted native payload.
	std::string sql = "UPDATE " + std::string(table(keeper)) +
			  " SET weight=" + std::to_string(item.weight) +
			  ",cost=" + std::to_string(item.cost) +
			  ",timer=" + std::to_string(item.timers[0]) +
			  ",extra_flags=" + std::to_string(item.extra_flags) +
			  ",wear_flags=" + std::to_string(item.wear_flags) +
			  ",item_type=" + std::to_string(item.type) +
			  ",item_material=" + std::to_string(item.material) +
			  ",item_condition=" + std::to_string(item.condition) +
			  ",item_properties=" + quote(connection, property);
	for (size_t i = 0; i < item.values.size(); ++i)
		sql += ",value" + std::to_string(i) + "=" + std::to_string(item.values[i]);
	for (size_t i = 0; i < item.bitvectors.size(); ++i)
		sql += ",bitvector" + std::to_string(i + 1) + "=" +
		       std::to_string(item.bitvectors[i]);
	sql += ",name=" + strings(connection, item, 1, item.name) +
	       ",short_descr=" + strings(connection, item, 4, item.short_description) +
	       ",description=" + strings(connection, item, 2, item.description) +
	       ",action_descr=" + strings(connection, item, 8, item.action_description) +
	       " WHERE id=" + std::to_string(id) +
	       " AND obj_uid=" + std::to_string(item.object_uid) +
	       " AND vnum=" + std::to_string(item.vnum);
	run(connection, sql);
	auto identity = read(connection, "SELECT id FROM " + std::string(table(keeper)) +
						 " WHERE id=" + std::to_string(id) +
						 " AND obj_uid=" + std::to_string(item.object_uid) +
						 " AND vnum=" + std::to_string(item.vnum) +
						 " LIMIT 2 FOR UPDATE");
	need(mysql_num_rows(identity.get()) == 1, ESTALE);
	run(connection, "INSERT INTO " + std::string(runtime(keeper)) +
				"(item_id,payload) VALUES(" + std::to_string(id) + ',' +
				quote(connection, payload) +
				") ON DUPLICATE KEY UPDATE payload=VALUES(payload)");
	run(connection,
	    "DELETE FROM " + std::string(affects(keeper)) + " WHERE item_id=" + std::to_string(id));
	std::set<std::pair<int16_t, int16_t>> unique;
	for (const auto &affect : item.affects)
		if ((affect[0] || affect[1]) && unique.emplace(affect[0], affect[1]).second)
			run(connection, "INSERT INTO " + std::string(affects(keeper)) +
						"(item_id,location,modifier) VALUES(" +
						std::to_string(id) + ',' +
						std::to_string(affect[0]) + ',' +
						std::to_string(affect[1]) + ")");
	run(connection, "DELETE FROM " + std::string(descriptions(keeper)) +
				" WHERE item_id=" + std::to_string(id));
	std::set<std::pair<std::string, std::string>> extras;
	for (const auto &description : item.extra_descriptions)
	{
		const auto text = description_text(description);
		need(!description.keyword.empty() &&
		     extras.emplace(description.keyword, text).second);
		run(connection, "INSERT INTO " + std::string(descriptions(keeper)) +
					"(item_id,keyword,description) VALUES(" +
					std::to_string(id) + ',' +
					quote(connection, description.keyword) + ',' +
					quote(connection, text) + ")");
	}
	// Read back every physical/property/metadata value before the caller can
	// commit, including any server-mode truncation of legacy SQL columns.
	auto destination =
		read(connection, "SELECT " + std::string(keeper ? "shopkeeper_id" : "pid") +
					 ",COALESCE(container_id,0) FROM " + table(keeper) +
					 " WHERE id=" + std::to_string(id) + " LIMIT 2 FOR UPDATE");
	MYSQL_ROW row = mysql_fetch_row(destination.get());
	need(row && mysql_num_rows(destination.get()) == 1, ESTALE);
	verify(connection, keeper, id, number<uint64_t>(row[0]), number<uint64_t>(row[1]), item,
	       false);
}
bool enrolled(MYSQL *connection, uint64_t uid, bool current_read = false)
{
	// Exact original event/UID/revision and canonical successful v6/v7/v8 receipt;
	// current custody below remains the only current owner/topology authority.
	auto result = read(
		connection,
		"SELECT r.after_revision,OCTET_LENGTH(i.result_payload),SUBSTRING(i.result_payload,1," +
			std::to_string(SHOP_TRADE_RESULT_BYTES + 1) +
			") FROM economic_accounting_item_reference r "
			"JOIN economic_accounting_operation o ON o.operation_id=r.operation_id "
			"JOIN critical_operation_inbox i ON i.operation_id=r.operation_id "
			"JOIN item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND l.event_index=r.legacy_event_index "
			"AND l.item_uid=r.item_uid AND l.item_revision=r.after_revision WHERE r.item_uid=" +
			std::to_string(uid) +
			" AND o.writer_id=14 AND o.outcome=1 AND o.result_code=0 AND i.command_type=15 "
			"AND i.schema_version=2 AND i.payload_version IN (6,7,8) AND i.status=1 AND i.result_code=0 AND i.failure_stage=0 "
			"ORDER BY r.after_revision DESC LIMIT 1" +
			(current_read ? " LOCK IN SHARE MODE" : ""));
	MYSQL_ROW row = mysql_fetch_row(result.get());
	if (!row)
		return false;
	const auto *lengths = mysql_fetch_lengths(result.get());
	need(lengths && row[2] && number<uint64_t>(row[1]) == SHOP_TRADE_RESULT_BYTES &&
	     lengths[2] == SHOP_TRADE_RESULT_BYTES);
	shop_trade_result receipt{};
	std::array<uint8_t, SHOP_TRADE_RESULT_BYTES> canonical{};
	need(shop_trade_command_decode_result(reinterpret_cast<const uint8_t *>(row[2]), lengths[2],
					      &receipt) &&
	     shop_trade_command_encode_result(receipt, &canonical) &&
	     !memcmp(canonical.data(), row[2], canonical.size()));
	const uint64_t revision = number<uint64_t>(row[0]);
	for (size_t index = 0; index < receipt.item_count; ++index)
		if (receipt.item_uids[index] == uid)
		{
			need(receipt.item_revisions[index] == revision);
			return true;
		}
	throw refusal{ EILSEQ };
}
void keeper_image(MYSQL *connection, uint64_t keeper, uint32_t shop, int32_t vnum,
		  shop_item_runtime_image *output)
{
	auto owner = read(
		connection,
		"SELECT shop_id,mob_vnum,shop_revision,runtime_payload_checkpoint_revision FROM shopkeepers WHERE id=" +
			std::to_string(keeper) + " FOR UPDATE");
	MYSQL_ROW row = mysql_fetch_row(owner.get());
	need(row && mysql_num_rows(owner.get()) == 1 && number<uint32_t>(row[0]) == shop &&
		     number<int32_t>(row[1]) == vnum,
	     ESTALE);
	const bool checkpointed = row[3] != nullptr;
	if (checkpointed)
		need(number<uint64_t>(row[3]) &&
			     number<uint64_t>(row[3]) <= number<uint64_t>(row[2]),
		     ESTALE);
	// A wholly legacy keeper is not enrolled by adding the table. Preserve
	// its original loader/save path, including legacy NULL identities.
	auto any = read(
		connection,
		"SELECT si.id FROM shopkeeper_items si JOIN shopkeeper_item_runtime_state rs ON rs.item_id=si.id "
		"WHERE si.shopkeeper_id=" +
			std::to_string(keeper) + " LIMIT 1");
	if (!mysql_num_rows(any.get()) && !checkpointed)
	{
		any = read(
			connection,
			"SELECT r.item_uid FROM economic_accounting_item_reference r "
			"JOIN economic_accounting_operation o ON o.operation_id=r.operation_id "
			"JOIN critical_operation_inbox i ON i.operation_id=r.operation_id "
			"JOIN item_ownership_ledger l ON l.operation_id=r.legacy_operation_id AND l.event_index=r.legacy_event_index AND l.item_uid=r.item_uid AND l.item_revision=r.after_revision "
			"LEFT JOIN shopkeeper_items si ON si.obj_uid=r.item_uid AND si.shopkeeper_id=" +
				std::to_string(keeper) +
				" LEFT JOIN item_current_owner c ON c.item_uid=r.item_uid AND c.owner_type=" +
				std::to_string(static_cast<uint8_t>(item_owner_type::shopkeeper)) +
				" AND c.owner_id=" +
				std::to_string(item_shopkeeper_owner_id(shop)) +
				" AND c.owner_context_id=0 AND c.state=" +
				std::to_string(static_cast<uint8_t>(item_custody_state::active)) +
				" WHERE o.writer_id=14 AND o.outcome=1 AND o.result_code=0 AND i.command_type=15 AND i.schema_version=2 "
				"AND i.payload_version IN (6,7,8) AND i.status=1 AND i.result_code=0 AND (si.id IS NOT NULL OR c.item_uid IS NOT NULL) LIMIT 1");
		if (!mysql_num_rows(any.get()))
		{
			*output = {};
			return;
		}
	}
	// Keeper serialization first, then sorted current custody, then physical
	// rows: compatible with the accounted trade's native lock order.
	auto locked_custody =
		read(connection,
		     "SELECT item_uid FROM item_current_owner WHERE owner_type=" +
			     std::to_string(static_cast<uint8_t>(item_owner_type::shopkeeper)) +
			     " AND owner_id=" + std::to_string(item_shopkeeper_owner_id(shop)) +
			     " AND owner_context_id=0 ORDER BY item_uid LIMIT " +
			     std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(locked_custody.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	auto physical = read(
		connection,
		"SELECT id,obj_uid,COALESCE(container_id,0),equip_slot FROM shopkeeper_items WHERE shopkeeper_id=" +
			std::to_string(keeper) + " ORDER BY id LIMIT " +
			std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(physical.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	std::map<uint64_t, uint64_t> ids;
	shop_item_runtime_image result;
	size_t bytes = 0;
	while ((row = mysql_fetch_row(physical.get())))
	{
		const uint64_t id = number<uint64_t>(row[0]),
			       uid = row[1] ? number<uint64_t>(row[1]) : 0;
		const auto parent = number<uint64_t>(row[2]);
		const auto slot = number<int16_t>(row[3]);
		need(id && uid != UINT64_MAX && slot >= 0 && slot <= MAX_WEAR &&
		     (!parent || !slot));
		need(ids.emplace(id, uid).second);
		player_item_snapshot item{};
		bool present = false;
		read_payload(connection, true, id, &item, &present);
		const bool required = uid && enrolled(connection, uid);
		need(present || (!required && !checkpointed), ENODATA);
		if (!present)
			continue;
		// The original native checkpoint designates its complete physical forest.
		// Current custody and strict native physical/payload proof still follow.
		need((required || checkpointed) && uid, ENODATA);
		need(item.object_uid == uid && item.equipment_slot == slot);
		bytes += encode(item).size();
		need(bytes <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
		need(result.emplace(uid, shop_item_runtime_row{ id, parent, 0, 0, slot,
								std::move(item), true })
			     .second);
	}
	for (auto &[uid, value] : result)
	{
		uint64_t root = uid, parent_uid = 0, cursor = value.id;
		size_t depth = 0;
		std::set<uint64_t> seen;
		while (cursor)
		{
			need(++depth <= PLAYER_SNAPSHOT_MAX_DEPTH && seen.insert(cursor).second,
			     ELOOP);
			const auto position = ids.find(cursor);
			need(position != ids.end(), ESTALE);
			const auto entry = result.find(position->second);
			need(entry != result.end(), ENODATA);
			if (cursor == value.id && entry->second.parent_id)
			{
				const auto parent = ids.find(entry->second.parent_id);
				need(parent != ids.end(), ESTALE);
				parent_uid = parent->second;
			}
			root = position->second;
			cursor = entry->second.parent_id;
		}
		value.root_uid = root;
		auto custody = read(
			connection,
			"SELECT root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,owner_context_id,item_revision,vnum,state,equipment_slot "
			"FROM item_current_owner WHERE item_uid=" +
				std::to_string(uid) + " FOR UPDATE");
		row = mysql_fetch_row(custody.get());
		need(row && mysql_num_rows(custody.get()) == 1 &&
			     number<uint64_t>(row[0]) == root &&
			     number<uint64_t>(row[1]) == parent_uid &&
			     number<uint8_t>(row[2]) ==
				     static_cast<uint8_t>(item_owner_type::shopkeeper) &&
			     number<uint64_t>(row[3]) == item_shopkeeper_owner_id(shop) &&
			     number<uint64_t>(row[4]) == 0 &&
			     number<int32_t>(row[6]) == value.item.vnum &&
			     number<uint8_t>(row[7]) ==
				     static_cast<uint8_t>(item_custody_state::active) &&
			     number<int16_t>(row[8]) == value.slot,
		     ESTALE);
		value.revision = number<uint64_t>(row[5]);
		need(value.revision, ESTALE);
		verify(connection, true, value.id, keeper, value.parent_id, value.item);
		auto children =
			read(connection, "SELECT id FROM shopkeeper_items WHERE container_id=" +
						 std::to_string(value.id) + " LIMIT " +
						 std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) +
						 " FOR UPDATE");
		need(mysql_num_rows(children.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
		while ((row = mysql_fetch_row(children.get())))
		{
			const auto child = ids.find(number<uint64_t>(row[0]));
			need(child != ids.end(), ESTALE);
			need(result.count(child->second), ENODATA);
		}
	}
	// A deleted whole enrolled tree must not downgrade to legacy absence.
	auto custody =
		read(connection,
		     "SELECT item_uid,root_item_uid FROM item_current_owner WHERE owner_type=" +
			     std::to_string(static_cast<uint8_t>(item_owner_type::shopkeeper)) +
			     " AND owner_id=" + std::to_string(item_shopkeeper_owner_id(shop)) +
			     " AND owner_context_id=0 AND state=" +
			     std::to_string(static_cast<uint8_t>(item_custody_state::active)) +
			     " ORDER BY item_uid LIMIT " +
			     std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(custody.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	while ((row = mysql_fetch_row(custody.get())))
	{
		const auto uid = number<uint64_t>(row[0]), root = number<uint64_t>(row[1]);
		if (checkpointed || result.count(root) || enrolled(connection, uid))
			need(result.count(uid), ENODATA);
	}
	*output = std::move(result);
}

// A marked dirty save replaces the complete current keeper forest, not just
// the UIDs which happened to have a prior payload. Current custody is required
// independently for every live incoming node; no UID is adopted by this save.
void refresh_marked_image(MYSQL *connection, uint64_t keeper, uint32_t shop, char_data *body,
			  shop_item_runtime_image *output,
			  std::vector<player_item_snapshot> *literal_out)
{
	std::vector<player_item_snapshot> current;
	need(shop_item_runtime_capture_keeper_literal(body, &current), EAGAIN);
	std::map<uint64_t, size_t> indices;
	std::vector<uint64_t> roots(current.size()), parents(current.size());
	std::ostringstream selected;
	selected.exceptions(std::ios::badbit | std::ios::failbit);
	for (size_t index = 0; index < current.size(); ++index)
	{
		const auto &item = current[index];
		need(indices.emplace(item.object_uid, index).second, ESTALE);
		if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
			roots[index] = item.object_uid;
		else
		{
			need(item.parent_index >= 0 &&
				     static_cast<size_t>(item.parent_index) < index,
			     ESTALE);
			const auto parent = static_cast<size_t>(item.parent_index);
			roots[index] = roots[parent];
			parents[index] = current[parent].object_uid;
		}
		selected << (index ? "," : "") << item.object_uid;
	}
	const auto uids = selected.str();
	std::string scope =
		"(owner_type=" + std::to_string(static_cast<uint8_t>(item_owner_type::shopkeeper)) +
		" AND owner_id=" + std::to_string(item_shopkeeper_owner_id(shop)) +
		" AND state=" + std::to_string(static_cast<uint8_t>(item_custody_state::active)) +
		")";
	if (!current.empty())
		scope += " OR item_uid IN(" + uids + ") OR ((root_item_uid IN(" + uids +
			 ") OR parent_item_uid IN(" + uids + ")) AND state=" +
			 std::to_string(static_cast<uint8_t>(item_custody_state::active)) + ")";
	auto custody = read(
		connection,
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
		"owner_context_id,item_revision,vnum,state,equipment_slot FROM item_current_owner WHERE " +
			scope + " ORDER BY item_uid LIMIT " +
			std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(custody.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	need(mysql_num_rows(custody.get()) == current.size(), ESTALE);
	shop_item_runtime_image candidate;
	MYSQL_ROW row;
	while ((row = mysql_fetch_row(custody.get())))
	{
		const auto uid = number<uint64_t>(row[0]);
		const auto found = indices.find(uid);
		need(found != indices.end(), ESTALE);
		const auto index = found->second;
		const auto revision = number<uint64_t>(row[6]);
		need(number<uint64_t>(row[1]) == roots[index] &&
			     number<uint64_t>(row[2]) == parents[index] &&
			     number<uint8_t>(row[3]) ==
				     static_cast<uint8_t>(item_owner_type::shopkeeper) &&
			     number<uint64_t>(row[4]) == item_shopkeeper_owner_id(shop) &&
			     number<uint64_t>(row[5]) == 0 && revision &&
			     number<int32_t>(row[7]) == current[index].vnum &&
			     number<uint8_t>(row[8]) ==
				     static_cast<uint8_t>(item_custody_state::active) &&
			     number<int16_t>(row[9]) == current[index].equipment_slot,
		     ESTALE);
		auto item = current[index];
		item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
		need(candidate
			     .emplace(uid, shop_item_runtime_row{ 0, 0, roots[index], revision,
								  item.equipment_slot,
								  std::move(item), false })
			     .second,
		     ESTALE);
	}
	auto physical = read(
		connection,
		"SELECT id,obj_uid,vnum,COALESCE(container_id,0),equip_slot FROM shopkeeper_items WHERE shopkeeper_id=" +
			std::to_string(keeper) + " ORDER BY id LIMIT " +
			std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(physical.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	std::map<uint64_t, uint64_t> ids, physical_parents;
	std::set<int16_t> slots;
	while ((row = mysql_fetch_row(physical.get())))
	{
		const auto id = number<uint64_t>(row[0]), uid = number<uint64_t>(row[1]),
			   parent = number<uint64_t>(row[3]);
		const auto slot = number<int16_t>(row[4]);
		auto found = candidate.find(uid);
		need(id && found != candidate.end() && !found->second.id &&
			     number<int32_t>(row[2]) == found->second.item.vnum && slot >= 0 &&
			     slot <= MAX_WEAR && (!parent || !slot) &&
			     (!slot || slots.insert(slot).second) && ids.emplace(id, uid).second &&
			     physical_parents.emplace(id, parent).second,
		     ESTALE);
		player_item_snapshot stored;
		bool present = false;
		read_payload(connection, true, id, &stored, &present);
		// A marked existing row cannot lose its history and downgrade to legacy.
		need(present && stored.object_uid == uid &&
			     stored.vnum == found->second.item.vnum &&
			     stored.equipment_slot == slot,
		     ENODATA);
		verify(connection, true, id, keeper, parent, stored);
		found->second.id = id;
		found->second.parent_id = parent;
		found->second.payload_present = true;
	}
	for (const auto &[id, parent] : physical_parents)
	{
		uint64_t cursor = id;
		size_t depth = 0;
		std::set<uint64_t> seen;
		while (cursor)
		{
			need(++depth <= PLAYER_SNAPSHOT_MAX_DEPTH && seen.insert(cursor).second,
			     ELOOP);
			const auto found = physical_parents.find(cursor);
			need(found != physical_parents.end(), ESTALE);
			cursor = found->second;
		}
		(void)parent;
	}
	auto foreign_children = read(
		connection,
		"SELECT child.id FROM shopkeeper_items parent JOIN shopkeeper_items child ON child.container_id=parent.id "
		"WHERE parent.shopkeeper_id=" +
			std::to_string(keeper) +
			" AND child.shopkeeper_id<>parent.shopkeeper_id LIMIT 1 FOR UPDATE");
	need(!mysql_num_rows(foreign_children.get()), ESTALE);
	if (!current.empty())
	{
		auto duplicate =
			read(connection, "SELECT obj_uid FROM shopkeeper_items WHERE obj_uid IN(" +
						 uids + ") AND shopkeeper_id<>" +
						 std::to_string(keeper) + " LIMIT 1 FOR UPDATE");
		need(!mysql_num_rows(duplicate.get()), ESTALE);
		for (const char *table :
		     { "player_items", "player_pet_items", "locker_items", "account_locker_items",
		       "corpse_items", "saved_items", "siege_items" })
		{
			auto other = read(connection, "SELECT obj_uid FROM " + std::string(table) +
							      " WHERE obj_uid IN(" + uids +
							      ") LIMIT 1 FOR UPDATE");
			need(!mysql_num_rows(other.get()), ESTALE);
		}
	}
	size_t bytes = 0;
	for (const auto &[uid, value] : candidate)
	{
		(void)uid;
		bytes += encode(value.item).size();
		need(bytes <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
	}
	*output = std::move(candidate);
	*literal_out = std::move(current);
}
template <class F> bool attempt(MYSQL *connection, F &&call) noexcept
{
	try
	{
		transaction(connection);
		const auto session = mysql_thread_id(connection);
		call();
		need(mysql_thread_id(connection) == session &&
			     (connection->server_status & SERVER_STATUS_IN_TRANS),
		     ENOTCONN);
		return true;
	}
	catch (const refusal &failure)
	{
		errno = failure.error;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
	}
	catch (...)
	{
		errno = EIO;
	}
	return false;
}
}

bool shop_item_runtime_lock_checkpoint_image(MYSQL *connection, uint64_t keeper, uint32_t shop,
					     int32_t vnum,
					     std::span<const player_item_snapshot> original,
					     shop_item_runtime_image *output) noexcept
{
	if (!output || !keeper || vnum <= 0 || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
	{
		errno = EINVAL;
		return false;
	}
	shop_item_runtime_image candidate;
	if (!attempt(
		    connection,
		    [&]
		    {
			    need(mysql_thread_id(connection) != 0, ENOTCONN);
			    std::vector<player_item_snapshot> items(original.begin(),
								    original.end());
			    std::vector<uint8_t> canonical;
			    const auto code = player_item_snapshot_list_encode(items, &canonical);
			    need(code == player_snapshot_codec_result::ok,
				 code == player_snapshot_codec_result::allocation_failure ? ENOMEM :
											    EINVAL);
			    need(canonical.size() <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
			    std::map<uint64_t, size_t> indices;
			    std::set<int16_t> equipment;
			    std::vector<uint64_t> roots(items.size()), parents(items.size());
			    std::ostringstream selected;
			    selected.exceptions(std::ios::badbit | std::ios::failbit);
			    for (size_t index = 0; index < items.size(); ++index)
			    {
				    const auto &item = items[index];
				    need(item.object_uid && item.object_uid != UINT64_MAX &&
						 item.vnum > 0 &&
						 item.string_mask ==
							 (STRUNG_KEYS | STRUNG_DESC1 |
							  STRUNG_DESC2 | STRUNG_DESC3) &&
						 item.equipment_slot >= 0 &&
						 item.equipment_slot <= MAX_WEAR &&
						 indices.emplace(item.object_uid, index).second,
					 EINVAL);
				    if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
				    {
					    roots[index] = item.object_uid;
					    need(!item.equipment_slot ||
							 equipment.insert(item.equipment_slot)
								 .second,
						 EINVAL);
				    }
				    else
				    {
					    need(item.parent_index >= 0 &&
							 static_cast<size_t>(item.parent_index) <
								 index &&
							 !item.equipment_slot,
						 EINVAL);
					    const auto parent =
						    static_cast<size_t>(item.parent_index);
					    roots[index] = roots[parent];
					    parents[index] = items[parent].object_uid;
				    }
				    selected << (index ? "," : "") << item.object_uid;
			    }
			    const auto selected_uids = selected.str();
			    // Caller has already taken keeper and owner-revision locks; recheck
			    // the exact native keeper identity, never infer it from an item blob.
			    auto keeper_row = read(
				    connection,
				    "SELECT shop_id,mob_vnum,shop_revision,runtime_payload_checkpoint_revision FROM shopkeepers WHERE id=" +
					    std::to_string(keeper) + " LIMIT 2 FOR UPDATE");
			    MYSQL_ROW row = mysql_fetch_row(keeper_row.get());
			    need(row && mysql_num_rows(keeper_row.get()) == 1 &&
					 number<uint32_t>(row[0]) == shop &&
					 number<int32_t>(row[1]) == vnum,
				 ESTALE);
			    const bool checkpointed = row[3] != nullptr;
			    if (checkpointed)
				    need(number<uint64_t>(row[3]) &&
						 number<uint64_t>(row[3]) <=
							 number<uint64_t>(row[2]),
					 ESTALE);
			    std::string custody_scope =
				    "(owner_type=" +
				    std::to_string(
					    static_cast<uint8_t>(item_owner_type::shopkeeper)) +
				    " AND owner_id=" +
				    std::to_string(item_shopkeeper_owner_id(shop)) + " AND state=" +
				    std::to_string(
					    static_cast<uint8_t>(item_custody_state::active)) +
				    ")";
			    if (!items.empty())
				    custody_scope += " OR item_uid IN(" + selected_uids +
						     ") OR ((root_item_uid IN(" + selected_uids +
						     ") OR parent_item_uid IN(" + selected_uids +
						     ")) AND state=" +
						     std::to_string(static_cast<uint8_t>(
							     item_custody_state::active)) +
						     ")";
			    auto custody = read(
				    connection,
				    "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
				    "owner_context_id,item_revision,vnum,state,equipment_slot FROM item_current_owner WHERE " +
					    custody_scope + " ORDER BY item_uid LIMIT " +
					    std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) +
					    " FOR UPDATE");
			    need(mysql_num_rows(custody.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS,
				 E2BIG);
			    need(mysql_num_rows(custody.get()) == items.size(), ESTALE);
			    std::map<uint64_t, uint64_t> revisions;
			    while ((row = mysql_fetch_row(custody.get())))
			    {
				    const auto uid = number<uint64_t>(row[0]);
				    const auto found = indices.find(uid);
				    need(found != indices.end(), ESTALE);
				    const auto index = found->second;
				    const auto revision = number<uint64_t>(row[6]);
				    need(number<uint64_t>(row[1]) == roots[index] &&
						 number<uint64_t>(row[2]) == parents[index] &&
						 number<uint8_t>(row[3]) ==
							 static_cast<uint8_t>(
								 item_owner_type::shopkeeper) &&
						 number<uint64_t>(row[4]) ==
							 item_shopkeeper_owner_id(shop) &&
						 number<uint64_t>(row[5]) == 0 && revision &&
						 number<int32_t>(row[7]) == items[index].vnum &&
						 number<uint8_t>(row[8]) ==
							 static_cast<uint8_t>(
								 item_custody_state::active) &&
						 number<int16_t>(row[9]) ==
							 items[index].equipment_slot &&
						 revisions.emplace(uid, revision).second,
					 ESTALE);
			    }
			    auto physical = read(
				    connection,
				    "SELECT id,obj_uid,vnum,COALESCE(container_id,0),equip_slot FROM shopkeeper_items WHERE shopkeeper_id=" +
					    std::to_string(keeper) + " ORDER BY obj_uid,id LIMIT " +
					    std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) +
					    " FOR UPDATE");
			    need(mysql_num_rows(physical.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS,
				 E2BIG);
			    need(mysql_num_rows(physical.get()) == items.size(), ESTALE);
			    std::map<uint64_t, uint64_t> physical_ids;
			    while ((row = mysql_fetch_row(physical.get())))
			    {
				    const auto id = number<uint64_t>(row[0]),
					       uid = number<uint64_t>(row[1]);
				    const auto found = indices.find(uid);
				    need(id && found != indices.end() &&
						 physical_ids.emplace(id, uid).second,
					 ESTALE);
				    const auto index = found->second;
				    const auto parent = number<uint64_t>(row[3]);
				    const auto slot = number<int16_t>(row[4]);
				    need(number<int32_t>(row[2]) == items[index].vnum &&
						 slot == items[index].equipment_slot &&
						 candidate
							 .emplace(uid,
								  shop_item_runtime_row{
									  id, parent, roots[index],
									  revisions.at(uid), slot,
									  items[index] })
							 .second,
					 ESTALE);
			    }
			    size_t payload_bytes = 0;
			    for (const auto &[uid, value] : candidate)
			    {
				    const auto index = indices.at(uid);
				    const auto parent_uid = parents[index];
				    need(value.parent_id ==
						 (parent_uid ? candidate.at(parent_uid).id : 0),
					 ESTALE);
				    auto children = read(
					    connection,
					    "SELECT id FROM shopkeeper_items WHERE container_id=" +
						    std::to_string(value.id) + " LIMIT " +
						    std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS +
								   1) +
						    " FOR UPDATE");
				    need(mysql_num_rows(children.get()) <=
						 PLAYER_SNAPSHOT_MAX_OBJECTS,
					 E2BIG);
				    size_t expected_children = 0;
				    for (const auto &child : candidate)
					    if (child.second.parent_id == value.id)
						    ++expected_children;
				    need(mysql_num_rows(children.get()) == expected_children,
					 ESTALE);
				    while ((row = mysql_fetch_row(children.get())))
					    need(physical_ids.count(number<uint64_t>(row[0])),
						 ESTALE);
				    // Historical room payload and ledger/reference rows are NOT
				    // physical copies and deliberately remain outside this census.
				    for (const char *other :
					 { "player_pet_items", "locker_items",
					   "account_locker_items", "corpse_items", "saved_items",
					   "siege_items" })
				    {
					    auto duplicate =
						    read(connection, "SELECT obj_uid FROM " +
									     std::string(other) +
									     " WHERE obj_uid=" +
									     std::to_string(uid) +
									     " LIMIT 1 FOR UPDATE");
					    need(!mysql_num_rows(duplicate.get()), ESTALE);
				    }
				    player_item_snapshot retained{};
				    bool present = false;
				    read_payload(connection, true, value.id, &retained, &present);
				    candidate.at(uid).payload_present = present;
				    const bool required = enrolled(connection, uid, true);
				    need(present || (!required && !checkpointed), ENODATA);
				    if (present)
				    {
					    const auto bytes = encode(retained);
					    need(bytes.size() <=
							 PLAYER_SNAPSHOT_MAX_BYTES - payload_bytes,
						 E2BIG);
					    payload_bytes += bytes.size();
					    need(bytes == encode(value.item), ESTALE);
				    }
				    verify(connection, true, value.id, keeper, value.parent_id,
					   value.item, true,
					   present ? physical_proof_policy::strict_payload :
						     physical_proof_policy::legacy_checkpoint);
			    }
		    }))
		return false;
	*output = std::move(candidate);
	return true;
}

namespace
{
// Current facts come only from locked custody, physical rows and canonical
// sidecars. Retained UID order is a value input, not a custody/manifest authority.
void lock_player_image(MYSQL *connection, uint32_t pid, std::span<const uint64_t> original,
		       shop_item_runtime_image *output, std::vector<uint8_t> *canonical_out)
{
	need(mysql_thread_id(connection) != 0, ENOTCONN);
	std::map<uint64_t, size_t> indices;
	std::ostringstream selected;
	selected.exceptions(std::ios::badbit | std::ios::failbit);
	for (size_t index = 0; index < original.size(); ++index)
	{
		need(original[index] && original[index] != UINT64_MAX &&
			     indices.emplace(original[index], index).second,
		     EINVAL);
		selected << (index ? "," : "") << original[index];
	}
	const auto selected_uids = selected.str();
	// Preserve the native save repository's inline-coin separation. Claimed
	// UIDs and all active references to this forest remain included regardless
	// of coin_payload; no physical player_items row is excluded.
	// Caller already holds the complete original GLOBAL custody cut, including
	// all involved player/keeper/related/selected UIDs, BEFORE physical locks.
	// This custody read precedes physical reads; no later custody is discovered.
	auto owner = read(connection, "SELECT pid FROM player_data WHERE pid=" +
					      std::to_string(pid) + " LIMIT 2 FOR UPDATE");
	MYSQL_ROW row = mysql_fetch_row(owner.get());
	need(row && mysql_num_rows(owner.get()) == 1 && number<uint32_t>(row[0]) == pid, ESTALE);
	std::string custody_scope =
		"(owner_type=" + std::to_string(static_cast<uint8_t>(item_owner_type::player)) +
		" AND owner_id=" + std::to_string(pid) + " AND coin_payload IS NULL AND state=" +
		std::to_string(static_cast<uint8_t>(item_custody_state::active)) + ")";
	if (!original.empty())
		custody_scope += " OR item_uid IN(" + selected_uids + ") OR ((root_item_uid IN(" +
				 selected_uids + ") OR parent_item_uid IN(" + selected_uids +
				 ")) AND state=" +
				 std::to_string(static_cast<uint8_t>(item_custody_state::active)) +
				 ")";
	auto custody = read(
		connection,
		"SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
		"owner_context_id,item_revision,vnum,state,equipment_slot FROM item_current_owner WHERE " +
			custody_scope + " ORDER BY item_uid LIMIT " +
			std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(custody.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	need(mysql_num_rows(custody.get()) == original.size(), ESTALE);
	struct custody_value
	{
		uint64_t root = 0, parent = 0, revision = 0;
		int32_t vnum = 0;
		int16_t slot = 0;
	};
	std::map<uint64_t, custody_value> current;
	while ((row = mysql_fetch_row(custody.get())))
	{
		const auto uid = number<uint64_t>(row[0]);
		need(indices.count(uid), ESTALE);
		custody_value value{ number<uint64_t>(row[1]), number<uint64_t>(row[2]),
				     number<uint64_t>(row[6]), number<int32_t>(row[7]),
				     number<int16_t>(row[9]) };
		need(number<uint8_t>(row[3]) == static_cast<uint8_t>(item_owner_type::player) &&
			     number<uint64_t>(row[4]) == pid && number<uint64_t>(row[5]) == 0 &&
			     value.revision && value.vnum > 0 && value.slot >= 0 &&
			     value.slot <= MAX_WEAR &&
			     number<uint8_t>(row[8]) ==
				     static_cast<uint8_t>(item_custody_state::active) &&
			     current.emplace(uid, value).second,
		     ESTALE);
	}
	std::set<int16_t> equipment;
	for (size_t index = 0; index < original.size(); ++index)
	{
		const auto uid = original[index];
		const auto &value = current.at(uid);
		if (!value.parent)
			need(value.root == uid &&
				     (!value.slot || equipment.insert(value.slot).second),
			     ESTALE);
		else
		{
			const auto parent = indices.find(value.parent);
			need(parent != indices.end() && parent->second < index && !value.slot &&
				     value.root == current.at(value.parent).root,
			     ESTALE);
		}
	}
	auto physical = read(
		connection,
		"SELECT id,obj_uid,vnum,COALESCE(container_id,0),equip_slot FROM player_items WHERE pid=" +
			std::to_string(pid) + " ORDER BY obj_uid,id LIMIT " +
			std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) + " FOR UPDATE");
	need(mysql_num_rows(physical.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
	need(mysql_num_rows(physical.get()) == original.size(), ESTALE);
	shop_item_runtime_image candidate;
	std::map<uint64_t, uint64_t> physical_ids;
	while ((row = mysql_fetch_row(physical.get())))
	{
		const auto id = number<uint64_t>(row[0]), uid = number<uint64_t>(row[1]);
		need(id && indices.count(uid) && physical_ids.emplace(id, uid).second, ESTALE);
		const auto &value = current.at(uid);
		const auto parent = number<uint64_t>(row[3]);
		const auto slot = number<int16_t>(row[4]);
		need(number<int32_t>(row[2]) == value.vnum && slot == value.slot &&
			     candidate
				     .emplace(uid, shop_item_runtime_row{ id,
									  parent,
									  value.root,
									  value.revision,
									  slot,
									  {} })
				     .second,
		     ESTALE);
	}
	size_t payload_bytes = 0;
	for (auto &[uid, value] : candidate)
	{
		const auto &actual = current.at(uid);
		need(value.parent_id == (actual.parent ? candidate.at(actual.parent).id : 0),
		     ESTALE);
		auto children =
			read(connection, "SELECT id FROM player_items WHERE container_id=" +
						 std::to_string(value.id) + " LIMIT " +
						 std::to_string(PLAYER_SNAPSHOT_MAX_OBJECTS + 1) +
						 " FOR UPDATE");
		need(mysql_num_rows(children.get()) <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
		size_t expected_children = 0;
		for (const auto &child : candidate)
			if (child.second.parent_id == value.id)
				++expected_children;
		need(mysql_num_rows(children.get()) == expected_children, ESTALE);
		while ((row = mysql_fetch_row(children.get())))
			need(physical_ids.count(number<uint64_t>(row[0])), ESTALE);
		// Historical room payload and ledger/reference rows are not physical copies.
		for (const char *other :
		     { "shopkeeper_items", "player_pet_items", "locker_items",
		       "account_locker_items", "corpse_items", "saved_items", "siege_items" })
		{
			auto duplicate =
				read(connection, "SELECT obj_uid FROM " + std::string(other) +
							 " WHERE obj_uid=" + std::to_string(uid) +
							 " LIMIT 1 FOR UPDATE");
			need(!mysql_num_rows(duplicate.get()), ESTALE);
		}
		player_item_snapshot retained{};
		bool present = false;
		read_payload(connection, false, value.id, &retained, &present);
		// Stored canonical data supplies every value and the original string policy.
		// Base SQL alone cannot fabricate generated keys, timers or metadata order.
		need(present, ENODATA);
		need(retained.object_uid == uid && retained.vnum == actual.vnum &&
			     retained.equipment_slot == value.slot &&
			     !(retained.string_mask &
			       ~(STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3)),
		     ESTALE);
		const auto bytes = encode(retained);
		need(bytes.size() <= PLAYER_SNAPSHOT_MAX_BYTES - payload_bytes, E2BIG);
		payload_bytes += bytes.size();
		// Standalone stored sidecars deliberately have NO_PARENT. Reconstruct
		// only that representation field from matching current physical/custody facts.
		retained.parent_index = actual.parent ?
						static_cast<int32_t>(indices.at(actual.parent)) :
						PLAYER_SNAPSHOT_NO_PARENT;
		value.item = std::move(retained);
		value.payload_present = true;
		verify(connection, false, value.id, pid, value.parent_id, value.item);
	}
	std::vector<player_item_snapshot> items;
	items.reserve(original.size());
	for (const auto uid : original)
		items.push_back(candidate.at(uid).item);
	std::vector<uint8_t> canonical;
	const auto code = player_item_snapshot_list_encode(items, &canonical);
	need(code == player_snapshot_codec_result::ok,
	     code == player_snapshot_codec_result::allocation_failure ? ENOMEM : EILSEQ);
	need(canonical.size() <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
	if (canonical_out)
		*canonical_out = std::move(canonical);
	*output = std::move(candidate);
}
}

bool shop_item_runtime_lock_player_image(MYSQL *connection, uint32_t pid,
					 std::span<const uint64_t> original,
					 shop_item_runtime_image *output) noexcept
{
	if (!output || !pid || pid > INT32_MAX || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
	{
		errno = EINVAL;
		return false;
	}
	shop_item_runtime_image candidate;
	if (!attempt(connection,
		     [&] { lock_player_image(connection, pid, original, &candidate, nullptr); }))
		return false;
	*output = std::move(candidate);
	return true;
}

bool shop_item_runtime_lock_player_image(MYSQL *connection, uint32_t pid,
					 std::span<const player_item_snapshot> original,
					 shop_item_runtime_image *output) noexcept
{
	if (!output || !pid || pid > INT32_MAX || original.size() > PLAYER_SNAPSHOT_MAX_OBJECTS)
	{
		errno = EINVAL;
		return false;
	}
	shop_item_runtime_image candidate;
	if (!attempt(connection,
		     [&]
		     {
			     std::vector<player_item_snapshot> items(original.begin(),
								     original.end());
			     std::vector<uint8_t> expected;
			     const auto code = player_item_snapshot_list_encode(items, &expected);
			     need(code == player_snapshot_codec_result::ok,
				  code == player_snapshot_codec_result::allocation_failure ?
					  ENOMEM :
					  EINVAL);
			     need(expected.size() <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
			     std::vector<uint64_t> uids;
			     std::set<uint64_t> unique;
			     std::set<int16_t> equipment;
			     uids.reserve(items.size());
			     for (size_t index = 0; index < items.size(); ++index)
			     {
				     const auto &item = items[index];
				     need(item.object_uid && item.object_uid != UINT64_MAX &&
						  item.vnum > 0 &&
						  !(item.string_mask &
						    ~(STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 |
						      STRUNG_DESC3)) &&
						  item.equipment_slot >= 0 &&
						  item.equipment_slot <= MAX_WEAR &&
						  unique.insert(item.object_uid).second,
					  EINVAL);
				     if (item.parent_index == PLAYER_SNAPSHOT_NO_PARENT)
					     need(!item.equipment_slot ||
							  equipment.insert(item.equipment_slot)
								  .second,
						  EINVAL);
				     else
					     need(item.parent_index >= 0 &&
							  static_cast<size_t>(item.parent_index) <
								  index &&
							  !item.equipment_slot,
						  EINVAL);
				     uids.push_back(item.object_uid);
			     }
			     std::vector<uint8_t> actual;
			     lock_player_image(connection, pid, uids, &candidate, &actual);
			     need(actual == expected, ESTALE);
		     }))
		return false;
	*output = std::move(candidate);
	return true;
}

bool shop_item_runtime_storage_available(MYSQL *connection, bool *available) noexcept
{
	if (!connection || !available)
	{
		errno = EINVAL;
		return false;
	}
	try
	{
		auto result = read(
			connection,
			"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
			"AND table_name='shopkeeper_item_runtime_state'");
		auto row = mysql_fetch_row(result.get());
		need(row);
		const bool exists = number<uint64_t>(row[0]) == 1;
		if (!exists)
		{
			result = read(
				connection,
				"SELECT COUNT(*) FROM information_schema.tables WHERE table_schema=DATABASE() "
				"AND table_name='critical_operation_inbox'");
			row = mysql_fetch_row(result.get());
			need(row);
			if (number<uint64_t>(row[0]))
			{
				result = read(
					connection,
					"SELECT operation_id FROM critical_operation_inbox WHERE command_type=15 "
					"AND schema_version=2 AND payload_version IN (6,7,8) AND status=1 AND result_code=0 LIMIT 1");
				need(mysql_num_rows(result.get()) == 0, ENODATA);
			}
		}
		*available = exists;
		return true;
	}
	catch (const refusal &failure)
	{
		errno = failure.error;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
	}
	catch (...)
	{
		errno = EIO;
	}
	return false;
}
bool shop_item_runtime_read(MYSQL *connection, bool keeper, uint64_t id,
			    player_item_snapshot *output, bool *present) noexcept
{
	if (!output || !present || !id)
	{
		errno = EINVAL;
		return false;
	}
	player_item_snapshot candidate;
	bool exists = false;
	if (!attempt(connection,
		     [&] { read_payload(connection, keeper, id, &candidate, &exists); }))
		return false;
	if (exists)
		*output = std::move(candidate);
	*present = exists;
	return true;
}
bool shop_item_runtime_verify(MYSQL *connection, bool keeper, uint64_t id, uint64_t owner,
			      uint64_t parent, const player_item_snapshot &item) noexcept
{
	return attempt(connection, [&] { verify(connection, keeper, id, owner, parent, item); });
}
bool shop_item_runtime_write(MYSQL *connection, bool keeper, uint64_t id,
			     const player_item_snapshot &item) noexcept
{
	return attempt(connection, [&] { write_payload(connection, keeper, id, item); });
}
bool shop_item_runtime_keeper_image(MYSQL *connection, uint64_t keeper, uint32_t shop, int32_t vnum,
				    shop_item_runtime_image *output) noexcept
{
	if (!output)
	{
		errno = EINVAL;
		return false;
	}
	shop_item_runtime_image candidate;
	if (!attempt(connection, [&] { keeper_image(connection, keeper, shop, vnum, &candidate); }))
		return false;
	*output = std::move(candidate);
	return true;
}
bool shop_item_runtime_keeper_image(MYSQL *connection, uint64_t keeper, uint32_t shop, int32_t vnum,
				    shop_item_runtime_image *image_out,
				    std::vector<item_ownership_runtime_entry> *custody_out) noexcept
{
	if (!keeper || vnum <= 0 || !image_out || !custody_out)
	{
		errno = EINVAL;
		return false;
	}
	shop_item_runtime_image image;
	std::vector<item_ownership_runtime_entry> custody;
	if (!attempt(
		    connection,
		    [&]
		    {
			    const item_owner_identity owner{ item_owner_type::shopkeeper,
							     item_shopkeeper_owner_id(shop), 0 };
			    need(item_owner_identity_valid(owner), EINVAL);
			    // The caller already serializes this keeper. Lock the actual native
			    // counter before current custody; never invent an optimistic counter.
			    auto counter = read(
				    connection,
				    "SELECT revision FROM item_owner_revision WHERE owner_type=" +
					    std::to_string(static_cast<uint8_t>(owner.type)) +
					    " AND owner_id=" + std::to_string(owner.id) +
					    " AND owner_context_id=0 FOR UPDATE");
			    need(mysql_num_rows(counter.get()) <= 1, ESTALE);
			    const auto counter_row = mysql_fetch_row(counter.get());
			    // Preserve every original keeper enrollment/history, physical,
			    // topology, metadata, sidecar and complete-census predicate.
			    keeper_image(connection, keeper, shop, vnum, &image);
			    need(image.size() <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
			    if (image.empty())
				    return;
			    need(counter_row, ENODATA);
			    const auto owner_revision = number<uint64_t>(counter_row[0]);
			    std::map<uint64_t, uint64_t> physical_uids;
			    std::string uids;
			    for (const auto &[uid, value] : image)
			    {
				    need(uid && uid != UINT64_MAX && value.id && value.root_uid &&
						 value.revision && value.item.object_uid == uid &&
						 value.item.vnum > 0 && value.payload_present &&
						 physical_uids.emplace(value.id, uid).second,
					 ESTALE);
				    uids += (uids.empty() ? "" : ",") + std::to_string(uid);
			    }
			    auto current = read(
				    connection,
				    "SELECT item_uid,root_item_uid,COALESCE(parent_item_uid,0),owner_type,owner_id,"
				    "owner_context_id,item_revision,vnum,state,equipment_slot FROM item_current_owner "
				    "WHERE item_uid IN(" +
					    uids + ") ORDER BY item_uid LIMIT " +
					    std::to_string(image.size() + 1) + " FOR UPDATE");
			    need(mysql_num_rows(current.get()) == image.size(), ESTALE);
			    custody.reserve(image.size());
			    MYSQL_ROW row;
			    while ((row = mysql_fetch_row(current.get())))
			    {
				    const auto uid = number<uint64_t>(row[0]);
				    const auto found = image.find(uid);
				    need(found != image.end(), ESTALE);
				    const auto &value = found->second;
				    const auto parent =
					    value.parent_id ? physical_uids.find(value.parent_id) :
							      physical_uids.end();
				    need(!value.parent_id || parent != physical_uids.end(), ESTALE);
				    const uint64_t parent_uid = value.parent_id ? parent->second :
										  0;
				    need(number<uint64_t>(row[1]) == value.root_uid &&
						 number<uint64_t>(row[2]) == parent_uid &&
						 number<uint8_t>(row[3]) ==
							 static_cast<uint8_t>(owner.type) &&
						 number<uint64_t>(row[4]) == owner.id &&
						 number<uint64_t>(row[5]) == owner.context_id &&
						 number<uint64_t>(row[6]) == value.revision &&
						 number<int32_t>(row[7]) == value.item.vnum &&
						 number<uint8_t>(row[8]) ==
							 static_cast<uint8_t>(
								 item_custody_state::active) &&
						 number<int16_t>(row[9]) == value.slot,
					 ESTALE);
				    custody.push_back({ uid, value.root_uid, parent_uid, owner,
							value.revision, owner_revision,
							value.item.vnum,
							item_custody_state::active });
			    }
		    }))
		return false;
	*image_out = std::move(image);
	*custody_out = std::move(custody);
	return true;
}
bool shop_item_runtime_refresh_image(MYSQL *connection, uint64_t keeper, uint32_t shop,
				     int32_t vnum, char_data *body, shop_item_runtime_image *output,
				     bool *complete_out,
				     std::vector<player_item_snapshot> *literal_out) noexcept
{
	if (!body || !output)
	{
		errno = EINVAL;
		return false;
	}
	shop_item_runtime_image candidate;
	bool complete = false;
	std::vector<player_item_snapshot> literal;
	const bool okay = attempt(
		connection,
		[&]
		{
			auto keeper_row = read(
				connection,
				"SELECT shop_id,mob_vnum,shop_revision,runtime_payload_checkpoint_revision FROM shopkeepers WHERE id=" +
					std::to_string(keeper) + " FOR UPDATE");
			auto row = mysql_fetch_row(keeper_row.get());
			need(row && mysql_num_rows(keeper_row.get()) == 1 &&
				     number<uint32_t>(row[0]) == shop &&
				     number<int32_t>(row[1]) == vnum,
			     ESTALE);
			complete = row[3] != nullptr;
			if (complete)
			{
				need(number<uint64_t>(row[3]) &&
					     number<uint64_t>(row[3]) <= number<uint64_t>(row[2]),
				     ESTALE);
				refresh_marked_image(connection, keeper, shop, body, &candidate,
						     &literal);
				return;
			}
			shop_item_runtime_image original;
			keeper_image(connection, keeper, shop, vnum, &original);
			// DELETE replaces every physical row, including legacy-only parents.
			// A child owned by another keeper must never follow that cascade.
			auto foreign_children = read(
				connection,
				"SELECT child.id FROM shopkeeper_items parent JOIN shopkeeper_items child ON child.container_id=parent.id "
				"WHERE parent.shopkeeper_id=" +
					std::to_string(keeper) +
					" AND child.shopkeeper_id<>parent.shopkeeper_id LIMIT 1 FOR UPDATE");
			need(mysql_num_rows(foreign_children.get()) == 0, ESTALE);
			if (original.empty())
			{
				candidate = {};
				return;
			}
			std::vector<player_item_snapshot> current;
			need(player_item_snapshot_list_capture(body, true, true, false, &current,
							       nullptr) ==
				     player_snapshot_capture_result::ok,
			     EAGAIN);
			std::set<uint64_t> seen;
			for (const auto &item : current)
				need(item.object_uid && seen.insert(item.object_uid).second,
				     ESTALE);
			// Preserve the existing stored literal policy per retained UID. A
			// generic capture of an unstrung live object must not downgrade a
			// retained all-four-literal payload to prototype/null semantics.
			constexpr uint8_t literal_mask = STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 |
							 STRUNG_DESC3;
			size_t literal_bytes = 0;
			const auto capture_literal_root = [&](P_obj root)
			{
				if (!root)
					return;
				const bool needed = std::any_of(
					original.begin(), original.end(),
					[&](const auto &entry)
					{
						return entry.second.root_uid == root->obj_uid &&
						       entry.second.item.string_mask ==
							       literal_mask;
					});
				if (!needed)
					return;
				std::vector<player_item_snapshot> tree_literal;
				need(player_item_snapshot_tree_capture_literal(root, &tree_literal,
									       nullptr) ==
					     player_snapshot_capture_result::ok,
				     EAGAIN);
				for (size_t index = 0; index < tree_literal.size(); ++index)
				{
					auto &item = tree_literal[index];
					const auto saved = original.find(item.object_uid);
					const auto ordinary = std::find_if(
						current.begin(), current.end(),
						[&](const auto &entry)
						{ return entry.object_uid == item.object_uid; });
					need(saved != original.end() && ordinary != current.end() &&
						     saved->second.root_uid == root->obj_uid &&
						     item.vnum == ordinary->vnum,
					     ESTALE);
					need(item.parent_index < 0 ||
						     static_cast<size_t>(item.parent_index) < index,
					     ESTALE);
					need(ordinary->parent_index < 0 ||
						     static_cast<size_t>(ordinary->parent_index) <
							     current.size(),
					     ESTALE);
					const uint64_t literal_parent =
						item.parent_index < 0 ?
							0 :
							tree_literal[static_cast<size_t>(
									     item.parent_index)]
								.object_uid;
					const uint64_t ordinary_parent =
						ordinary->parent_index < 0 ?
							0 :
							current[static_cast<size_t>(
									ordinary->parent_index)]
								.object_uid;
					need(literal_parent == ordinary_parent, ESTALE);
					// Tree capture roots use slot zero and local parent indexes;
					// preserve the existing full-player root slot and ordering.
					item.equipment_slot = ordinary->equipment_slot;
					item.parent_index = ordinary->parent_index;
					auto nonliteral = item;
					nonliteral.string_mask = ordinary->string_mask;
					nonliteral.name = ordinary->name;
					nonliteral.short_description = ordinary->short_description;
					nonliteral.description = ordinary->description;
					nonliteral.action_description =
						ordinary->action_description;
					need(encode(nonliteral) == encode(*ordinary), ESTALE);
					if (saved->second.item.string_mask == literal_mask)
					{
						literal_bytes += encode(item).size();
						need(literal_bytes <= PLAYER_SNAPSHOT_MAX_BYTES,
						     E2BIG);
						*ordinary = item;
					}
				}
			};
			for (int slot = 0; slot < MAX_WEAR; ++slot)
				capture_literal_root(body->equipment[slot]);
			size_t roots = 0;
			for (P_obj root = body->carrying; root; root = root->next_content)
			{
				need(++roots <= PLAYER_SNAPSHOT_MAX_OBJECTS, E2BIG);
				capture_literal_root(root);
			}
			for (auto &[uid, value] : original)
			{
				const auto found = std::find_if(current.begin(), current.end(),
								[&](const auto &item)
								{ return item.object_uid == uid; });
				need(found != current.end() && found->vnum == value.item.vnum &&
					     found->equipment_slot == value.slot,
				     ESTALE);
				const auto parent = found->parent_index;
				need(parent < 0 || static_cast<size_t>(parent) < current.size(),
				     ESTALE);
				const uint64_t parent_uid =
					parent < 0 ?
						0 :
						current[static_cast<size_t>(parent)].object_uid;
				const auto old_parent =
					value.parent_id ?
						std::find_if(original.begin(), original.end(),
							     [&](const auto &entry) {
								     return entry.second.id ==
									    value.parent_id;
							     }) :
						original.end();
				need(parent_uid ==
					     (old_parent == original.end() ? 0 : old_parent->first),
				     ESTALE);
				// Every live child of an enrolled node must be enrolled; no new
				// cascade may be silently adopted by the dirty replacement save.
				for (const auto &child : current)
					if (child.parent_index >= 0 &&
					    current[static_cast<size_t>(child.parent_index)]
							    .object_uid == uid)
						need(original.count(child.object_uid), ESTALE);
				value.item = *found;
				value.item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			}
			size_t total = 0;
			for (const auto &[uid, value] : original)
			{
				total += encode(value.item).size();
				need(total <= PLAYER_SNAPSHOT_MAX_BYTES, E2BIG);
			}
			candidate = std::move(original);
		});
	if (okay)
	{
		*output = std::move(candidate);
		if (literal_out)
			*literal_out = std::move(literal);
		if (complete_out)
			*complete_out = complete;
	}
	return okay;
}
bool shop_item_runtime_restore_stage(obj_data *object, const player_item_snapshot &item) noexcept
{
	try
	{
		need(object && !object->contains && object->obj_uid == item.object_uid &&
			     object->R_num >= 0 &&
			     obj_index[object->R_num].virtual_number == item.vnum,
		     ESTALE);
		// Native SQL metadata has already been verified. The legacy loader
		// prepends stored extra descriptions to prototype descriptions; select
		// exactly the stored canonical descriptors, preserving their order.
		std::vector<extra_descr_data *> remaining, ordered;
		std::set<extra_descr_data *> seen;
		for (auto *entry = object->ex_description; entry; entry = entry->next)
		{
			need(seen.insert(entry).second &&
				     remaining.size() < PLAYER_SNAPSHOT_MAX_ROWS,
			     ELOOP);
			remaining.push_back(entry);
		}
		for (const auto &expected : item.extra_descriptions)
		{
			const auto found = std::find_if(
				remaining.begin(), remaining.end(),
				[&](const auto *entry)
				{
					if (expected.spellbook)
					{
						if (!entry->keyword ||
						    strlen(entry->keyword) != 3 ||
						    entry->keyword[0] != 3 ||
						    entry->keyword[1] != 1 ||
						    entry->keyword[2] != 3 || !entry->description)
							return false;
						for (int skill = 0; skill < MAX_SKILLS; ++skill)
							if (((static_cast<unsigned char>(
								      entry->description[skill /
											 8]) >>
							      (skill % 8)) &
							     1) !=
							    (static_cast<int>(
								    std::find(
									    expected.spell_ids
										    .begin(),
									    expected.spell_ids.end(),
									    skill) !=
								    expected.spell_ids.end())))
								return false;
						return true;
					}
					return entry->keyword && entry->description &&
					       expected.keyword == entry->keyword &&
					       expected.description == entry->description;
				});
			need(found != remaining.end(), ESTALE);
			ordered.push_back(*found);
			remaining.erase(found);
		}
		// Custom empty strings are valid overrides, not prototype references.
		for (const auto &[mask, value, target] :
		     { std::tuple{ 1, &item.name, &object->name },
		       std::tuple{ 4, &item.short_description, &object->short_description },
		       std::tuple{ 2, &item.description, &object->description },
		       std::tuple{ 8, &item.action_description, &object->action_description } })
			if ((item.string_mask & mask) && value->empty() &&
			    !(object->str_mask & mask))
			{
				*target = str_dup("");
				need(*target, ENOMEM);
				object->str_mask |= mask;
			}
		object->extra2_flags = item.extra2_flags;
		object->g_key = item.generated_key;
		object->anti_flags = item.anti_flags;
		object->anti2_flags = item.anti2_flags;
		object->craftsmanship = item.craftsmanship;
		for (size_t index = 1; index < item.timers.size(); ++index)
			object->timer[index] = item.timers[index];
		for (size_t index = 0; index < item.affects.size(); ++index)
		{
			object->affected[index].location = item.affects[index][0];
			object->affected[index].modifier = item.affects[index][1];
		}
		for (auto *entry : remaining)
		{
			if (entry->keyword)
				str_free(entry->keyword);
			if (entry->description)
				str_free(entry->description);
			FREE(entry);
		}
		object->ex_description = ordered.empty() ? nullptr : ordered.front();
		for (size_t i = 0; i < ordered.size(); ++i)
			ordered[i]->next = i + 1 < ordered.size() ? ordered[i + 1] : nullptr;
		std::vector<player_item_snapshot> actual;
		need(player_item_snapshot_tree_capture(object, &actual, nullptr) ==
			     player_snapshot_capture_result::ok,
		     EAGAIN);
		need(actual.size() == 1);
		actual.front().equipment_slot = item.equipment_slot;
		need(encode(actual.front()) == encode(item), ESTALE);
		return true;
	}
	catch (const refusal &failure)
	{
		errno = failure.error;
	}
	catch (const std::bad_alloc &)
	{
		errno = ENOMEM;
	}
	catch (...)
	{
		errno = EIO;
	}
	return false;
}
#endif
