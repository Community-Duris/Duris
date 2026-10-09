#include "persistence/sql_room_creation_correspondence.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include "world/vnum.obj.h"

#include <algorithm>
#include <cerrno>
#include <charconv>
#include <map>
#include <new>
#include <set>
#include <string_view>
#include <type_traits>
#include <tuple>
#include <utility>

namespace
{
struct failure
{
	unsigned int code;
};
void require(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code };
}
const economic_sql_source_table &table(const std::vector<economic_sql_source_table> &tables,
				       std::string_view name)
{
	const auto found = std::find_if(tables.begin(), tables.end(),
					[&](const auto &value) { return value.name == name; });
	require(found != tables.end());
	return *found;
}
size_t column(const economic_sql_source_table &source, std::string_view name)
{
	const auto found = std::find(source.columns.begin(), source.columns.end(), name);
	require(found != source.columns.end());
	return static_cast<size_t>(found - source.columns.begin());
}
bool number(const economic_sql_source_row &row, size_t at, uint64_t &value, bool nullable = false)
{
	if (at >= row.cells.size())
		return false;
	if (!row.cells[at])
	{
		value = 0;
		return nullable;
	}
	const auto &text = *row.cells[at];
	if (text.empty() || (text.size() > 1 && text[0] == '0'))
		return false;
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
	return result.ec == std::errc{} && result.ptr == text.data() + text.size();
}
bool equal(const economic_sql_source_row &row, size_t at, uint64_t wanted, bool nullable = false)
{
	uint64_t value = 0;
	return number(row, at, value, nullable) && value == wanted;
}
bool signed_number(const economic_sql_source_row &row, size_t at, int64_t &value)
{
	if (at >= row.cells.size() || !row.cells[at])
		return false;
	const auto &text = *row.cells[at];
	const auto result = std::from_chars(text.data(), text.data() + text.size(), value);
	return result.ec == std::errc{} && result.ptr == text.data() + text.size() &&
	       text == std::to_string(value);
}
std::string operation(const critical_operation_id &id)
{
	return { reinterpret_cast<const char *>(id.bytes.data()), id.bytes.size() };
}
std::string literal_bytes(player_item_snapshot item)
{
	item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	item.equipment_slot = 0;
	std::vector<uint8_t> encoded;
	const auto result = player_item_snapshot_list_encode({ item }, &encoded);
	if (result == player_snapshot_codec_result::allocation_failure)
		throw std::bad_alloc{};
	require(result == player_snapshot_codec_result::ok);
	return { reinterpret_cast<const char *>(encoded.data()), encoded.size() };
}
using index = std::map<uint64_t, std::vector<size_t>>;
index indexed(const economic_sql_source_table &source, size_t at)
{
	index result;
	for (size_t i = 0; i < source.rows.size(); ++i)
	{
		uint64_t key = 0;
		if (number(source.rows[i], at, key) && key)
			result[key].push_back(i);
	}
	return result;
}
const std::vector<size_t> &rows(const index &source, uint64_t uid)
{
	static const std::vector<size_t> empty;
	const auto found = source.find(uid);
	return found == source.end() ? empty : found->second;
}
void inspect(const economic_sql_physical_source_snapshot &base,
	     const sql_room_item_source_snapshot &room,
	     const sql_room_creation_source_snapshot &creation, size_t maximum_diagnostics,
	     const sql_room_item_source_evidence &original,
	     sql_room_creation_correspondence_evidence &result)
{
	result.current_season = original.current_season;
	result.season_active = original.season_active;
	const auto &custody = table(base.source2.tables, "item_current_owner");
	const auto &equipment = table(base.source2.item_equipment_sources, "item_current_owner");
	const auto &owners = table(base.source2.tables, "item_owner_revision");
	const auto &payloads = room.tables[0], &ledger = room.tables[2],
		   &references = room.tables[3];
	const auto &lineages = table(creation.tables, "economic_lineage_state");
	const auto &epochs = table(creation.tables, "economic_epoch");
	const auto &operations = table(creation.tables, "economic_accounting_operation");
	const auto &effects = table(creation.tables, "economic_accounting_account_effect");
	const auto custody_uid = indexed(custody, 0), custody_root = indexed(custody, 1);
	const auto equipment_uid = indexed(equipment, 0);
	std::map<std::tuple<std::string, uint64_t, uint64_t>, std::vector<size_t>> payload_identity;
	std::map<std::pair<uint64_t, uint64_t>, size_t> payload_revision_count;
	std::map<std::pair<std::string, uint64_t>, std::vector<size_t>> ledger_event,
		reference_event;
	std::map<std::string, std::vector<size_t>> payload_operation;
	std::map<uint64_t, std::vector<size_t>> owner_room;
	std::map<uint64_t, size_t> physical_count;
	std::map<size_t, std::vector<size_t>> original_payload;
	std::map<uint64_t, std::vector<size_t>> witnesses_by_root;
	std::map<std::string, std::vector<size_t>> lineage_rows, operation_rows, effect_rows;
	std::map<std::pair<std::string, std::string>, size_t> books;
	for (size_t i = 0; i < lineages.rows.size(); ++i)
		if (lineages.rows[i].cells[0])
			lineage_rows[*lineages.rows[i].cells[0]].push_back(i);
	for (size_t i = 0; i < operations.rows.size(); ++i)
		if (operations.rows[i].cells[0])
			operation_rows[*operations.rows[i].cells[0]].push_back(i);
	for (size_t i = 0; i < effects.rows.size(); ++i)
		if (effects.rows[i].cells[2])
			effect_rows[*effects.rows[i].cells[2]].push_back(i);
	for (const auto &row : epochs.rows)
		if (row.cells[0] && row.cells[1])
			++books[{ *row.cells[0], *row.cells[1] }];
	auto current_birth_head = [&](const zone_reset_item_source_family &family, uint64_t uid)
	{
		const auto lineage = operation(family.plan.metadata.lineage);
		const auto pointer = lineage_rows.find(lineage);
		if (pointer == lineage_rows.end() || pointer->second.size() != 1)
			return false;
		const auto &state = lineages.rows[pointer->second[0]];
		uint64_t revision = 0;
		if (!state.cells[1] || state.cells[1]->size() != 16 ||
		    !std::any_of(state.cells[1]->begin(), state.cells[1]->end(),
				 [](char value) { return value != 0; }) ||
		    !number(state, 2, revision) || books[{ lineage, *state.cells[1] }] != 1)
			return false;
		economic_account_key key{ family.plan.metadata.lineage, economic_account_kind::pile,
					  uid, 0 };
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> encoded{};
		const auto key_code = economic_account_key_encode(key, &encoded);
		if (key_code == economic_accounting_error::capacity)
			throw std::bad_alloc{};
		if (key_code != economic_accounting_error::ok)
			return false;
		const std::string key_bytes{ reinterpret_cast<const char *>(encoded.data()),
					     encoded.size() };
		const auto pile = effect_rows.find(key_bytes);
		if (pile == effect_rows.end())
			return false;
		size_t included = 0;
		bool exact = false;
		for (const auto at : pile->second)
		{
			const auto &effect = effects.rows[at];
			const auto op = effect.cells[0] ? operation_rows.find(*effect.cells[0]) :
							  operation_rows.end();
			if (op == operation_rows.end() || op->second.size() != 1)
			{
				++included;
				exact = false;
				continue;
			}
			const auto &row = operations.rows[op->second[0]];
			bool shaped = effect.cells[0]->size() == 16 && row.cells[1] &&
				      row.cells[1]->size() == 16 && row.cells[2] &&
				      row.cells[2]->size() == 16;
			economic_coin_vector before{}, after{}, ignored{};
			for (size_t denomination = 0; denomination < before.size(); ++denomination)
			{
				if (!signed_number(effect, 3 + denomination,
						   before[denomination]) ||
				    !signed_number(effect, 7 + denomination, after[denomination]))
					shaped = false;
			}
			uint64_t account_index = 0, before_revision = 0, after_revision = 0;
			if (!number(effect, 1, account_index) ||
			    !number(effect, 11, before_revision) ||
			    !number(effect, 12, after_revision) ||
			    after_revision < before_revision ||
			    economic_coin_delta(before, after, &ignored) !=
				    economic_accounting_error::ok)
				shaped = false;
			const bool book = row.cells[1] && row.cells[2] &&
					  books[{ *row.cells[1], *row.cells[2] }] == 1;
			const bool successful = equal(row, 17, 1) && equal(row, 18, 0);
			const bool same_lineage = row.cells[1] && *row.cells[1] == lineage;
			const bool active = row.cells[2] && *row.cells[2] == *state.cells[1];
			// Mirror the cold reader's LEFT JOIN predicate; malformed/orphan,
			// wrong-book, failed and active-book effects cannot vanish behind
			// an old birth. Valid other historical books remain history.
			if (shaped && book && successful && same_lineage && !active)
				continue;
			++included;
			exact = shaped && book && successful && same_lineage && active &&
				*effect.cells[0] == operation(family.operation) &&
				equal(effect, 12, 1);
		}
		return included == 1 && exact;
	};
	for (size_t i = 0; i < original.witnesses.size(); ++i)
		if (original.witnesses[i].payload_row != SIZE_MAX)
			original_payload[original.witnesses[i].payload_row].push_back(i);
	auto finding = [&](size_t at, uint32_t flags)
	{
		if (!flags)
			return;
		if (at == SIZE_MAX)
			result.global_flags |= flags;
		if (result.diagnostics.size() < maximum_diagnostics)
			result.diagnostics.push_back({ flags, at });
		else
			result.diagnostics_truncated = true;
	};
	if (!result.season_active)
		finding(SIZE_MAX, SQL_ROOM_SOURCE_MALFORMED_IDENTITY);
	auto event_index =
		[&](const economic_sql_source_table &source, size_t op, size_t event, auto &target)
	{
		for (size_t i = 0; i < source.rows.size(); ++i)
		{
			const auto &row = source.rows[i];
			uint64_t value = 0;
			if (row.cells[op] && number(row, event, value))
				target[{ *row.cells[op], value }].push_back(i);
		}
	};
	event_index(ledger, 0, 1, ledger_event);
	event_index(references, 7, 8, reference_event);
	for (size_t i = 0; i < payloads.rows.size(); ++i)
	{
		const auto &row = payloads.rows[i];
		uint64_t uid = 0, revision = 0;
		if (row.cells[3])
			payload_operation[*row.cells[3]].push_back(i);
		if (number(row, 0, uid) && uid && number(row, 1, revision) && revision)
		{
			++payload_revision_count[{ uid, revision }];
			if (row.cells[3])
				payload_identity[{ *row.cells[3], uid, revision }].push_back(i);
		}
	}
	for (size_t i = 0; i < owners.rows.size(); ++i)
	{
		uint64_t room_id = 0;
		if (equal(owners.rows[i], 0, 3) && number(owners.rows[i], 1, room_id) && room_id &&
		    equal(owners.rows[i], 2, 0))
			owner_room[room_id].push_back(i);
	}
	auto physical = [&](const economic_sql_source_table &source, const char *key,
			    const char *status = nullptr, bool auction = false)
	{
		const auto uid_at = column(source, key);
		const auto state_at = status ? column(source, status) : 0;
		for (const auto &row : source.rows)
		{
			uint64_t uid = 0, state = 0;
			if (status && (!number(row, state_at, state) || (auction && state > 1)))
			{
				finding(SIZE_MAX, SQL_ROOM_SOURCE_MALFORMED_IDENTITY);
				continue;
			}
			if (status && (auction ? state != 0 : state != 2 && state != 3))
				continue;
			if (number(row, uid_at, uid) && uid)
				++physical_count[uid];
			else
				finding(SIZE_MAX, SQL_ROOM_SOURCE_MALFORMED_IDENTITY);
		}
	};
	for (const auto &source : base.physical_sources)
		if (std::find(source.columns.begin(), source.columns.end(), "obj_uid") !=
		    source.columns.end())
			physical(source, "obj_uid");
	for (const auto &source : base.source2.item_sources)
		physical(source, "obj_uid");
	physical(table(base.source2.tables, "auction_item_custody"), "item_uid",
		 "claimed_at IS NOT NULL", true);
	physical(table(base.source2.tables, "collector_listings"), "item_uid", "status");
	physical(table(creation.tables, "player_death_restitution_runtime"), "item_uid");
	std::set<size_t> superseded;
	std::set<uint64_t> current_roots;
	for (size_t family_index = 0; family_index < result.families.size(); ++family_index)
	{
		const auto &family = result.families[family_index];
		if (family.image.items.empty())
		{
			finding(SIZE_MAX, SQL_ROOM_SOURCE_MISSING_PROOF);
			continue;
		}
		const auto root = family.image.items[0].object_uid;
		const auto op = operation(family.operation);
		// A failed family grants no current interpretation. Keep its full decoded
		// image and operation-linked failures, while the unchanged original5
		// evidence retains actual custody/literal/graph diagnostics. Do not rescan
		// shared descendant histories once for every malformed duplicate family.
		if (family.flags)
		{
			for (size_t i = 0; i < family.image.items.size(); ++i)
			{
				const auto &item = family.image.items[i];
				sql_room_creation_source_witness witness;
				witness.family_index = family_index;
				witness.image_item = i;
				witness.room.item_uid = item.object_uid;
				witness.room.item_revision = 1;
				witness.room.root_item_uid = root;
				witness.room.season_epoch = family.image.season_epoch;
				witness.room.room = static_cast<uint64_t>(family.image.room_vnum);
				witness.room.flags = SQL_ROOM_SOURCE_MISSING_PROOF;
				witness.room.literal = item;
				witness.room.literal->parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				if (family.image.season_epoch != result.current_season)
					witness.room.flags |= SQL_ROOM_SOURCE_OLD_SEASON;
				result.witnesses.push_back(std::move(witness));
			}
			if (!critical_operation_id_is_zero(family.operation))
				for (const auto at : payload_operation[op])
				{
					sql_room_creation_source_witness witness;
					witness.family_index = family_index;
					witness.room.payload_row = at;
					witness.room.flags = SQL_ROOM_SOURCE_MISSING_PROOF;
					number(payloads.rows[at], 0, witness.room.item_uid);
					result.witnesses.push_back(std::move(witness));
				}
			continue;
		}
		std::map<uint64_t, size_t> current_witness;
		bool malformed_current_member = false;
		std::set<size_t> recognized_payloads;
		const auto &family_custody = rows(custody_root, root);
		const auto &root_rows = rows(custody_uid, root);
		const bool current_birth_root = root_rows.size() == 1 &&
						equal(custody.rows[root_rows[0]], 3, 3) &&
						equal(custody.rows[root_rows[0]], 8, 1) &&
						equal(custody.rows[root_rows[0]], 6, 1);
		const bool room_family = std::any_of(
			family_custody.begin(), family_custody.end(),
			[&](const auto at)
			{
				return equal(custody.rows[at], 3, 3) &&
				       equal(custody.rows[at], 8, 1) &&
				       (equal(custody.rows[at], 6, 1) || current_birth_root);
			});
		for (size_t i = 0; i < family.image.items.size(); ++i)
		{
			const auto &item = family.image.items[i];
			sql_room_creation_source_witness witness;
			witness.family_index = family_index;
			witness.image_item = i;
			auto &value = witness.room;
			value.item_uid = item.object_uid;
			value.item_revision = 1;
			value.root_item_uid = root;
			value.season_epoch = family.image.season_epoch;
			value.room = static_cast<uint64_t>(family.image.room_vnum);
			value.owner_revision = family.result.to_owner_revision;
			value.parent_item_uid =
				item.parent_index == PLAYER_SNAPSHOT_NO_PARENT ?
					0 :
					(item.parent_index >= 0 && static_cast<size_t>(
									   item.parent_index) < i ?
						 family.image.items[item.parent_index].object_uid :
						 0);
			value.literal = item;
			value.literal->parent_index = PLAYER_SNAPSHOT_NO_PARENT;
			if (item.type < ITEM_LOWEST || item.type > ITEM_LAST ||
			    item.type == ITEM_CORPSE || (item.extra_flags & ITEM_ARTIFACT) ||
			    item.equipment_slot ||
			    item.string_mask !=
				    (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) ||
			    ((item.type == ITEM_MONEY) != (item.vnum == VOBJ_COINS)))
				value.flags |= SQL_ROOM_SOURCE_MALFORMED_LITERAL;
			if (family.flags)
				value.flags |= SQL_ROOM_SOURCE_MISSING_PROOF;
			if (value.season_epoch != result.current_season)
				value.flags |= SQL_ROOM_SOURCE_OLD_SEASON;
			const auto encoded = literal_bytes(item);
			size_t payload_count = 0;
			const auto payload_entries =
				payload_identity.find({ op, item.object_uid, 1 });
			if (payload_entries != payload_identity.end())
				for (const auto at : payload_entries->second)
				{
					const auto &row = payloads.rows[at];
					if (row.cells[3] && *row.cells[3] == op && equal(row, 1, 1))
					{
						++payload_count;
						value.payload_row = at;
						recognized_payloads.insert(at);
						if (!equal(row, 2, SQL_ROOM_ITEM_PAYLOAD_VERSION) ||
						    !equal(row, 4, value.season_epoch) ||
						    !row.cells[5] || *row.cells[5] != encoded)
							value.flags |=
								SQL_ROOM_SOURCE_MALFORMED_LITERAL;
					}
				}
			if (!payload_count)
				value.flags |= SQL_ROOM_SOURCE_MISSING_LITERAL;
			if (payload_count > 1)
				value.flags |= SQL_ROOM_SOURCE_AMBIGUOUS_PROOF;
			if (payload_revision_count[{ item.object_uid, 1 }] > 1)
				value.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
			const auto led = ledger_event.find({ op, i }),
				   ref = reference_event.find({ op, i });
			if (led != ledger_event.end() && led->second.size() == 1)
				value.ledger_row = led->second[0];
			if (ref != reference_event.end() && ref->second.size() == 1)
				value.reference_row = ref->second[0];
			const auto &selected = rows(custody_uid, item.object_uid);
			bool current = false;
			if (selected.empty())
				value.flags |= SQL_ROOM_SOURCE_MISSING_CUSTODY;
			else if (selected.size() != 1)
				value.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
			else
			{
				value.custody_row = selected[0];
				const auto &row = custody.rows[selected[0]];
				for (size_t field = 0; field < 9; ++field)
				{
					uint64_t observed = 0;
					if (!number(row, field, observed, field == 2))
						value.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
				}
				current = room_family && equal(row, 1, root) && equal(row, 8, 1) &&
					  (equal(row, 6, 1) || current_birth_root);
				if (current)
				{
					current_roots.insert(root);
					if (!equal(row, 2, value.parent_item_uid, true) ||
					    !equal(row, 3, 3) || !equal(row, 4, value.room) ||
					    !equal(row, 5, 0) || !equal(row, 6, 1) ||
					    !equal(row, 7, static_cast<uint64_t>(item.vnum)))
						value.flags |= SQL_ROOM_SOURCE_STALE_PROVENANCE;
					const auto &slots = rows(equipment_uid, item.object_uid);
					if (slots.size() != 1 ||
					    !equal(equipment.rows[slots[0]], 1, 0))
						value.flags |= SQL_ROOM_SOURCE_MALFORMED_IDENTITY;
					if (item.type == ITEM_MONEY)
					{
						if (!row.cells[9] || *row.cells[9] != encoded)
							value.flags |=
								SQL_ROOM_SOURCE_MALFORMED_LITERAL;
						if (!current_birth_head(family, item.object_uid))
							value.flags |=
								SQL_ROOM_CREATION_MISSING_CURRENT_BOOK;
					}
					else if (row.cells[9])
						value.flags |= SQL_ROOM_SOURCE_MALFORMED_LITERAL;
					const auto owner = owner_room.find(value.room);
					if (owner == owner_room.end() ||
					    owner->second.size() != 1 ||
					    !number(owners.rows[owner->second[0]], 3,
						    value.owner_revision) ||
					    value.owner_revision <
						    family.result.to_owner_revision ||
					    !value.owner_revision)
						value.flags |=
							SQL_ROOM_SOURCE_MISSING_OWNER_REVISION;
					if (physical_count[item.object_uid])
						value.flags |= SQL_ROOM_CREATION_COMPETING_PHYSICAL;
					if (!result.season_active ||
					    value.season_epoch != result.current_season)
						value.flags |= SQL_ROOM_SOURCE_OLD_SEASON;
				}
				else if (!equal(row, 3, 3) || !equal(row, 1, root))
					value.flags |= SQL_ROOM_SOURCE_MOVED_FROM_ROOM;
			}
			value.flags |= current ? SQL_ROOM_SOURCE_CURRENT : SQL_ROOM_SOURCE_HISTORY;
			witness.exact_current = value.flags == SQL_ROOM_SOURCE_CURRENT;
			const auto at = result.witnesses.size();
			if (current)
				current_witness[item.object_uid] = at;
			// Historical authentication supersedes only the exact birth row. Current
			// defects remain in this new witness, never erased from the candidate.
			if (!family.flags && payload_count == 1 &&
			    payload_revision_count[{ item.object_uid, 1 }] == 1 &&
			    !(value.flags & SQL_ROOM_SOURCE_MALFORMED_LITERAL))
				for (const auto old : original_payload[value.payload_row])
					if (original.witnesses[old].item_uid == item.object_uid &&
					    original.witnesses[old].item_revision == 1)
						superseded.insert(old);
			result.witnesses.push_back(std::move(witness));
		}
		// Retain every extra/malformed literal tied to this family, not just the
		// canonical image subset. Such rows cannot be hidden by a valid root.
		for (const auto at : payload_operation[op])
			if (!recognized_payloads.count(at))
			{
				sql_room_creation_source_witness witness;
				witness.family_index = family_index;
				witness.room.payload_row = at;
				witness.room.flags = SQL_ROOM_SOURCE_MALFORMED_LITERAL |
						     SQL_ROOM_SOURCE_MISSING_PROOF;
				number(payloads.rows[at], 0, witness.room.item_uid);
				result.witnesses.push_back(std::move(witness));
			}
		// All actual current descendants participate, including foreign additions
		// and damaged members absent from the retained original image.
		for (const auto at : family_custody)
		{
			const auto &row = custody.rows[at];
			uint64_t uid = 0;
			if (!room_family || !equal(row, 8, 1) ||
			    (!equal(row, 6, 1) && !current_birth_root))
				continue;
			current_roots.insert(root);
			if (number(row, 0, uid) && current_witness.count(uid))
				continue;
			sql_room_creation_source_witness witness;
			witness.family_index = family_index;
			witness.room.item_uid = uid;
			witness.room.root_item_uid = root;
			witness.room.custody_row = at;
			witness.room.flags =
				SQL_ROOM_SOURCE_CURRENT | SQL_ROOM_SOURCE_MISSING_PROOF |
				SQL_ROOM_SOURCE_MISSING_LITERAL | SQL_ROOM_SOURCE_BAD_GRAPH;
			const auto index = result.witnesses.size();
			result.witnesses.push_back(std::move(witness));
			if (uid)
				current_witness[uid] = index;
			else
				malformed_current_member = true;
		}
		if (current_witness.empty())
			continue;
		sql_room_item_source_graph graph;
		graph.root_item_uid = root;
		graph.room = static_cast<uint64_t>(family.image.room_vnum);
		graph.valid = !family.flags && !malformed_current_member;
		if (current_witness.size() > ITEM_TRANSFER_MAX_ITEMS)
			graph.valid = false;
		const auto root_witness = current_witness.find(root);
		if (root_witness == current_witness.end())
			graph.valid = false;
		else
			graph.owner_revision =
				result.witnesses[root_witness->second].room.owner_revision;
		std::map<uint64_t, std::vector<size_t>> children;
		for (const auto &[uid, at] : current_witness)
		{
			const auto &value = result.witnesses[at].room;
			if (!result.witnesses[at].exact_current || value.room != graph.room ||
			    value.owner_revision != graph.owner_revision ||
			    (uid == root ? value.parent_item_uid != 0 :
					   !current_witness.count(value.parent_item_uid)))
				graph.valid = false;
			children[value.parent_item_uid].push_back(at);
		}
		// Foreign links to a present member are a damaged graph even if their root
		// claims some other family. Check once against the full custody projection.
		std::vector<std::pair<size_t, size_t>> pending;
		if (root_witness != current_witness.end())
			pending.push_back({ root_witness->second, 1 });
		std::set<size_t> visited;
		std::vector<player_item_snapshot> encoded_graph;
		std::map<uint64_t, int32_t> encoded_positions;
		size_t bytes = 0;
		for (size_t next = 0; next < pending.size(); ++next)
		{
			const auto [at, depth] = pending[next];
			if (!visited.insert(at).second || depth > PLAYER_SNAPSHOT_MAX_DEPTH)
			{
				graph.valid = false;
				continue;
			}
			graph.witness_indices.push_back(at);
			const auto &value = result.witnesses[at].room;
			if (!value.literal)
				graph.valid = false;
			else
			{
				auto item = *value.literal;
				item.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
				if (value.parent_item_uid)
				{
					const auto parent =
						encoded_positions.find(value.parent_item_uid);
					if (parent == encoded_positions.end())
						graph.valid = false;
					else
						item.parent_index = parent->second;
				}
				encoded_positions.emplace(
					item.object_uid,
					static_cast<int32_t>(encoded_graph.size()));
				encoded_graph.push_back(std::move(item));
				if (value.payload_row != SIZE_MAX &&
				    payloads.rows[value.payload_row].cells[5])
				{
					const auto length =
						payloads.rows[value.payload_row].cells[5]->size();
					if (length > SQL_ROOM_ITEM_GRAPH_MAX_BYTES - bytes)
						graph.valid = false;
					else
						bytes += length;
				}
			}
			for (const auto child : children[value.item_uid])
				pending.push_back({ child, depth + 1 });
		}
		if (visited.size() != current_witness.size())
			graph.valid = false;
		std::vector<uint8_t> canonical;
		const auto graph_code = player_item_snapshot_list_encode(encoded_graph, &canonical);
		if (graph_code == player_snapshot_codec_result::allocation_failure)
			throw std::bad_alloc{};
		if (graph_code != player_snapshot_codec_result::ok ||
		    canonical.size() > SQL_ROOM_ITEM_GRAPH_MAX_BYTES)
			graph.valid = false;
		if (!graph.valid)
			for (const auto &[uid, at] : current_witness)
			{
				result.witnesses[at].room.flags |= SQL_ROOM_SOURCE_BAD_GRAPH;
				result.witnesses[at].exact_current = false;
			}
		result.graphs.push_back(std::move(graph));
	}
	// Cross-root parent references are checked in one linear indexed pass.
	std::map<uint64_t, std::vector<size_t>> present;
	for (size_t at = 0; at < result.witnesses.size(); ++at)
		if (result.witnesses[at].room.flags & SQL_ROOM_SOURCE_CURRENT)
		{
			present[result.witnesses[at].room.item_uid].push_back(at);
			witnesses_by_root[result.witnesses[at].room.root_item_uid].push_back(at);
		}
	std::set<uint64_t> foreign_roots;
	for (const auto &row : custody.rows)
	{
		uint64_t parent = 0, root = 0;
		if (!number(row, 2, parent, true) || !parent || !present.count(parent))
			continue;
		if (!number(row, 1, root))
			root = 0;
		for (const auto at : present[parent])
			if (root != result.witnesses[at].room.root_item_uid)
				foreign_roots.insert(result.witnesses[at].room.root_item_uid);
	}
	for (auto &graph : result.graphs)
		if (foreign_roots.count(graph.root_item_uid))
		{
			graph.valid = false;
			for (const auto at : witnesses_by_root[graph.root_item_uid])
			{
				result.witnesses[at].room.flags |= SQL_ROOM_SOURCE_BAD_GRAPH;
				result.witnesses[at].exact_current = false;
			}
		}
	// Count the union with unchanged original current roots, not a separate allowance.
	for (const auto &graph : original.graphs)
		current_roots.insert(graph.root_item_uid);
	if (current_roots.size() > SQL_ROOM_ITEM_ROOT_MAX)
	{
		result.native_root_limit_exceeded = true;
		for (auto &graph : result.graphs)
			graph.valid = false;
		for (auto &witness : result.witnesses)
			if (witness.room.flags & SQL_ROOM_SOURCE_CURRENT)
			{
				witness.room.flags |= SQL_ROOM_SOURCE_NATIVE_ROOT_LIMIT;
				witness.exact_current = false;
			}
	}
	result.superseded_room_witness_indices.assign(superseded.begin(), superseded.end());
	for (size_t i = 0; i < result.witnesses.size(); ++i)
		finding(i, result.witnesses[i].room.flags &
				   ~static_cast<uint32_t>(SQL_ROOM_SOURCE_CURRENT |
							  SQL_ROOM_SOURCE_HISTORY |
							  SQL_ROOM_SOURCE_MOVED_FROM_ROOM));
}
}
unsigned int sql_room_creation_correspondence_inspect(
	const economic_sql_physical_source_snapshot &base,
	const sql_room_item_source_snapshot &room,
	const sql_room_creation_source_snapshot &creation, const economic_sql_source_limits &limits,
	size_t maximum_diagnostics, sql_room_creation_correspondence_evidence *output) noexcept
{
	try
	{
		require(output && maximum_diagnostics && maximum_diagnostics <= 512, EINVAL);
		sql_room_creation_correspondence_evidence result;
		auto code = zone_reset_item_source_inspect(base, room, creation, limits,
							   &result.families);
		require(!code, code);
		sql_room_item_source_evidence original;
		code = sql_room_item_payload_inspect_sources(base, room.tables, limits,
							     maximum_diagnostics, &original);
		require(!code, code);
		inspect(base, room, creation, maximum_diagnostics, original, result);
		static_assert(
			std::is_nothrow_move_assignable_v<sql_room_creation_correspondence_evidence>);
		*output = std::move(result);
		return 0;
	}
	catch (const failure &error)
	{
		return error.code;
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
