// Independent physical retained-operation scan. No mutation/recovery codecs.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_RECORDS_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_RECORDS_H
#include "qualify_flatfile_economic_baseline.h"
#include "qualify_flatfile_economic_lifecycle.h"
#include <bit>
#include <tuple>

namespace restore_economic_records
{
struct initialization_provenance
{
	size_t legacy_unknown_epochs = 0, never_initialized_epochs = 0, initialized_epochs = 0;
	size_t lifecycle_receipts = 0;
	size_t unknown_initialized_origins = 0, baseline_participant_epochs = 0,
	       lifecycle_owner_epochs = 0;
	bool complete() const { return legacy_unknown_epochs == 0; }
	bool lifecycle_complete() const { return complete() && unknown_initialized_origins == 0; }
};
using namespace restore_economic_authority;
constexpr size_t index_limit = 80 + 4096 * 64, segment_limit = 8 * 1024 * 1024;
constexpr size_t command_limit = 512 * 1024, plan_limit = 4 * 1024 * 1024;
constexpr size_t record_limit = 48 + 26 + command_limit + plan_limit + 4096;
constexpr uint64_t bucket_limit = uint64_t{ 256 } << 20;
constexpr size_t segment_count_limit = bucket_limit / (segment_limit - record_limit - 80) + 1;
// Independent schema-2 envelope grammar. Native codecs remain test oracles.
inline bool command_capability_valid(uint64_t type, uint64_t version, uint64_t publication)
{
	return type >= 1 && type <= 21 && version > 0 && version <= UINT16_MAX &&
	       publication <= 1 &&
	       (!publication || type == 3 || type == 5 || type == 17 || type == 18 || type == 21 ||
		(type == 15 && version >= 6 && version <= 8));
}
inline bool command_key_valid(uint64_t kind, uint64_t id)
{
	return kind >= 1 && kind <= 15 && id && (kind != 15 || id != UINT64_MAX);
}
inline std::span<const uint8_t> unwrap(std::span<const uint8_t> value, const char *magic)
{
	reader in{ value };
	need(same(in.take(8), { reinterpret_cast<const uint8_t *>(magic), 8 }) &&
	     in.number(4) == 1 && in.number(4) == value.size() - 48);
	auto checksum = in.fixed<32>();
	auto body = in.take(value.size() - 48);
	need(hash(body) == checksum);
	return body;
}
template <size_t N> inline digest tagged_hash(const char (&tag)[N], std::span<const uint8_t> value)
{
	bytes encoded(tag, tag + N); // The native wire contract includes the NUL delimiter.
	encoded.insert(encoded.end(), value.begin(), value.end());
	return hash(encoded);
}
// Independent interpretation of the version-1 reason contract. The native
// codec is a qualification oracle, never an implementation dependency.
inline bool source_allowed(uint64_t reason, uint64_t kind)
{
	switch (reason)
	{
	case 3:
	case 41:
	case 42:
		return kind == 16;
	case 5:
		return kind == 1;
	case 44:
		return kind == 19;
	case 6:
		return kind == 2;
	case 7:
		return kind == 3 || kind == 7;
	case 8:
		return kind == 3;
	case 9:
		return kind == 4;
	case 10:
		return kind == 5;
	case 11:
	case 12:
	case 13:
	case 14:
	case 15:
	case 16:
		return kind == 17;
	case 17:
		return kind == 8;
	case 18:
	case 19:
	case 45:
	case 46:
		return kind == 6;
	case 21:
	case 22:
		return kind == 12 || kind == 18;
	case 24:
		return kind == 17 || kind == 18;
	case 26:
	case 27:
	case 28:
	case 29:
	case 30:
		return kind == 13;
	case 31:
		return kind == 13 || kind == 17;
	case 36:
		return kind == 14;
	case 38:
		return kind == 10;
	case 43:
		return kind == 18;
	default:
		return true;
	}
}
inline void intent_semantics(std::span<const uint8_t> intent)
{
	const auto reason = number(intent, 24, 2);
	need(reason >= 1 && reason <= 46 && number(intent, 16, 4) == 1 &&
	     number(intent, 20, 4) == 1 && intent[26] == (reason >= 38 && reason <= 42 ? 2 : 1) &&
	     !same(intent.subspan(64, 16), intent.subspan(80, 16)));
	const bool original = reason == 19 || reason == 20 || reason == 40 || reason == 45 ||
			      reason == 46;
	const bool source = (reason >= 5 && reason <= 22) || (reason >= 26 && reason <= 31) ||
			    (reason >= 33 && reason <= 36) || reason >= 38;
	need((!original || nonzero(intent.subspan(80, 16))) && (!source || intent[27]));
	if (intent[27])
		need(number(intent, 114, 2) == 1 && nonzero(intent.subspan(116, 16)) &&
		     nonzero(intent.subspan(132, 16)) &&
		     source_allowed(reason, number(intent, 112, 2)));
}
using wide = __int128_t;
using coins = std::array<int64_t, 4>;
inline coins coin_vector(std::span<const uint8_t> row, size_t offset)
{
	coins result;
	for (size_t i = 0; i < result.size(); ++i)
		result[i] = std::bit_cast<int64_t>(number(row, offset + i * 8, 8));
	return result;
}
inline int64_t coin_value(const coins &value)
{
	constexpr std::array<int, 4> units = { 1, 10, 100, 1000 };
	wide total = 0;
	for (size_t i = 0; i < value.size(); ++i)
		total += wide(value[i]) * units[i];
	need(total >= INT64_MIN && total <= INT64_MAX);
	return static_cast<int64_t>(total);
}
inline bool ordinary(uint64_t kind)
{
	return kind <= 6 || kind == 11;
}
inline void position_semantics(uint64_t uid, std::span<const uint8_t> value)
{
	need(value.size() == 56 && !nonzero(value.subspan(2, 6)) && !nonzero(value.subspan(50, 6)));
	const auto type = value[0], state = value[1];
	if (!state)
	{
		need(!nonzero(value));
		return;
	}
	const auto owner = number(value, 8, 8), context = number(value, 16, 8),
		   root = number(value, 24, 8), parent = number(value, 32, 8),
		   revision = number(value, 40, 8), slot = number(value, 48, 2);
	need(type >= 1 && type <= 12 && state <= 3 && root && parent != uid);
	if (type == 7 || type == 8)
		need(!owner && !context);
	else
		need(owner && (type != 10 || !context) &&
		     (type != 11 || (context && context <= INT32_MAX)) &&
		     (type != 12 || (owner < UINT64_MAX && !context)));
	need(type != 12 || slot <= 43);
	need(!slot ||
	     ((type == 1 || type == 12) && !parent && state == 1 && (type != 12 || root == uid)));
	need(state == 2 ? type == 8 && revision : type != 8 && (parent || root == uid));
}
using item = std::pair<uint64_t, std::span<const uint8_t>>;
inline auto item_at(const std::vector<item> &items, uint64_t uid)
{
	return std::lower_bound(items.begin(), items.end(), uid,
				[](const auto &row, uint64_t value) { return row.first < value; });
}
inline std::vector<item> snapshots(reader &in, size_t count)
{
	std::vector<item> result;
	for (size_t i = 0; i < count; ++i)
	{
		auto row = in.take(64);
		const auto uid = number(row, 0, 8);
		need(uid && (result.empty() || result.back().first < uid));
		position_semantics(uid, row.subspan(8));
		result.emplace_back(uid, row.subspan(8));
	}
	std::vector<uint8_t> colors(count);
	std::vector<size_t> path;
	for (size_t start = 0; start < count; ++start)
	{
		if (colors[start] == 2 || result[start].second[1] == 0 ||
		    result[start].second[1] == 2)
			continue;
		path.clear();
		size_t current = start;
		while (colors[current] != 2)
		{
			need(colors[current] != 1);
			colors[current] = 1;
			path.push_back(current);
			const auto child = result[current].second;
			const auto parent_uid = number(child, 32, 8);
			if (!parent_uid)
				break;
			const auto parent = item_at(result, parent_uid);
			need(parent != result.end() && parent->first == parent_uid);
			const auto ancestor = parent->second;
			need((ancestor[1] == 1 || ancestor[1] == 3) && ancestor[0] == child[0] &&
			     same(ancestor.subspan(8, 24), child.subspan(8, 24)));
			current = static_cast<size_t>(parent - result.begin());
		}
		for (auto index : path)
			colors[index] = 2;
	}
	return result;
}
inline void plan_semantics(std::span<const uint8_t> plan, const identity &lineage)
{
	constexpr std::array<unsigned, 47> masks = {
		0,   126, 126,	126,  126, 134, 134, 134, 134,	134, 134, 326, 326, 326,  326, 326,
		326, 326, 2050, 2178, 382, 326, 198, 0,	  326,	0,   274, 306, 306, 306,  306, 306,
		0,   136, 264,	138,  382, 126, 638, 126, 1150, 382, 638, 394, 326, 2304, 2050
	};
	const auto reason = number(plan, 96, 2);
	need(reason >= 1 && reason < masks.size());
	std::array<uint64_t, 6> counts;
	for (size_t i = 0; i < counts.size(); ++i)
		counts[i] = number(plan, 216 + 4 * i, 4);
	// Retained flatfile storage has no child reservations. Preserve that refusal.
	need(!counts[2]);
	reader in{ plan.subspan(256) };
	std::vector<std::span<const uint8_t>> accounts, postings;
	std::vector<uint64_t> kinds;
	std::tuple<uint64_t, uint64_t, uint64_t> previous;
	for (size_t i = 0; i < counts[0]; ++i)
	{
		auto row = in.take(120);
		auto key = restore_economic_baseline::account(row.first(40), lineage);
		const auto kind = std::get<0>(key);
		need((!i || previous < key) && (masks[reason] & (1U << kind)));
		previous = key;
		const auto before = coin_vector(row, 40), after = coin_vector(row, 72);
		const auto before_revision = number(row, 104, 8),
			   after_revision = number(row, 112, 8);
		if (ordinary(kind))
		{
			need(std::all_of(before.begin(), before.end(),
					 [](auto value) { return value >= 0; }) &&
			     std::all_of(after.begin(), after.end(),
					 [](auto value) { return value >= 0; }));
			(void)coin_value(before);
			(void)coin_value(after);
			need(after_revision >= before_revision &&
			     (before == after || after_revision > before_revision));
		}
		else
			need(!nonzero(row.subspan(40)));
		accounts.push_back(row);
		kinds.push_back(kind);
	}
	std::vector<std::array<wide, 4>> totals(accounts.size());
	std::vector<bool> referenced(accounts.size());
	wide balance = 0;
	for (size_t i = 0; i < counts[1]; ++i)
	{
		auto row = in.take(48);
		const auto index = number(row, 4, 2);
		need(number(row, 0, 4) == i && index < accounts.size() && !number(row, 6, 2) &&
		     nonzero(row.subspan(8, 32)));
		const auto delta = coin_vector(row, 8);
		const auto value = std::bit_cast<int64_t>(number(row, 40, 8));
		const auto kind = kinds[index];
		need(coin_value(delta) == value && ((kind != 7 && kind != 10) || value < 0) &&
		     (kind != 8 || (reason == 20 ? value < 0 : value > 0)) &&
		     (ordinary(kind) || value));
		balance += value;
		referenced[index] = true;
		for (size_t part = 0; part < delta.size(); ++part)
			totals[index][part] += delta[part];
		postings.push_back(row);
	}
	need(!balance);
	for (size_t i = 0; i < accounts.size(); ++i)
	{
		const auto row = accounts[i];
		const auto before = coin_vector(row, 40), after = coin_vector(row, 72);
		need(referenced[i] || (ordinary(kinds[i]) && before == after &&
				       number(row, 112, 8) > number(row, 104, 8)));
		if (ordinary(kinds[i]))
			for (size_t part = 0; part < before.size(); ++part)
				need(wide(before[part]) + totals[i][part] == after[part]);
	}
	if (reason == 18 || reason == 19 || reason == 45 || reason == 46)
	{
		const bool opening = reason == 18;
		need(plan[100] && number(plan, 148, 4) == (opening ? 0 : 1) && !counts[3] &&
		     !counts[4] && !counts[5]);
		const auto wallets = std::count(kinds.begin(), kinds.end(), 1),
			   sinks = std::count(kinds.begin(), kinds.end(), 8),
			   issuances = std::count(kinds.begin(), kinds.end(), 7),
			   stakes = std::count(kinds.begin(), kinds.end(), 11);
		need(stakes == 1 && (reason == 45 ? sinks == 1 && kinds.size() == 2 :
				     reason == 19 ? wallets == 1 && issuances <= 1 &&
							    kinds.size() == size_t(2 + issuances) :
						    wallets == 1 && kinds.size() == 2));
		const auto held =
			accounts[std::find(kinds.begin(), kinds.end(), 11) - kinds.begin()];
		need(number(held, 28, 8) && number(held, 28, 8) == number(plan, 140, 8));
		const auto stake = coin_vector(held, opening ? 72 : 40);
		need(!nonzero(held.subspan(opening ? 40 : 72, 32)));
		size_t denomination = stake.size();
		for (size_t i = 0; i < stake.size(); ++i)
			if (stake[i])
			{
				need(stake[i] > 0 && denomination == stake.size());
				denomination = i;
			}
		need(denomination != stake.size());
		size_t issuance_postings = 0;
		for (auto row : postings)
		{
			const auto delta = coin_vector(row, 8);
			for (size_t i = 0; i < delta.size(); ++i)
				need(i == denomination || !delta[i]);
			if (reason == 19 && issuances && kinds[number(row, 4, 2)] == 7)
			{
				++issuance_postings;
				need(delta[denomination] == -stake[denomination]);
			}
		}
		need(reason != 19 || !issuances || issuance_postings == 1);
	}
	auto current = snapshots(in, counts[3]);
	const auto after = snapshots(in, counts[4]);
	need(current.size() == after.size());
	for (size_t i = 0; i < current.size(); ++i)
		need(current[i].first == after[i].first);
	for (size_t i = 0; i < counts[5]; ++i)
	{
		auto row = in.take(128);
		const auto uid = number(row, 8, 8);
		const auto found = item_at(current, uid);
		need(number(row, 0, 4) == i && !number(row, 4, 2) && !nonzero(row.subspan(6, 2)) &&
		     found != current.end() && found->first == uid);
		const auto before = row.subspan(16, 56), next = row.subspan(72, 56);
		position_semantics(uid, next);
		need(same(found->second, before) && before[1] != 2 && next[1] != 0 &&
		     (before[1] != 0 || next[1] == 1 || next[1] == 3) &&
		     number(next, 40, 8) > number(before, 40, 8));
		current[found - current.cbegin()].second = next;
	}
	in.done();
	for (size_t i = 0; i < current.size(); ++i)
		need(same(current[i].second, after[i].second));
}
struct entry
{
	identity operation;
	digest checksum;
	uint64_t segment, offset, size;
};
struct root_page
{
	identity lineage = {}, cursor = {}, ceiling = {};
	digest authority_body = {};
	size_t rows = 0, verified = 0, bucket_rows = 0;
	bool exhausted = false;
	std::vector<identity> invalid_records;
};
class checker
{
	std::filesystem::path root, directory;
	identity lineage = {};
	std::set<identity> epochs;
	std::set<std::string> expected_files;
	std::vector<digest> claimed_events;
	restore_economic_baseline::checker baselines;
	restore_economic_lifecycle::checker lifecycles;

	void record(std::span<const uint8_t> encoded, const identity &operation)
	{
		need(encoded.size() >= 48 && encoded.size() <= record_limit);
		reader in{ unwrap(encoded, "DURECR2") };
		auto command_size = in.number(4), plan_size = in.number(4),
		     result_size = in.number(4);
		auto result_code = in.number(4);
		auto durable_revision = in.number(8);
		auto failure_stage = in.number(2);
		need(command_size <= command_limit && plan_size <= plan_limit &&
		     result_size <= 4096 && failure_stage <= 0x3fff &&
		     (result_code || failure_stage == 0) &&
		     (result_code ? plan_size == 0 : plan_size >= 256));
		auto command = in.take(command_size), plan = in.take(plan_size);
		(void)in.take(result_size);
		in.done();
		lifecycles.record(operation, command, plan, durable_revision, result_code,
				  result_size, failure_stage);
		reader cmd{ command };
		need(same(cmd.take(4), { reinterpret_cast<const uint8_t *>("CCM1"), 4 }) &&
		     cmd.number(4) == 2 && cmd.fixed<16>() == operation);
		auto type = cmd.number(2), payload_version = cmd.number(2), source = cmd.number(2);
		auto deadline = cmd.number(1), publication = cmd.number(1),
		     accepted = cmd.number(8);
		auto keys = cmd.number(4), revisions = cmd.number(4), payload_size = cmd.number(4);
		need(command_capability_valid(type, payload_version, publication) && source >= 1 &&
		     source <= 6 && deadline >= 1 && deadline <= 4 && accepted && keys > 0 &&
		     keys <= 3003 && revisions <= 3003 && payload_size <= 384 * 1024);
		using key = std::pair<uint64_t, uint64_t>;
		std::vector<key> identities;
		auto read_key = [&]
		{
			auto kind = cmd.number(1);
			need(!nonzero(cmd.take(7)));
			auto id = cmd.number(8);
			need(command_key_valid(kind, id));
			return key{ kind, id };
		};
		for (size_t i = 0; i < keys; ++i)
		{
			auto id = read_key();
			need(identities.empty() || identities.back() < id);
			identities.push_back(id);
		}
		key previous = {};
		for (size_t i = 0; i < revisions; ++i)
		{
			auto id = read_key();
			need((!i || previous < id) &&
			     std::binary_search(identities.begin(), identities.end(), id));
			(void)cmd.number(8);
			previous = id;
		}
		auto payload = cmd.take(payload_size);
		const auto prefix_size = cmd.offset;
		auto intent_size = cmd.number(4);
		need(intent_size >= 256 && intent_size <= 8192);
		auto intent = cmd.take(intent_size);
		cmd.done();
		need(same(intent.first(4), { reinterpret_cast<const uint8_t *>("EAI1"), 4 }) &&
		     number(intent, 4, 2) == 1 && number(intent, 6, 2) == 256 &&
		     number(intent, 8, 4) == intent.size() &&
		     number(intent, 104, 4) == intent.size() - 256 && intent[27] <= 1 &&
		     number(intent, 28, 2) == 1 && !nonzero(intent.subspan(30, 2)) &&
		     !nonzero(intent.subspan(108, 4)) && !nonzero(intent.subspan(224, 32)) &&
		     same(intent.subspan(32, 16), lineage) &&
		     same(intent.subspan(64, 16), operation));
		identity epoch;
		std::copy_n(intent.begin() + 48, 16, epoch.begin());
		need(epochs.contains(epoch));
		need(number(intent, 12, 4) && number(intent, 16, 4) && number(intent, 20, 4) &&
		     number(intent, 24, 2) >= 1 && number(intent, 24, 2) <= 46 &&
		     (intent[26] == 1 || intent[26] == 2) && number(intent, 96, 8));
		auto event = intent.subspan(112, 48);
		if (intent[27])
			need(number(event, 0, 2) >= 1 && number(event, 0, 2) <= 23 &&
			     number(event, 2, 2) == 1 && nonzero(event.subspan(4, 16)) &&
			     nonzero(event.subspan(20, 16)));
		else
			need(!nonzero(event));
		intent_semantics(intent);
		bytes normalized(command.begin(), command.begin() + prefix_size);
		normalized[4] = 1;
		normalized[31] = 0;
		std::fill(normalized.begin() + 32, normalized.begin() + 40, 0);
		normalized[32] = 1;
		need(same(tagged_hash("DURIS-ECONOMIC-COMMAND-V1", normalized),
			  intent.subspan(160, 32)));
		bytes domain;
		put(domain, type, 2);
		put(domain, payload_version, 2);
		put(domain, payload_size, 4);
		domain.insert(domain.end(), payload.begin(), payload.end());
		need(same(tagged_hash("DURIS-ECONOMIC-DOMAIN-V1", domain),
			  intent.subspan(192, 32)));
		if (result_code)
			return;
		need(same(plan.first(4), { reinterpret_cast<const uint8_t *>("EAP1"), 4 }) &&
		     number(plan, 4, 2) == 1 && !nonzero(plan.subspan(6, 2)) &&
		     same(plan.subspan(8, 64), intent.subspan(32, 64)) && plan[72] == intent[26] &&
		     !nonzero(plan.subspan(73, 3)) &&
		     same(plan.subspan(76, 8), intent.subspan(96, 8)) &&
		     same(plan.subspan(84, 14), intent.subspan(12, 14)) &&
		     !nonzero(plan.subspan(98, 2)) && plan[100] == intent[27] &&
		     !nonzero(plan.subspan(101, 3)) && same(plan.subspan(104, 48), event) &&
		     same(plan.subspan(152, 32), tagged_hash("DURIS-ECONOMIC-INTENT-V1", intent)) &&
		     same(plan.subspan(184, 32), intent.subspan(192, 32)) &&
		     !nonzero(plan.subspan(240, 16)));
		std::array<uint64_t, 6> counts;
		for (size_t i = 0; i < counts.size(); ++i)
			counts[i] = number(plan, 216 + 4 * i, 4);
		need(counts[0] <= 3072 && counts[1] <= 6144 && counts[2] == 0 &&
		     counts[3] <= 6000 && counts[4] <= 6000 && counts[5] <= 3000 &&
		     plan.size() == 256 + counts[0] * 120 + counts[1] * 48 +
					    (counts[3] + counts[4]) * 64 + counts[5] * 128);
		plan_semantics(plan, lineage);
		if (type == 20 || number(intent, 24, 2) == 38 || number(event, 0, 2) == 10)
		{
			// Native baselines retain dedupe in their witness/reservation book.
			need(type == 20 && number(intent, 24, 2) == 38 && intent[27] == 1 &&
			     number(intent, 12, 4) == 4 && intent[26] == 2 &&
			     !nonzero(intent.subspan(80, 16)) && number(event, 0, 2) == 10 &&
			     same(event.subspan(20, 16), intent.subspan(48, 16)) &&
			     number(event, 44, 4) == 0 && payload.size() == 48 &&
			     same(payload.first(4),
				  { reinterpret_cast<const uint8_t *>("EBC1"), 4 }) &&
			     number(payload, 4, 2) == 1 && number(payload, 6, 2) == 48 &&
			     !nonzero(payload.subspan(12, 4)) && nonzero(payload.subspan(16, 32)));
			need(payload_version == 1 && source == 6 && deadline == 4 && !publication &&
			     keys == 1 && identities[0] == key{ 9, 0x45434f4e42415345 } &&
			     !revisions && !result_size);
			baselines.observe(lineage, epoch, operation, intent, payload, plan,
					  durable_revision);
			return;
		}
		if (intent[27])
		{
			// The claim key is lineage-wide, including earlier retained epochs.
			bytes expected(lineage.begin(), lineage.end());
			expected.insert(expected.end(), event.begin(), event.end());
			const auto key = hash(expected);
			std::string name = "source-claim-";
			constexpr char digits[] = "0123456789abcdef";
			for (auto byte : key)
			{
				name += digits[byte >> 4];
				name += digits[byte & 15];
			}
			name += ".bin";
			expected.insert(expected.end(), operation.begin(), operation.end());
			put(expected, 1, 1);
			put(expected, 0, 7);
			need(frame(directory, name, "DURSCL1", 136) == expected);
			need(claimed_events.size() < buckets * bucket_capacity);
			claimed_events.push_back(key);
		}
	}
	std::vector<entry> index(size_t bucket)
	{
		auto name = filename("bucket-", bucket, ".eai");
		expected_files.insert(name);
		auto body = frame(directory, name, "DURECI1", index_limit);
		reader in{ body };
		need(in.fixed<16>() == lineage && in.number(4) == bucket);
		auto count = in.number(4), stored_total = in.number(8);
		need(count <= 4096 && stored_total <= bucket_limit &&
		     body.size() == 32 + count * 64);
		std::vector<entry> entries;
		uint64_t total = 0;
		for (size_t i = 0; i < count; ++i)
		{
			entry row{ in.fixed<16>(), in.fixed<32>(), in.number(4), in.number(4),
				   in.number(4) };
			need(in.number(4) == 0 && nonzero(row.operation) &&
			     row.operation[0] == bucket && nonzero(row.checksum) &&
			     row.size >= 48 && row.size <= record_limit && row.segment < count &&
			     row.segment < segment_count_limit &&
			     (entries.empty() || entries.back().operation < row.operation));
			total += row.size;
			entries.push_back(row);
		}
		in.done();
		need(total == stored_total);
		return entries;
	}
	void bucket(size_t bucket)
	{
		auto entries = index(bucket);
		if (entries.empty())
			return;
		std::sort(entries.begin(), entries.end(),
			  [](const auto &first, const auto &second) {
				  return std::tie(first.segment, first.offset) <
					 std::tie(second.segment, second.offset);
			  });
		const auto last_segment = entries.back().segment;
		size_t at = 0;
		for (size_t segment = 0; segment <= last_segment; ++segment)
		{
			auto segment_name =
				filename("bucket-", bucket, "-") + std::to_string(segment) + ".eas";
			expected_files.insert(segment_name);
			auto content = frame(directory, segment_name, "DURECS1", segment_limit);
			reader file{ content };
			need(file.fixed<16>() == lineage && file.number(4) == bucket &&
			     file.number(4) == segment);
			auto record_count = file.number(4);
			need(record_count > 0 && record_count <= entries.size() - at &&
			     file.number(4) == 0);
			uint64_t offset = 0;
			for (size_t i = 0; i < record_count; ++i)
			{
				const auto &row = entries[at++];
				need(row.segment == segment && row.offset == offset);
				auto encoded = file.take(row.size);
				need(hash(encoded) == row.checksum);
				record(encoded, row.operation);
				offset += row.size;
			}
			file.done();
		}
		need(at == entries.size());
	}

	std::map<uint64_t, bytes> selected_segments(size_t bucket, std::vector<entry> entries,
						    const std::vector<entry> &selected)
	{
		std::sort(entries.begin(), entries.end(),
			  [](const auto &a, const auto &b) {
				  return std::tie(a.segment, a.offset) <
					 std::tie(b.segment, b.offset);
			  });
		std::map<uint64_t, bytes> segments;
		for (const auto &selected_row : selected)
		{
			if (!segments.contains(selected_row.segment))
			{
				auto content =
					frame(directory,
					      filename("bucket-", bucket, "-") +
						      std::to_string(selected_row.segment) + ".eas",
					      "DURECS1", segment_limit);
				reader file{ content };
				need(file.fixed<16>() == lineage && file.number(4) == bucket &&
				     file.number(4) == selected_row.segment);
				const auto count = file.number(4);
				need(count && file.number(4) == 0);
				uint64_t offset = 0, observed = 0;
				for (const auto &row : entries)
					if (row.segment == selected_row.segment)
					{
						need(row.offset == offset &&
						     hash(file.take(row.size)) == row.checksum);
						offset += row.size;
						++observed;
					}
				file.done();
				need(count == observed);
				segments.emplace(selected_row.segment, std::move(content));
			}
		}
		return segments;
	}

    public:
	explicit checker(const std::filesystem::path &path)
		: root(path)
		, directory(path / "economic-evidence")
		, baselines(path)
		, lifecycles(path)
	{
	}
	root_page page(size_t bucket, const identity &after, const identity &ceiling,
		       bool ceiling_known)
	{
		need(bucket < buckets && (!nonzero(after) || ceiling_known) &&
		     (!ceiling_known || after <= ceiling));
		restore_economic_authority::checker authority(root);
		authority.begin_page();
		auto control = frame(directory, "authority.eal", "DURECA1");
		need(control.size() == 16552);
		std::copy_n(control.begin(), 16, lineage.begin());
		digest catalog_digest;
		std::copy_n(control.begin() + 104, 32, catalog_digest.begin());
		auto catalog = restore_economic_authority::catalog(directory, catalog_digest);
		need(catalog.lineage == lineage);
		for (const auto &entry : catalog.entries)
			epochs.insert(entry.epoch);
		root_page result;
		result.lineage = lineage;
		result.authority_body = hash(control);
		result.cursor = after;
		std::vector<entry> entries;
		if (control[16520 + bucket / 8] & (1u << (bucket % 8)))
			entries = index(bucket);
		else
		{
			struct stat info = {};
			need(lstat((directory / filename("bucket-", bucket, ".eai")).c_str(),
				   &info) == -1 &&
			     errno == ENOENT);
		}
		result.bucket_rows = entries.size();
		auto retained = [&](const identity &operation)
		{
			return std::any_of(entries.begin(), entries.end(),
					   [&](const auto &row)
					   { return row.operation == operation; });
		};
		need((!nonzero(after) || retained(after)) &&
		     (!ceiling_known || !nonzero(ceiling) || retained(ceiling)));
		result.ceiling = ceiling_known	 ? ceiling :
				 entries.empty() ? identity{} :
						   entries.back().operation;
		std::vector<entry> selected;
		bool more = false;
		for (const auto &row : entries)
			if (row.operation > after && row.operation <= result.ceiling)
			{
				if (selected.size() == 2)
				{
					more = true;
					break;
				}
				selected.push_back(row);
			}
		// Reuse bounded frames and the original semantic decoder. Inspect the
		// physical geometry and every record hash in each touched segment; only
		// the selected two records are semantically interpreted on this page.
		auto segments = selected_segments(bucket, entries, selected);
		for (const auto &selected_row : selected)
		{
			const auto &content = segments.at(selected_row.segment);
			try
			{
				record(std::span<const uint8_t>(content).subspan(
					       32 + selected_row.offset, selected_row.size),
				       selected_row.operation);
				++result.verified;
			}
			catch (const audit_budget_refused &)
			{
				throw;
			}
			catch (const std::runtime_error &)
			{
				result.invalid_records.push_back(selected_row.operation);
			}
			result.cursor = selected_row.operation;
			++result.rows;
		}
		result.exhausted = !more;
		return result;
	}
	// Required receipts are discovered from the immutable authority catalogue,
	// including old inactive epochs. A missing filename is a selected finding.
	root_page lifecycle_page(size_t bucket, const identity &after, const identity &ceiling,
				 bool ceiling_known)
	{
		need(bucket < buckets && (!nonzero(after) || ceiling_known) &&
		     (!ceiling_known || after <= ceiling));
		restore_economic_authority::checker authority(root);
		authority.begin_page();
		auto control = frame(directory, "authority.eal", "DURECA1");
		need(control.size() == 16552);
		std::copy_n(control.begin(), 16, lineage.begin());
		digest catalog_digest;
		std::copy_n(control.begin() + 104, 32, catalog_digest.begin());
		auto catalog = restore_economic_authority::catalog(directory, catalog_digest);
		need(catalog.lineage == lineage);
		std::vector<identity> operations;
		for (const auto &marker : catalog.entries)
		{
			epochs.insert(marker.epoch);
			if (marker.origin == initialization_origin::lifecycle_owner &&
			    marker.initializing_operation[0] == bucket)
				operations.push_back(marker.initializing_operation);
		}
		std::sort(operations.begin(), operations.end());
		auto retained = [&](const identity &operation)
		{ return std::binary_search(operations.begin(), operations.end(), operation); };
		need((!nonzero(after) || retained(after)) &&
		     (!ceiling_known || !nonzero(ceiling) || retained(ceiling)));
		root_page result;
		result.lineage = lineage;
		result.authority_body = hash(control);
		result.cursor = after;
		result.bucket_rows = operations.size();
		result.ceiling = ceiling_known	    ? ceiling :
				 operations.empty() ? identity{} :
						      operations.back();
		bool more = false;
		for (const auto &operation : operations)
			if (operation > after && operation <= result.ceiling)
			{
				if (result.rows == 2)
				{
					more = true;
					break;
				}
				try
				{
					// Each receipt gets isolated link/semantic state: one damaged
					// receipt cannot poison a healthy sibling on the same page.
					checker selected(root);
					selected.lineage = lineage;
					selected.epochs = epochs;
					auto original = selected.lifecycles.load_one(
						operation, lineage, catalog, control,
						[&](auto account)
						{ return authority.mapped_account(account); });
					const auto root_bucket = original[0];
					need(control[16520 + root_bucket / 8] &
					     (1u << (root_bucket % 8)));
					auto entries = selected.index(root_bucket);
					auto found =
						std::find_if(entries.begin(), entries.end(),
							     [&](const auto &row)
							     { return row.operation == original; });
					need(found != entries.end());
					auto segments = selected.selected_segments(
						root_bucket, entries, { *found });
					selected.record(std::span<const uint8_t>(
								segments.at(found->segment))
								.subspan(32 + found->offset,
									 found->size),
							original);
					need(selected.lifecycles.finish() == 1);
					++result.verified;
				}
				catch (const audit_budget_refused &)
				{
					throw;
				}
				catch (const std::runtime_error &)
				{
					result.invalid_records.push_back(operation);
				}
				result.cursor = operation;
				++result.rows;
			}
		result.exhausted = !more;
		return result;
	}
	// Catalogue-required books can exist without any retained root. Validate their
	// controls and every reservation/terminal reference independently of filenames.
	// Earlier empty roots have no reservation; this page cannot close their history.
	root_page baseline_controls_page(size_t bucket, const identity &after,
					 const identity &ceiling, bool ceiling_known)
	{
		need(bucket < buckets && (!nonzero(after) || ceiling_known) &&
		     (!ceiling_known || after <= ceiling));
		restore_economic_authority::checker authority(root);
		authority.begin_page();
		auto control = frame(directory, "authority.eal", "DURECA1");
		need(control.size() == 16552);
		std::copy_n(control.begin(), 16, lineage.begin());
		digest catalog_digest;
		std::copy_n(control.begin() + 104, 32, catalog_digest.begin());
		auto catalog = restore_economic_authority::catalog(directory, catalog_digest);
		need(catalog.lineage == lineage);
		std::vector<epoch_marker> books;
		for (const auto &marker : catalog.entries)
		{
			epochs.insert(marker.epoch);
			if (marker.initialization == baseline_initialization::initialized &&
			    marker.epoch[0] == bucket)
				books.push_back(marker);
		}
		std::sort(books.begin(), books.end(),
			  [](const auto &a, const auto &b) { return a.epoch < b.epoch; });
		auto retained = [&](const identity &epoch)
		{
			return std::any_of(books.begin(), books.end(),
					   [&](const auto &marker)
					   { return marker.epoch == epoch; });
		};
		need((!nonzero(after) || retained(after)) &&
		     (!ceiling_known || !nonzero(ceiling) || retained(ceiling)));
		root_page result;
		result.lineage = lineage;
		result.authority_body = hash(control);
		result.cursor = after;
		result.bucket_rows = books.size();
		result.ceiling = ceiling_known ? ceiling :
				 books.empty() ? identity{} :
						 books.back().epoch;
		bool more = false;
		for (const auto &marker : books)
			if (marker.epoch > after && marker.epoch <= result.ceiling)
			{
				if (result.rows == 2)
				{
					more = true;
					break;
				}
				try
				{
					checker selected(root);
					selected.lineage = lineage;
					selected.epochs = epochs;
					const auto book =
						selected.baselines.control(lineage, marker);
					std::array<
						std::vector<restore_economic_baseline::reservation>,
						16>
						actual;
					std::set<identity> operations;
					if (book.revision)
						operations.insert(book.terminal);
					for (size_t slot = 0; slot < actual.size(); ++slot)
					{
						actual[slot] = selected.baselines.reservations(
							lineage, marker.epoch, slot,
							book.checksums[slot]);
						need(book.revision || actual[slot].empty());
						for (const auto &row : actual[slot])
							operations.insert(row.operation);
					}
					need(operations.size() <= buckets * bucket_capacity);
					for (const auto &operation : operations)
					{
						const auto root_bucket = operation[0];
						need(control[16520 + root_bucket / 8] &
						     (1u << (root_bucket % 8)));
						auto entries = selected.index(root_bucket);
						auto found = std::find_if(
							entries.begin(), entries.end(),
							[&](const auto &row)
							{ return row.operation == operation; });
						need(found != entries.end());
						auto segments = selected.selected_segments(
							root_bucket, entries, { *found });
						selected.record(std::span<const uint8_t>(
									segments.at(found->segment))
									.subspan(32 + found->offset,
										 found->size),
								operation);
					}
					bool terminal = !book.revision;
					std::set<uint64_t> revisions;
					std::span<const restore_economic_baseline::root> originals;
					if (!operations.empty())
					{
						const auto &observed =
							selected.baselines.observed(marker.epoch);
						need(observed.size() == operations.size());
						originals = observed;
						for (const auto &entry : observed)
						{
							need(entry.revision <= book.revision &&
							     revisions.insert(entry.revision)
								     .second);
							if (entry.operation == book.terminal)
							{
								need(entry.revision ==
								     book.revision);
								terminal = true;
							}
						}
					}
					need(terminal);
					auto expected = selected.baselines.expected_reservations(
						lineage, marker.epoch, book.opening, originals);
					for (size_t slot = 0; slot < actual.size(); ++slot)
					{
						std::sort(expected[slot].begin(),
							  expected[slot].end(),
							  restore_economic_baseline::less);
						need(actual[slot].size() == expected[slot].size());
						for (size_t i = 0; i < actual[slot].size(); ++i)
							need((!i || restore_economic_baseline::less(
									    expected[slot][i - 1],
									    expected[slot][i])) &&
							     actual[slot][i].kind ==
								     expected[slot][i].kind &&
							     actual[slot][i].id ==
								     expected[slot][i].id &&
							     actual[slot][i].operation ==
								     expected[slot][i].operation);
					}
					++result.verified;
				}
				catch (const audit_budget_refused &)
				{
					throw;
				}
				catch (const std::runtime_error &)
				{
					result.invalid_records.push_back(marker.epoch);
				}
				result.cursor = marker.epoch;
				++result.rows;
			}
		result.exhausted = !more;
		return result;
	}
	initialization_provenance run()
	{
		initialization_provenance provenance;
		// Pure metadata validation, distinct from candidate journal recovery.
		restore_economic_authority::checker authority(root);
		authority.run();
		if (!std::filesystem::exists(directory) || std::filesystem::is_empty(directory))
			return provenance;
		auto control = frame(directory, "authority.eal", "DURECA1");
		need(control.size() == 16552);
		std::copy_n(control.begin(), 16, lineage.begin());
		digest catalog_digest;
		std::copy_n(control.begin() + 104, 32, catalog_digest.begin());
		auto catalog = restore_economic_authority::catalog(directory, catalog_digest);
		need(catalog.lineage == lineage);
		lifecycles.load(lineage, catalog, control,
				[&](auto account) { return authority.mapped_account(account); });
		for (const auto &entry : catalog.entries)
		{
			epochs.insert(entry.epoch);
			if (entry.initialization == baseline_initialization::legacy_unknown)
				++provenance.legacy_unknown_epochs;
			else if (entry.initialization == baseline_initialization::never_initialized)
				++provenance.never_initialized_epochs;
			else
			{
				++provenance.initialized_epochs;
				if (entry.origin == initialization_origin::legacy_unknown)
					++provenance.unknown_initialized_origins;
				else if (entry.origin ==
					 initialization_origin::baseline_participant)
					++provenance.baseline_participant_epochs;
				else
					++provenance.lifecycle_owner_epochs;
			}
		}
		for (size_t index = 0; index < 256; ++index)
			if (control[16520 + index / 8] & (1u << (index % 8)))
				bucket(index);
		baselines.finish(lineage, catalog.entries);
		provenance.lifecycle_receipts = lifecycles.finish();
		// At most 256 * 4096 keys (32 MiB of digests), bounded by the native
		// index format. Keep no unbounded map of roots or retained record bytes.
		std::sort(claimed_events.begin(), claimed_events.end());
		need(std::adjacent_find(claimed_events.begin(), claimed_events.end()) ==
		     claimed_events.end());
		size_t observed_claims = 0;
		for (const auto &file : std::filesystem::directory_iterator(directory))
		{
			audit_directory_entry();
			const auto name = file.path().filename().string();
			if (name.starts_with("bucket-"))
				need(expected_files.contains(name));
			else if (name.starts_with("source-claim"))
			{
				need(name.size() == 81 && name.starts_with("source-claim-") &&
				     name.ends_with(".bin"));
				digest key = {};
				for (size_t i = 0; i < 64; ++i)
				{
					const auto c = name[13 + i];
					need((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
					key[i / 2] = static_cast<uint8_t>(
						(key[i / 2] << 4) |
						(c <= '9' ? c - '0' : c - 'a' + 10));
				}
				need(std::binary_search(claimed_events.begin(),
							claimed_events.end(), key));
				++observed_claims;
			}
		}
		need(observed_claims == claimed_events.size());
		return provenance;
	}
};
inline initialization_provenance audit(const std::filesystem::path &root,
				       restore_economic_authority::audit_budget &budget)
{
	restore_economic_authority::scoped_audit_budget scope(budget);
	restore_economic_authority::authority_read_lock lock(root);
	audit_checkpoint();
	initialization_provenance result;
	if (lock.locked())
		result = checker(root).run();
	lock.finish();
	audit_checkpoint();
	return result;
}
} // namespace restore_economic_records
#endif
