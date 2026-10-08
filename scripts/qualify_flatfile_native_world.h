// Independent world literals and custody comparison; no native storage calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_WORLD_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_WORLD_H

#include "qualify_flatfile_native_custody.h"

namespace restore_native_world
{
using namespace restore_native_custody;
struct world_owner
{
	owner location;
	uint64_t revision;
	std::array<int32_t, 4> money;
	std::vector<item_literal> items;
};
struct world_catalog
{
	uint32_t version;
	uint64_t revision;
	size_t corpses, saved, rooms;
	std::vector<world_owner> records;
};
inline std::string printable(reader &in, size_t maximum, bool required, bool canonical = false)
{
	const auto value = text(in, maximum);
	need(!required || !value.empty());
	for (auto c : value)
		need(c >= 0x20 && c != 0x7f && (!canonical || c < 'A' || c > 'Z'));
	return { reinterpret_cast<const char *>(value.data()), value.size() };
}
// All item spans borrow encoded. Metadata aliases are validated, never exported.
inline world_catalog decode_world(std::span<const uint8_t> encoded)
{
	const auto file = unwrap(encoded, "DURWRLD", 256 * 1024 * 1024, 3);
	reader in{ file.body };
	world_catalog result{ file.version,
			      file.revision,
			      static_cast<size_t>(in.number(4)),
			      static_cast<size_t>(in.number(4)),
			      file.version >= 3 ? static_cast<size_t>(in.number(4)) : 0,
			      {} };
	need(result.corpses <= 262144 && result.saved <= 262144 && result.rooms <= 262144);
	std::set<uint64_t> uids;
	const auto items = [&]()
	{
		const auto size = in.number(4);
		need(size && size <= 4 * 1024 * 1024);
		auto values = decode_items(in.take(size));
		for (const auto &row : values)
			need(row.uid && row.vnum > 0 && row.equipment == -1 &&
			     uids.insert(row.uid).second);
		return values;
	};
	const auto money = [&]()
	{
		std::array<int32_t, 4> values{};
		for (auto &value : values)
		{
			value = signed32(in);
			need(value >= 0);
		}
		return values;
	};
	std::set<std::string> owner_names;
	uint64_t previous_corpse = 0;
	uint32_t previous_pid = 0;
	std::string previous_name;
	for (size_t i = 0; i < result.corpses; ++i)
	{
		const auto pid = in.number(4);
		const auto name = printable(in, 255, true, true);
		const auto save = in.number(4);
		const auto room = signed32(in);
		const auto id = (pid << 32) | save;
		need(pid && save && room >= 0 && id > previous_corpse);
		if (pid == previous_pid)
			need(name == previous_name);
		else
			need(owner_names.insert(name).second);
		(void)printable(in, 512, false);
		(void)printable(in, 64 * 1024, false);
		(void)printable(in, 512, false);
		(void)in.take(4 + 8 * 4); // Native signed weight and eight corpse values.
		const auto coins = file.version >= 2 ? money() : std::array<int32_t, 4>{};
		const auto revision = in.number(8);
		need(revision);
		result.records.push_back({ { 4, id, 0 }, revision, coins, items() });
		previous_corpse = id;
		previous_pid = pid;
		previous_name = name;
	}
	std::string previous_key;
	for (size_t i = 0; i < result.saved; ++i)
	{
		const auto key = printable(in, 100, true);
		const auto room = signed32(in);
		const auto revision = in.number(8);
		need(room > 0 && revision && (i == 0 || previous_key < key));
		auto values = items();
		need(!values.empty());
		result.records.push_back(
			{ { 3, static_cast<uint64_t>(room), 0 }, revision, {}, std::move(values) });
		previous_key = key;
	}
	int32_t previous_room = 0;
	for (size_t i = 0; i < result.rooms; ++i)
	{
		const auto room = signed32(in);
		const auto revision = in.number(8);
		need(room > previous_room && revision);
		const auto coins = money();
		result.records.push_back(
			{ { 3, static_cast<uint64_t>(room), 0 }, revision, coins, items() });
		previous_room = room;
	}
	in.done();
	return result;
}
struct finding
{
	const char *code;
	uint64_t uid;
};
struct result
{
	bool world_present = false, custody_present = false;
	uint32_t world_version = 0;
	uint64_t world_revision = 0;
	size_t corpses = 0, saved = 0, rooms = 0, world_items = 0, world_coins = 0,
	       compared_items = 0, custody_world_items = 0, other_custody_items = 0,
	       finding_count = 0;
	std::map<std::string, size_t> finding_counts;
	std::vector<finding> findings;
	bool valid() const { return !finding_count; }
	bool verified() const { return world_present && custody_present && valid(); }
};
inline result audit(const std::filesystem::path &root, audit_budget &budget,
		    size_t detail_limit = 100)
{
	need(detail_limit <= 100);
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	const auto domains = root / "domains";
	const auto read = [&](const char *name, size_t maximum)
	{
		struct stat info = {};
		if (lstat((domains / name).c_str(), &info) != 0)
		{
			need(errno == ENOENT);
			return bytes{};
		}
		need(lock.locked());
		auto encoded = file_bytes(domains, name, maximum);
		need(!encoded.empty());
		return encoded;
	};
	const auto custody_bytes = read("item_ownership", catalog_limit);
	const auto world_bytes = read("world_item_catalog", 256 * 1024 * 1024);
	result output;
	output.custody_present = !custody_bytes.empty();
	output.world_present = !world_bytes.empty();
	restore_native_custody::catalog custody{};
	world_catalog world{};
	if (output.custody_present)
		custody = decode_catalog(custody_bytes);
	if (output.world_present)
		world = decode_world(world_bytes);
	output.world_version = world.version;
	output.world_revision = world.revision;
	output.corpses = world.corpses;
	output.saved = world.saved;
	output.rooms = world.rooms;
	const auto issue = [&](const char *code, uint64_t uid = 0)
	{
		++output.finding_count;
		++output.finding_counts[code];
		if (output.findings.size() < detail_limit)
			output.findings.push_back({ code, uid });
	};
	std::map<uint64_t, const item *> owned;
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		owned.emplace(row.uid, &row);
		if (row.state == 2)
			continue;
		if (row.location.type == 3 || row.location.type == 4)
			++output.custody_world_items;
		else
			++output.other_custody_items;
	}
	std::set<uint64_t> seen;
	for (const auto &record : world.records)
		for (size_t i = 0; i < record.items.size(); ++i)
		{
			audit_checkpoint();
			const auto &literal = record.items[i];
			++output.world_items;
			seen.insert(literal.uid);
			if (literal.type == 20)
			{
				++output.world_coins;
				if (std::any_of(literal.values.begin(), literal.values.begin() + 4,
						[](auto value) { return value < 0; }))
					issue("world_negative_coin_value", literal.uid);
			}
			const auto found = owned.find(literal.uid);
			if (found == owned.end())
			{
				issue("world_uid_unadmitted", literal.uid);
				continue;
			}
			++output.compared_items;
			const auto &row = *found->second;
			if (row.state != 1)
				issue("world_uid_not_active", row.uid);
			if (row.location != record.location)
				issue("world_owner_mismatch", row.uid);
			size_t root_index = i;
			while (record.items[root_index].parent >= 0)
				root_index = record.items[root_index].parent;
			const auto parent = literal.parent < 0 ? 0 :
								 record.items[literal.parent].uid;
			if (row.root != record.items[root_index].uid || row.parent != parent)
				issue("world_item_topology_mismatch", row.uid);
			if (row.vnum != literal.vnum)
				issue("world_item_vnum_mismatch", row.uid);
			if (row.equipment)
				issue("world_item_equipment_mismatch", row.uid);
			// Detached inline coin framing changes only the signed parent index.
			// Compare every remaining literal byte, including equipment and strings.
			if (!row.coin_payload.empty() &&
			    !same(row.coin_payload.subspan(8), literal.encoded.subspan(4)))
				issue("world_coin_literal_mismatch", row.uid);
		}
	if (output.world_items && !output.custody_present)
		issue("world_custody_catalog_missing");
	if (output.custody_world_items && !output.world_present)
		issue("custody_world_catalog_missing");
	for (const auto &row : custody.items)
	{
		audit_checkpoint();
		if (row.state != 2 && (row.location.type == 3 || row.location.type == 4) &&
		    !seen.contains(row.uid))
			issue("world_uid_missing_literal", row.uid);
	}
	lock.no_pending_player_domains();
	lock.finish();
	budget.checkpoint();
	return output;
}
} // namespace restore_native_world
#endif
