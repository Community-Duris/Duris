// Independent current player and pet literals. No storage, recovery or codec calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_PLAYER_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_PLAYER_H

#include "qualify_flatfile_native_custody.h"
#include <charconv>

namespace restore_native_player
{
using namespace restore_native_custody;
constexpr size_t snapshot_limit = 4 * 1024 * 1024;
struct pet_literal
{
	uint64_t uid, hold_reason;
	std::vector<item_literal> items;
};
struct player_literal
{
	uint32_t pid, wire_version;
	uint64_t revision;
	bool death;
	std::vector<item_literal> items;
	std::vector<pet_literal> pets;
};
inline bool death_schema(uint32_t version)
{
	return version == 8 || version == 10 || (version >= 13 && version <= 16) || version == 18 ||
	       version == 19;
}
inline bool evidence_schema(uint32_t version)
{
	return version == 10 || version == 14 || version == 16 || version == 19;
}
inline void death_evidence(reader &in, snapshot_budget &budget)
{
	const std::array<std::vector<std::string>, 5> required{
		std::vector<std::string>{ "id", "pid", "obj_uid", "vnum", "container_id" },
		{ "id", "item_id", "location", "modifier" },
		{ "id", "item_id", "keyword", "description" },
		{ "item_uid", "root_item_uid", "parent_item_uid", "item_revision", "vnum", "state",
		  "owner_type", "owner_id", "owner_context_id" },
		{ "owner_type", "owner_id", "owner_context_id", "revision" }
	};
	size_t total_rows = 0;
	for (const auto &names : required)
	{
		const auto columns = in.number(4);
		need(columns && columns <= 64);
		std::set<std::string> observed;
		for (size_t i = 0; i < columns; ++i)
		{
			const auto value = text(in, 64);
			need(!value.empty());
			for (auto c : value)
				need((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') ||
				     (c >= '0' && c <= '9') || c == '_');
			need(observed.emplace(reinterpret_cast<const char *>(value.data()),
					      value.size())
				     .second);
		}
		for (const auto &name : names)
			need(observed.contains(name));
		const auto rows = budget.count(in);
		total_rows += rows;
		need(rows * columns <= in.value.size() - in.offset);
		for (size_t i = 0; i < rows * columns; ++i)
		{
			const auto present = in.number(1);
			need(present <= 1);
			if (present)
				(void)text(in, snapshot_limit);
		}
	}
	need(total_rows);
}
// Death corpse and conflict observations validate the frame; they grant no
// current player or pet ownership and are never exported as active inventory.
inline void death_literal(reader &in, snapshot_budget &budget, uint32_t pid)
{
	const auto operation = in.fixed<16>();
	const auto room = signed32(in);
	const auto revision = in.number(8);
	need(nonzero(operation) && room > 0 && revision);
	std::array<int32_t, 4> money{};
	bool has_money = false;
	for (auto &value : money)
	{
		value = signed32(in);
		need(value >= 0);
		has_money = has_money || value;
	}
	const auto pile = in.number(8);
	need(has_money == (pile != 0));
	const auto corpse = decode_items(in, budget);
	need(!corpse.empty() && corpse.front().vnum == 2 && corpse.front().type == 24 &&
	     corpse.front().values[3] == static_cast<int32_t>(pid) &&
	     corpse.front().values[6] > 0 && (corpse.front().values[1] & 1));
	std::set<uint64_t> captured;
	for (size_t i = 0; i < corpse.size(); ++i)
	{
		const auto &row = corpse[i];
		need(row.uid && captured.insert(row.uid).second && (!i || row.parent != -1));
		if (row.uid == pile)
			need(row.vnum == 3 && row.type == 20 && row.parent == 0 &&
			     std::equal(money.begin(), money.end(), row.values.begin()));
	}
	need(!has_money || (pile != corpse.front().uid && captured.contains(pile)));
	captured.erase(corpse.front().uid);
	std::set<uint64_t> observed;
	const auto count = budget.count(in);
	for (size_t i = 0; i < count; ++i)
	{
		const auto uid = in.number(8), root = in.number(8);
		(void)in.take(8); // Observed parent is evidence, not inventory topology.
		const auto item_revision = in.number(8);
		const auto vnum = signed32(in);
		const auto state = in.number(1), kind = in.number(1), owner_id = in.number(8),
			   context = in.number(8), owner_revision = in.number(8);
		need(uid && observed.insert(uid).second && vnum > 0 && state <= 3 && kind <= 12);
		if (kind == 12)
			need(owner_id && owner_id != UINT64_MAX && !context);
		if (!state)
			need(item_revision == UINT64_MAX && !kind && !owner_id && !context &&
			     !owner_revision);
		else
			need(item_revision && item_revision != UINT64_MAX && root && kind);
		captured.erase(uid);
	}
	need(captured.empty());
	std::set<identity> unresolved;
	const auto pending = budget.count(in);
	for (size_t i = 0; i < pending; ++i)
	{
		const auto id = in.fixed<16>();
		need(nonzero(id) && id != operation && unresolved.insert(id).second);
	}
}
// Parses every wire field and the full trailing evidence. Spans borrow encoded.
// Historical versions normalize only for semantic checks, not framing.
inline player_literal decode_player(std::span<const uint8_t> encoded, uint32_t expected_pid)
{
	need(encoded.size() >= 68 && encoded.size() <= snapshot_limit + 68 && expected_pid &&
	     expected_pid <= INT32_MAX &&
	     same(encoded.first(8), { reinterpret_cast<const uint8_t *>("DURPLYR\0"), 8 }));
	reader header{ encoded.subspan(8) };
	need(header.number(4) == 1 && header.number(4) == encoded.size() - 68 &&
	     header.number(4) == expected_pid);
	const auto revision = header.number(8), components = header.number(8);
	need(revision && components == 16383);
	const auto digest = header.fixed<32>();
	const auto payload = encoded.subspan(68);
	need(same(digest, hash(payload)));
	reader in{ payload };
	snapshot_budget budget;
	const auto raw_version = in.number(4);
	const bool ward = raw_version >= 20 && raw_version <= 32;
	const auto wire = static_cast<uint32_t>(ward ? raw_version - 13 : raw_version);
	need(raw_version <= 32 && wire && wire <= 19 && wire != 9);
	const auto schema = wire <= 8 ? (wire % 2 ? 7 : 8) : wire;
	need(in.number(4) == expected_pid && in.number(8) == revision &&
	     in.number(8) == components);
	const auto save_intent = signed32(in);
	(void)in.take(4); // Room observation is a signed native value.
	const auto bound = in.number(8);
	need(bound && bound <= snapshot_limit);
	player_literal result{ expected_pid, static_cast<uint32_t>(raw_version),
			       revision,     death_schema(schema),
			       {},	     {} };
	const auto integers = budget.count(in);
	for (size_t i = 0; i < integers; ++i)
	{
		need(in.number(2) <= 62);
		(void)in.take(16);
		need(in.number(1) <= 1);
	}
	const auto strings = budget.count(in);
	for (size_t i = 0; i < strings; ++i)
	{
		need(in.number(1) <= 6);
		(void)text(in, 4096);
	}
	(void)in.take((5 + 14) * 4); // Conditions and quest values.
	for (size_t i = 0; i < 5; ++i)
		(void)in.take(budget.count(in) * 20); // Indexed value/auxiliary rows.
	(void)in.take(budget.count(in) * 4); // Granted commands.
	(void)in.take(budget.count(in) * 6); // Skills.
	const auto affects = budget.count(in);
	for (size_t i = 0; i < affects; ++i)
	{
		(void)in.take(17 + 5 * 8);
		if (ward)
			(void)in.take(8 + 4 + 8 + 8 + 4 + 3);
		(void)text(in, 4096);
		(void)text(in, 4096);
	}
	result.items = decode_items(in, budget);
	const auto pets = budget.count(in);
	for (size_t i = 0; i < pets; ++i)
	{
		pet_literal pet{ wire >= 7 ? in.number(8) : 0, 0, {} };
		(void)in.take(10 * 4);
		pet.items = decode_items(in, budget);
		if (wire >= 3)
		{
			(void)text(in, 32768);
			pet.hold_reason = in.number(4);
		}
		result.pets.push_back(std::move(pet));
	}
	(void)in.take(budget.count(in) * 24); // Shape observations.
	(void)in.take(budget.count(in) * 8); // Trophies.
	need(in.number(1) <= 1);
	if (wire >= 5)
		(void)text(in, 512);
	const bool craft = wire >= 17;
	const bool quest = craft || wire == 11 || wire == 12 || wire == 15 || wire == 16;
	const bool spell = craft || (wire >= 12 && wire <= 16);
	if (quest)
	{
		const auto count = budget.count(in);
		need(count <= 64 && (count || wire == 12 || craft));
		std::set<std::pair<identity, uint64_t>> seen;
		for (size_t i = 0; i < count; ++i)
		{
			const auto id = in.fixed<16>();
			const auto index = in.number(4), amount = in.number(4);
			need(nonzero(id) && index < 64 && amount && seen.emplace(id, index).second);
		}
	}
	if (spell)
	{
		const auto count = budget.count(in);
		need(count <= 4096 && (count || craft || wire == 15 || wire == 16));
		std::set<identity> seen;
		for (size_t i = 0; i < count; ++i)
		{
			const auto id = in.fixed<16>();
			const auto effect = in.number(4);
			need(nonzero(id) && effect && effect <= 6 && seen.insert(id).second);
		}
	}
	if (craft)
	{
		const auto count = budget.count(in);
		need(count && count <= 64);
		std::set<identity> seen;
		for (size_t i = 0; i < count; ++i)
		{
			const auto id = in.fixed<16>();
			const auto discipline = in.number(4), experience = in.number(4);
			need(nonzero(id) && discipline >= 1 && discipline <= 6 &&
			     (discipline <= 2 || !experience) && experience <= INT32_MAX &&
			     seen.insert(id).second);
		}
	}
	if (result.death)
	{
		need(save_intent == 4 && result.items.empty());
		for (const auto &pet : result.pets)
			need(pet.hold_reason);
		death_literal(in, budget, expected_pid);
		if (evidence_schema(schema))
			death_evidence(in, budget);
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
	bool players_present = false, custody_present = false;
	size_t players = 0, pets = 0, legacy_pets = 0, death_frames = 0, player_items = 0,
	       pet_items = 0, coin_literals = 0, wallet_roots_excluded = 0, compared_items = 0,
	       custody_player_items = 0, other_custody_items = 0, finding_count = 0,
	       custody_equipment_fields_absent = 0, coin_payloads_compared = 0,
	       coin_payloads_absent = 0;
	std::map<std::string, size_t> finding_counts;
	std::vector<finding> findings;
	bool valid() const { return !finding_count; }
	bool verified() const { return players_present && custody_present && valid(); }
};
inline result audit(const std::filesystem::path &root, audit_budget &budget,
		    size_t detail_limit = 100)
{
	need(detail_limit <= 100);
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	result output;
	bytes custody_bytes;
	struct stat info = {};
	const auto domains = root / "domains", players = root / "players";
	if (lstat((domains / "item_ownership").c_str(), &info) == 0)
	{
		need(lock.locked());
		custody_bytes = file_bytes(domains, "item_ownership", catalog_limit);
		output.custody_present = true;
	}
	else
		need(errno == ENOENT);
	restore_native_custody::catalog custody{};
	if (output.custody_present)
		custody = decode_catalog(custody_bytes);
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
		owned.emplace(row.uid, &row);
		if (row.state == 2)
			continue;
		if (row.location.type == 1 || row.location.type == 11)
			++output.custody_player_items;
		else
			++output.other_custody_items;
	}
	std::set<uint64_t> seen, pet_uids;
	const auto compare = [&](const std::vector<item_literal> &items, owner location, bool pet)
	{
		for (size_t i = 0; i < items.size(); ++i)
		{
			audit_checkpoint();
			const auto &literal = items[i];
			if (pet)
				++output.pet_items;
			else
				++output.player_items;
			// Wallet roots are governed by the wallet path, not item transfers.
			if (literal.type == 20 && literal.parent == -1)
			{
				++output.wallet_roots_excluded;
				continue;
			}
			if (!literal.uid || literal.vnum <= 0)
			{
				issue("player_item_identity_invalid", literal.uid);
				continue;
			}
			if (!seen.insert(literal.uid).second)
				issue("player_uid_duplicate_literal", literal.uid);
			if (literal.type == 20)
			{
				++output.coin_literals;
				if (std::any_of(literal.values.begin(), literal.values.begin() + 4,
						[](auto value) { return value < 0; }))
					issue("player_negative_coin_value", literal.uid);
			}
			const auto found = owned.find(literal.uid);
			if (found == owned.end())
			{
				issue("player_uid_unadmitted", literal.uid);
				continue;
			}
			++output.compared_items;
			const auto &row = *found->second;
			if (row.state != 1)
				issue("player_uid_not_active", row.uid);
			if (row.location != location)
				issue("player_owner_mismatch", row.uid);
			size_t root_index = i;
			while (items[root_index].parent >= 0)
				root_index = items[root_index].parent;
			const auto parent = literal.parent < 0 ? 0 : items[literal.parent].uid;
			if (row.root != items[root_index].uid || row.parent != parent)
				issue("player_item_topology_mismatch", row.uid);
			if (row.vnum != literal.vnum)
				issue("player_item_vnum_mismatch", row.uid);
			if (custody.version < 5)
				++output.custody_equipment_fields_absent;
			else if (literal.equipment < 0 || row.equipment != literal.equipment)
				issue("player_item_equipment_mismatch", row.uid);
			if (!row.coin_payload.empty())
			{
				++output.coin_payloads_compared;
				if (!same(row.coin_payload.subspan(8), literal.encoded.subspan(4)))
					issue("player_coin_literal_mismatch", row.uid);
			}
			else if (literal.type == 20)
				++output.coin_payloads_absent;
		}
	};
	const int fd = open(players.c_str(), O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	struct closer
	{
		int fd;
		~closer()
		{
			if (fd >= 0)
				close(fd);
		}
	} close_players{ fd };
	if (fd >= 0)
	{
		need(lock.locked() && fstat(fd, &info) == 0 && info.st_uid == geteuid() &&
		     !(info.st_mode & 0077));
		output.players_present = true;
		std::vector<std::pair<uint32_t, std::string>> names;
		for (const auto &entry : std::filesystem::directory_iterator(players))
		{
			audit_directory_entry();
			const auto leaf = entry.path().filename().string();
			if (!leaf.ends_with(".snapshot"))
				continue;
			const auto body = leaf.substr(0, leaf.size() - 9);
			uint32_t pid = 0;
			const auto parsed =
				std::from_chars(body.data(), body.data() + body.size(), pid);
			need(parsed.ec == std::errc{} && parsed.ptr == body.data() + body.size() &&
			     pid && pid <= INT32_MAX && std::to_string(pid) == body);
			names.emplace_back(pid, leaf);
		}
		std::sort(names.begin(), names.end());
		for (const auto &[pid, leaf] : names)
		{
			const auto encoded = file_bytes(players, leaf, snapshot_limit + 68);
			const auto player = decode_player(encoded, pid);
			++output.players;
			output.death_frames += player.death;
			compare(player.items, { 1, pid, 0 }, false);
			for (const auto &pet : player.pets)
			{
				++output.pets;
				if (!pet.uid)
					++output.legacy_pets;
				else if (!pet_uids.insert(pet.uid).second)
					issue("player_pet_uid_duplicate", pet.uid);
				compare(pet.items,
					pet.uid ? owner{ 11, pet.uid, pid } : owner{ 1, pid, 0 },
					true);
			}
		}
		struct stat named = {};
		need(lstat(players.c_str(), &named) == 0 && named.st_dev == info.st_dev &&
		     named.st_ino == info.st_ino);
	}
	else
		need(errno == ENOENT);
	if (output.compared_items <
		    output.player_items + output.pet_items - output.wallet_roots_excluded &&
	    !output.custody_present)
		issue("player_custody_catalog_missing");
	if (output.custody_player_items && !output.players_present)
		issue("custody_player_directory_missing");
	for (const auto &row : custody.items)
		if (row.state != 2 && (row.location.type == 1 || row.location.type == 11) &&
		    !seen.contains(row.uid))
			issue("player_uid_missing_literal", row.uid);
	lock.no_pending_player_domains();
	lock.finish();
	budget.checkpoint();
	return output;
}
} // namespace restore_native_player
#endif
