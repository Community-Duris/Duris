// Independent durable custody decoding. No native repository or recovery calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_CUSTODY_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_CUSTODY_H

#include "qualify_flatfile_native_domains.h"
#include <bit>
#include <compare>
#include <map>
#include <optional>
#include <tuple>

namespace restore_native_custody
{
using namespace restore_native_domains;
constexpr size_t catalog_limit = 128 * 1024 * 1024, item_payload_limit = 128 * 1024;
struct owner
{
	uint8_t type;
	uint64_t id, context;
	auto operator<=>(const owner &) const = default;
};
inline owner read_owner(reader &in)
{
	owner value{ static_cast<uint8_t>(in.number(1)), in.number(8), in.number(8) };
	need(value.type >= 1 && value.type <= 12);
	if (value.type == 7 || value.type == 8)
		need(!value.id && !value.context);
	else if (value.type == 10)
		need(value.id && !value.context);
	else if (value.type == 12)
		need(value.id && value.id != UINT64_MAX && !value.context);
	else if (value.type == 11)
		need(value.id && value.context && value.context <= INT32_MAX);
	else
		need(value.id);
	return value;
}
inline int32_t signed32(reader &in)
{
	return std::bit_cast<int32_t>(static_cast<uint32_t>(in.number(4)));
}
inline std::span<const uint8_t> text(reader &in, size_t maximum)
{
	const auto size = in.number(4);
	need(size <= maximum);
	return in.take(size);
}
struct coin_literal
{
	uint64_t uid;
	int32_t vnum;
	std::array<int32_t, 4> balance;
};
struct item_literal
{
	int32_t parent;
	int16_t equipment;
	uint64_t uid;
	int32_t vnum;
	uint8_t type;
	std::array<int32_t, 8> values;
	std::span<const uint8_t> encoded;
};
// Native snapshot-list framing; every literal borrows the immutable input.
inline std::vector<item_literal> decode_items(std::span<const uint8_t> encoded)
{
	need(!encoded.empty() && encoded.size() <= 4 * 1024 * 1024);
	reader in{ encoded };
	const auto count = in.number(4);
	need(count <= 4096);
	size_t remaining = 8192 - count;
	const auto rows = [&]()
	{
		const auto count = in.number(4);
		need(count <= remaining);
		remaining -= count;
		return count;
	};
	std::vector<item_literal> result;
	std::vector<size_t> depths;
	for (size_t i = 0; i < count; ++i)
	{
		const auto begin = in.offset;
		item_literal row{};
		row.parent = signed32(in);
		row.equipment = std::bit_cast<int16_t>(static_cast<uint16_t>(in.number(2)));
		row.uid = in.number(8);
		(void)in.take(8); // Generated key.
		row.vnum = signed32(in);
		row.type = in.number(1);
		(void)in.take(1); // The native codec does not constrain string mask bits.
		for (size_t j = 0; j < 4; ++j)
			(void)text(in, 4096);
		for (auto &value : row.values)
			value = signed32(in);
		(void)in.take(6 * 8 + 5 * 4 + 4 + 1 + 4 + 2 + 2 + 5 * 8 + 8 * 2);
		(void)in.take(rows() * 12);
		const auto descriptions = rows();
		for (size_t j = 0; j < descriptions; ++j)
		{
			(void)text(in, 4096);
			(void)text(in, 4096);
			need(in.number(1) <= 1);
			(void)in.take(rows() * 4);
		}
		need(row.parent >= -1 && row.parent < static_cast<int32_t>(i));
		const auto depth = row.parent < 0 ? 1 : depths[row.parent] + 1;
		need(depth <= 32);
		depths.push_back(depth);
		row.encoded = encoded.subspan(begin, in.offset - begin);
		result.push_back(row);
	}
	in.done();
	return result;
}
inline coin_literal decode_coin(std::span<const uint8_t> encoded)
{
	need(encoded.size() <= item_payload_limit);
	const auto items = decode_items(encoded);
	need(items.size() == 1 && items[0].type == 20);
	coin_literal result{ items[0].uid, items[0].vnum, {} };
	std::copy_n(items[0].values.begin(), 4, result.balance.begin());
	return result;
}
// Validate the entire continuation, retaining only the admissible XP bit slots.
// Character aliases and quest strings are never exported by the operator audit.
inline uint64_t quest_slots(std::span<const uint8_t> encoded)
{
	need(encoded.size() >= 40 && encoded.size() <= 8192);
	reader in{ encoded };
	const auto version = in.number(4), pid = in.number(4), quester = in.number(4),
		   completion = in.number(4), mobile = in.number(4), room = in.number(4),
		   completed = in.number(8), root_count = in.number(4);
	need(version >= 1 && version <= 6 && pid && quester <= INT32_MAX &&
	     completion <= INT32_MAX && mobile && mobile <= INT32_MAX && room &&
	     room <= INT32_MAX && completed && completed <= INT64_MAX &&
	     (version == 6 ? !root_count : root_count != 0) && root_count <= 14);
	std::set<uint64_t> roots;
	for (size_t i = 0; i < root_count; ++i)
	{
		const auto root = in.number(8);
		need(root && roots.insert(root).second);
	}
	struct reward
	{
		uint64_t type, number, frozen;
	};
	const auto reward_count = in.number(4);
	need(reward_count <= 64);
	std::vector<reward> rewards;
	uint64_t solo_slots = 0;
	for (size_t i = 0; i < reward_count; ++i)
	{
		const auto type = in.number(4), number = in.number(4),
			   flags = version >= 3 ? in.number(4) : 0,
			   frozen = version >= 4 ? in.number(4) : 0;
		need((type == 1 || type == 3 || type == 4 || type == 5) && number <= INT32_MAX &&
		     (type == 4 || number) && flags <= 1 && (type == 4 || !flags) &&
		     frozen <= INT32_MAX &&
		     (version >= 4 && type == 5 ? frozen && frozen <= number : !frozen));
		if (type == 5)
			solo_slots |= UINT64_C(1) << i;
		rewards.push_back({ type, number, frozen });
	}
	if (version == 1)
	{
		in.done();
		return 0;
	}
	const auto zone = in.number(4);
	const auto level = signed32(in);
	(void)in.take(4); // Racewar is a signed native observation.
	const auto party = in.number(4);
	const auto strongest = signed32(in);
	const auto count = in.number(4);
	need(zone && zone <= INT32_MAX && level >= 0 && strongest >= 0 && count && count <= 64 &&
	     party == count);
	std::set<uint64_t> recipients;
	uint64_t first = 0;
	for (size_t i = 0; i < count; ++i)
	{
		const auto recipient = in.number(4);
		need(recipient && recipients.insert(recipient).second);
		if (!i)
			first = recipient;
	}
	need(recipients.contains(pid));
	const auto name = text(in, 64);
	need(!name.empty() && std::find(name.begin(), name.end(), 0) == name.end());
	const auto definition = text(in, 4096);
	need(!definition.empty());
	for (auto c : definition)
		need(c >= 0x21 && c <= 0x7e);
	if (version < 5)
	{
		in.done();
		return version == 4 && count == 1 ? solo_slots : 0;
	}
	need(first == pid);
	const auto awards = in.number(4);
	need(awards <= 64);
	std::set<std::pair<uint64_t, uint64_t>> observed;
	for (size_t i = 0; i < awards; ++i)
	{
		const auto recipient = in.number(4), index = in.number(4), amount = in.number(4);
		need(recipients.contains(recipient) && index < rewards.size() && amount &&
		     rewards[index].type == 5 && amount <= rewards[index].number &&
		     (recipient != pid || amount == rewards[index].frozen) &&
		     observed.emplace(recipient, index).second);
	}
	for (size_t i = 0; i < rewards.size(); ++i)
		if (rewards[i].type == 5)
			for (auto recipient : recipients)
				need(observed.contains({ recipient, i }));
	if (version == 6)
	{
		need(same(in.take(4), { reinterpret_cast<const uint8_t *>("QRF6"), 4 }));
		const auto action = in.fixed<16>();
		const auto source = in.take(48);
		const auto lifetime = in.number(8);
		const auto acceptance = in.fixed<16>();
		const auto result = in.take(48);
		need(nonzero(action) && nonzero(acceptance) && action != acceptance &&
		     number(source, 0, 2) == 19 && number(source, 2, 2) == 1 &&
		     same(source.subspan(4, 16), action) && nonzero(source.subspan(20, 16)) &&
		     number(source, 36, 8) && number(source, 44, 4) == completion && lifetime &&
		     lifetime != UINT64_MAX && number(result, 0, 8) && number(result, 8, 2) &&
		     number(result, 8, 2) <= 3000 && !nonzero(result.subspan(10, 6)) &&
		     !number(result, 40, 8));
	}
	in.done();
	return awards == 64 ? UINT64_MAX : (UINT64_C(1) << awards) - 1;
}
struct item
{
	uint64_t uid, root, parent, revision;
	owner location;
	int32_t vnum;
	uint8_t state;
	uint16_t equipment;
	// Borrowed from the caller-owned encoded catalog for later literal comparison.
	std::span<const uint8_t> coin_payload;
};
struct operation
{
	identity id;
	digest command_digest;
	uint64_t result_code, root, count, from_revision, to_revision, item_revision,
		corpse_revision, xp_mask, creation_source, creation_recipient;
	int32_t creation_vnum;
	bool collector_changed, coin, acknowledged, legacy_economic;
	std::span<const uint8_t> coin_result, quest_continuation;
};
struct catalog
{
	uint64_t revision;
	uint32_t version;
	std::map<owner, uint64_t> owners;
	std::vector<item> items;
	std::vector<operation> operations;
};
// Every span in the result borrows encoded. Keep that buffer alive and immutable.
inline catalog decode_catalog(std::span<const uint8_t> encoded)
{
	const auto file = unwrap(encoded, "DUROWN\0", catalog_limit, 8);
	reader in{ file.body };
	catalog result{ file.revision, file.version, {}, {}, {} };
	const auto owners = in.number(4), items = in.number(4), operations = in.number(4);
	need(owners <= 262144 && items <= 262144 && operations <= 1048576);
	std::optional<owner> previous_owner;
	for (size_t i = 0; i < owners; ++i)
	{
		const auto value = read_owner(in);
		const auto revision = in.number(8);
		need(!previous_owner || *previous_owner < value);
		need(result.owners.emplace(value, revision).second);
		previous_owner = value;
	}
	uint64_t previous_uid = 0;
	for (size_t i = 0; i < items; ++i)
	{
		const auto uid = in.number(8), root = in.number(8), parent = in.number(8);
		const auto location = read_owner(in);
		const auto revision = in.number(8);
		const auto vnum = signed32(in);
		const auto state = in.number(1);
		need(uid > previous_uid && root && vnum > 0 && state >= 1 && state <= 3 &&
		     result.owners.contains(location));
		std::span<const uint8_t> coin_payload;
		if (file.version >= 3)
		{
			const auto length = in.number(4);
			need(length <= item_payload_limit);
			coin_payload = in.take(length);
			if (length)
			{
				const auto literal = decode_coin(coin_payload);
				need(literal.uid == uid && literal.vnum == vnum);
			}
		}
		const auto equipment = file.version >= 5 ? in.number(2) : 0;
		need(equipment <= 43 &&
		     (!equipment || ((location.type == 1 || location.type == 12) && !parent &&
				     state == 1 && (location.type != 12 || root == uid))));
		result.items.push_back({ uid, root, parent, revision, location, vnum,
					 static_cast<uint8_t>(state),
					 static_cast<uint16_t>(equipment), coin_payload });
		previous_uid = uid;
	}
	std::set<identity> ids;
	for (size_t i = 0; i < operations; ++i)
	{
		operation value{};
		value.id = in.fixed<16>();
		value.command_digest = in.fixed<32>();
		value.result_code = in.number(4);
		value.root = in.number(8);
		value.count = in.number(2);
		value.from_revision = in.number(8);
		value.to_revision = in.number(8);
		value.item_revision = in.number(8);
		value.corpse_revision = file.version >= 2 ? in.number(8) : 0;
		const auto collector = file.version >= 4 ? in.number(1) : 0;
		const auto coin = file.version >= 3 ? in.number(1) : 0;
		need(collector <= 1 && coin <= 1);
		value.collector_changed = collector;
		value.coin = coin;
		if (coin)
			value.coin_result = in.take(256);
		if (file.version >= 6)
		{
			const auto length = in.number(4);
			need(length <= 8192);
			value.quest_continuation = in.take(length);
			const auto acknowledged = in.number(1);
			need(acknowledged <= 1);
			value.acknowledged = acknowledged;
		}
		const auto mask = file.version >= 7 ? in.number(8) : 0;
		value.xp_mask = mask;
		for (int j = 0; j < std::popcount(mask); ++j)
			need(in.number(8));
		if (file.version >= 8)
		{
			value.creation_source = in.number(8);
			value.creation_recipient = in.number(4);
			value.creation_vnum = signed32(in);
			const auto legacy = in.number(1);
			need(legacy <= 1);
			value.legacy_economic = legacy;
		}
		else
			value.legacy_economic = !value.quest_continuation.empty();
		need(nonzero(value.id) && ids.insert(value.id).second &&
		     (value.coin || (value.root && value.count && value.count <= 3000)) &&
		     ((!value.creation_source) == (!value.creation_recipient)) &&
		     (value.creation_source ?
			      !value.coin && !value.result_code && value.creation_vnum > 0 &&
				      value.item_revision == 1 :
			      !value.creation_vnum));
		if (value.quest_continuation.empty())
			need(!value.acknowledged && !mask && !value.legacy_economic);
		else
			need(!value.coin && !value.result_code &&
			     !(mask & ~quest_slots(value.quest_continuation)));
		result.operations.push_back(value);
	}
	in.done();
	return result;
}
struct summary
{
	bool present = false;
	uint64_t revision = 0;
	uint32_t version = 0;
	size_t owners = 0, items = 0, operations = 0, active = 0, destroyed = 0, quarantined = 0,
	       inline_coin_payloads = 0, coin_operations = 0, quest_operations = 0;
};
inline summary audit_catalog(const std::filesystem::path &root, audit_budget &budget)
{
	scoped_audit_budget scope(budget);
	authority_read_lock lock(root);
	lock.no_pending_player_domains();
	summary result;
	const auto domains = root / "domains";
	struct stat info = {};
	if (lstat((domains / "item_ownership").c_str(), &info) != 0)
		need(errno == ENOENT);
	else
	{
		need(lock.locked());
		const auto encoded = file_bytes(domains, "item_ownership", catalog_limit);
		const auto decoded = decode_catalog(encoded);
		result.present = true;
		result.version = decoded.version;
		result.revision = decoded.revision;
		result.owners = decoded.owners.size();
		result.items = decoded.items.size();
		result.operations = decoded.operations.size();
		for (const auto &row : decoded.items)
		{
			result.active += row.state == 1;
			result.destroyed += row.state == 2;
			result.quarantined += row.state == 3;
			result.inline_coin_payloads += !row.coin_payload.empty();
		}
		for (const auto &row : decoded.operations)
		{
			result.coin_operations += row.coin;
			result.quest_operations += !row.quest_continuation.empty();
		}
	}
	lock.no_pending_player_domains();
	lock.finish();
	budget.checkpoint();
	return result;
}
} // namespace restore_native_custody
#endif
