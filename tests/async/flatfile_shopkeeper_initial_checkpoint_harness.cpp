// Prepared native regression; execution deferred until major-plan readiness.
// Synthetic fixtures only. Framing revision 1 is never an actual catalog clock.
#include "flatfile/flatfile_shopkeeper_repository.h"

#include <algorithm>
#include <array>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <limits>
#include <openssl/sha.h>
#include <string>
#include <tuple>
#include <vector>

namespace fs = std::filesystem;
using bytes = std::vector<uint8_t>;

// DURSHOPv2 wire offsets, independent of native object layout/padding.
constexpr size_t header_bytes = 56;
constexpr size_t count_offset = 56;
constexpr size_t record_offset = 60;
constexpr size_t revision_offset = 80;
constexpr size_t cash_offset = 88;
constexpr size_t roaming_offset = 96;
constexpr size_t affect_count_offset = 97;
constexpr size_t affects_offset = 101;
constexpr size_t affect_bytes = 56;
constexpr size_t affect_limit = 4096;
constexpr size_t checkpoint_limit = 105 + affect_limit * affect_bytes + PLAYER_SNAPSHOT_MAX_BYTES;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		std::exit(1);
	}
}

static void put(bytes &value, size_t offset, uint64_t number, size_t width)
{
	require(width <= 8 && offset <= value.size() && width <= value.size() - offset,
		"bad fixture mutation extent");
	for (size_t index = 0; index < width; ++index)
		value[offset + index] = static_cast<uint8_t>(number >> (8 * index));
}

static void reframe(bytes &value)
{
	require(value.size() >= header_bytes, "short checksum fixture");
	put(value, 12, value.size() - header_bytes, 4);
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(value.data() + header_bytes, value.size() - header_bytes, digest.data());
	std::copy(digest.begin(), digest.end(), value.begin() + 24);
}

static bytes encode(const flatfile_shopkeeper_record &record)
{
	bytes result;
	require(flatfile_shopkeeper_initial_checkpoint_encode(record, &result),
		"valid INITIAL fixture did not encode");
	return result;
}

static bool same_affect(const flatfile_shopkeeper_affect_record &left,
			const flatfile_shopkeeper_affect_record &right)
{
	return std::tie(left.type, left.duration, left.modifier, left.location, left.bitvectors) ==
	       std::tie(right.type, right.duration, right.modifier, right.location,
			right.bitvectors);
}

static bool same_item(const player_item_snapshot &left, const player_item_snapshot &right)
{
	if (std::tie(left.parent_index, left.equipment_slot, left.object_uid, left.generated_key,
		     left.vnum, left.type, left.string_mask, left.name, left.short_description,
		     left.description, left.action_description, left.values, left.timers,
		     left.wear_flags, left.extra_flags, left.anti_flags, left.anti2_flags,
		     left.extra2_flags, left.weight, left.material, left.cost, left.condition,
		     left.craftsmanship, left.bitvectors, left.affects) !=
		    std::tie(right.parent_index, right.equipment_slot, right.object_uid,
			     right.generated_key, right.vnum, right.type, right.string_mask,
			     right.name, right.short_description, right.description,
			     right.action_description, right.values, right.timers, right.wear_flags,
			     right.extra_flags, right.anti_flags, right.anti2_flags,
			     right.extra2_flags, right.weight, right.material, right.cost,
			     right.condition, right.craftsmanship, right.bitvectors,
			     right.affects) ||
	    left.dynamic_affects.size() != right.dynamic_affects.size() ||
	    left.extra_descriptions.size() != right.extra_descriptions.size())
		return false;
	for (size_t index = 0; index < left.dynamic_affects.size(); ++index)
	{
		const auto &a = left.dynamic_affects[index];
		const auto &b = right.dynamic_affects[index];
		if (std::tie(a.type, a.data, a.extra2) != std::tie(b.type, b.data, b.extra2))
			return false;
	}
	for (size_t index = 0; index < left.extra_descriptions.size(); ++index)
	{
		const auto &a = left.extra_descriptions[index];
		const auto &b = right.extra_descriptions[index];
		if (std::tie(a.keyword, a.description, a.spellbook, a.spell_ids) !=
		    std::tie(b.keyword, b.description, b.spellbook, b.spell_ids))
			return false;
	}
	return true;
}

static void expect_record(const flatfile_shopkeeper_record &actual,
			  const flatfile_shopkeeper_record &expected, const std::string &label)
{
	require(std::tie(actual.shop_id, actual.mob_vnum, actual.room_vnum, actual.saved_at,
			 actual.revision, actual.cash, actual.roaming) ==
			std::tie(expected.shop_id, expected.mob_vnum, expected.room_vnum,
				 expected.saved_at, expected.revision, expected.cash,
				 expected.roaming),
		label + ": scalar state changed");
	require(actual.affects.size() == expected.affects.size() &&
			actual.items.size() == expected.items.size(),
		label + ": collection extent changed");
	for (size_t index = 0; index < actual.affects.size(); ++index)
		require(same_affect(actual.affects[index], expected.affects[index]),
			label + ": affect state or multiplicity changed");
	for (size_t index = 0; index < actual.items.size(); ++index)
		require(same_item(actual.items[index], expected.items[index]),
			label + ": item literal or forest state changed");
}

static player_item_snapshot item(uint64_t uid, int32_t parent, int16_t slot)
{
	player_item_snapshot value = {};
	value.parent_index = parent;
	value.equipment_slot = slot;
	value.object_uid = uid;
	value.generated_key = -1234567890123LL;
	value.vnum = 401;
	value.type = 15;
	value.string_mask = 15;
	value.name = "synthetic keeper satchel";
	value.short_description = "a stitched satchel";
	value.description = "A stitched satchel rests here.\r\n";
	value.action_description = std::string("literal\0action", 14);
	value.values = { -17, 2, 3, 4, 5, 6, 7, 8001 };
	value.timers = { 0, -1, 20000000000LL, 4, 5, 6 };
	value.wear_flags = 0x80000001;
	value.extra_flags = 0x01020304;
	value.anti_flags = 0x00000008;
	value.anti2_flags = 0x00000010;
	value.extra2_flags = 0x40000020;
	value.weight = 12;
	value.material = 3;
	value.cost = 12345;
	value.condition = 93;
	value.craftsmanship = 47;
	value.bitvectors = { 1, 2, 4, 8, 0x8000000000000010ULL };
	value.affects = { { { 1, -2 }, { 3, 4 }, { 5, 6 }, { 7, 8 } } };
	value.dynamic_affects = { { 3, -4, 0x8000000000000005ULL }, { 3, -4, 5 } };
	value.extra_descriptions = { { "mark", "a maker's mark\n", false, {} },
				     { "spells", "a copied spell list", true, { 12, 37, 99 } } };
	return value;
}

static flatfile_shopkeeper_record empty_record()
{
	flatfile_shopkeeper_record value;
	value.shop_id = 0; // Zero is an existing valid shop identifier.
	value.mob_vnum = 11005;
	value.room_vnum = 22007;
	value.saved_at = 20000000000LL;
	value.revision = 1;
	return value;
}

static flatfile_shopkeeper_record realistic_record()
{
	auto value = empty_record();
	value.shop_id = 7;
	value.cash = 123456;
	value.roaming = true;
	value.affects = { { 9, 20, -3, 4, { 1, 2, 4, 8, 0x8000000000000010ULL } },
			  { 2, 10, 5, 6, { 32, 64, 128, 256, 512 } } };
	value.items = { item(9001, PLAYER_SNAPSHOT_NO_PARENT, 3),
			item(9002, 0, 0),
			item(9003, 1, 0),
			item(9004, PLAYER_SNAPSHOT_NO_PARENT, 0),
			item(9005, 3, 0),
			item(9006, PLAYER_SNAPSHOT_NO_PARENT, 7) };
	return value;
}

static void rejected_encode(const flatfile_shopkeeper_record &record, const std::string &label)
{
	const bytes sentinel = { 0x42, 0, 0xff, 0x73 };
	bytes output = sentinel;
	require(!flatfile_shopkeeper_initial_checkpoint_encode(record, &output),
		label + ": invalid record encoded");
	require(output == sentinel, label + ": failed encode altered caller output");
}

static void rejected_decode(const bytes &input, const std::string &label)
{
	auto output = realistic_record();
	std::reverse(output.affects.begin(), output.affects.end());
	const auto before = output;
	const auto canonical_before = encode(output);
	require(!flatfile_shopkeeper_initial_checkpoint_decode(input, &output),
		label + ": invalid bytes decoded");
	expect_record(output, before, label + ": failed decode output");
	require(encode(output) == canonical_before,
		label + ": failed decode changed canonical output");
}

static void round_trip(const flatfile_shopkeeper_record &record, const std::string &label)
{
	const auto wire = encode(record);
	flatfile_shopkeeper_record output;
	require(flatfile_shopkeeper_initial_checkpoint_decode(wire, &output),
		label + ": decode failed");
	expect_record(output, record, label);
	require(encode(output) == wire, label + ": canonical bytes changed after round trip");
}

static void check_round_trips()
{
	auto original = realistic_record();
	const auto before = original;
	const auto wire = encode(original);
	expect_record(original, before, "encode input preservation");
	std::reverse(original.affects.begin(), original.affects.end());
	flatfile_shopkeeper_record decoded;
	require(flatfile_shopkeeper_initial_checkpoint_decode(wire, &decoded), "realistic decode");
	expect_record(decoded, original, "realistic nested EQ/INV and all literals");
	require(encode(original) == wire, "affect input order affected canonical bytes");
	round_trip(empty_record(), "zero cash and empty forest");
	auto maximum_cash = empty_record();
	maximum_cash.cash = std::numeric_limits<int>::max();
	round_trip(maximum_cash, "maximum observed cash");

	// Deliberately state the expected original affect ordering, including all tie-breaks.
	const flatfile_shopkeeper_affect_record a{ 1, 9, 9, 9, { 9, 9, 9, 9, 9 } };
	const flatfile_shopkeeper_affect_record b{ 2, 9, 9, 1, { 9, 9, 9, 9, 9 } };
	const flatfile_shopkeeper_affect_record c{ 2, 9, 1, 2, { 9, 9, 9, 9, 9 } };
	const flatfile_shopkeeper_affect_record d{ 2, 1, 2, 2, { 9, 9, 9, 9, 9 } };
	const flatfile_shopkeeper_affect_record e{ 2, 2, 2, 2, { 1, 2, 3, 4, 5 } };
	const flatfile_shopkeeper_affect_record f{ 2, 2, 2, 2, { 1, 2, 3, 4, 6 } };
	auto ordered = empty_record();
	ordered.affects = { a, b, c, d, e, e, f };
	auto shuffled = ordered;
	shuffled.affects = { f, e, d, c, e, b, a };
	const auto shuffled_before = shuffled;
	require(encode(shuffled) == encode(ordered),
		"canonical affect order or duplicate count changed");
	expect_record(shuffled, shuffled_before, "sorting did not mutate input");
	round_trip(ordered, "canonical affects and duplicate multiplicity");
}

static void check_invalid_records()
{
	const auto base = empty_record();
	for (const int64_t cash :
	     { -2LL, -1LL, static_cast<long long>(std::numeric_limits<int>::max()) + 1 })
	{
		auto bad = base;
		bad.cash = cash;
		rejected_encode(bad, "unobserved or out-of-range cash");
	}
	for (const uint64_t revision : { uint64_t{ 0 }, uint64_t{ 2 }, UINT64_MAX })
	{
		auto bad = base;
		bad.revision = revision;
		rejected_encode(bad, "non-INITIAL revision");
	}
	auto bad = base;
	bad.mob_vnum = 0;
	rejected_encode(bad, "missing mobile");
	bad = base;
	bad.room_vnum = -1;
	rejected_encode(bad, "invalid room");
	bad = base;
	bad.saved_at = -1;
	rejected_encode(bad, "negative saved time");
	bad = realistic_record();
	bad.items[1].object_uid = bad.items[0].object_uid;
	rejected_encode(bad, "duplicate object UID");
	bad = realistic_record();
	bad.items[1].parent_index = 1;
	rejected_encode(bad, "self-parented object");
	bad = realistic_record();
	bad.items[1].equipment_slot = 1;
	rejected_encode(bad, "equipped child");
	bad = realistic_record();
	bad.items.back().equipment_slot = 3;
	rejected_encode(bad, "duplicate EQ slot");
	bad = realistic_record();
	bad.items[0].equipment_slot = 256;
	rejected_encode(bad, "EQ slot over 255");
	bad.items[0].equipment_slot = -1;
	rejected_encode(bad, "negative EQ slot");
	bad = realistic_record();
	bad.items[0].vnum = 0;
	rejected_encode(bad, "invalid object prototype");
	bad.items[0].vnum = 401;
	bad.items[0].object_uid = 0;
	rejected_encode(bad, "missing object UID");
	require(!flatfile_shopkeeper_initial_checkpoint_encode(base, nullptr),
		"null encode output");
	require(!flatfile_shopkeeper_initial_checkpoint_decode(encode(base), nullptr),
		"null decode output");
}

static void check_malformed_frames()
{
	const auto minimal = encode(empty_record());
	const auto realistic = encode(realistic_record());
	auto bad = minimal;
	bad[24] ^= 1;
	rejected_decode(bad, "checksum corruption");
	bad = minimal;
	bad[0] ^= 1;
	rejected_decode(bad, "wrong magic");
	for (const uint64_t version : { 0, 1, 3 })
	{
		bad = minimal;
		put(bad, 8, version, 4);
		rejected_decode(bad, "unsupported checkpoint version");
	}
	bad = minimal;
	put(bad, 12, bad.size() - header_bytes + 1, 4);
	rejected_decode(bad, "header payload length mismatch");
	for (const uint64_t revision : { uint64_t{ 0 }, uint64_t{ 2 }, UINT64_MAX })
	{
		bad = minimal;
		put(bad, 16, revision, 8);
		rejected_decode(bad, "noncanonical framing revision");
		bad = minimal;
		put(bad, revision_offset, revision, 8);
		reframe(bad);
		rejected_decode(bad, "non-INITIAL contained revision");
	}
	for (const uint64_t count : { 0, 2, 262145 })
	{
		bad = minimal;
		put(bad, count_offset, count, 4);
		reframe(bad);
		rejected_decode(bad, "record count must be exactly one");
	}
	for (const uint64_t cash : { UINT64_MAX, uint64_t{ 2147483648ULL } })
	{
		bad = minimal;
		put(bad, cash_offset, cash, 8);
		reframe(bad);
		rejected_decode(bad, "invalid observed cash bytes");
	}
	bad = minimal;
	put(bad, roaming_offset, 2, 1);
	reframe(bad);
	rejected_decode(bad, "nonboolean roaming");
	for (const size_t offset : { record_offset + 4, record_offset + 8 })
	{
		bad = minimal;
		put(bad, offset, 0, 4);
		reframe(bad);
		rejected_decode(bad, "invalid mobile or room bytes");
	}
	bad = minimal;
	put(bad, record_offset + 12, UINT64_MAX, 8);
	reframe(bad);
	rejected_decode(bad, "negative saved time bytes");
	for (const uint64_t count : std::array<uint64_t, 3>{ 1, 4097, 0xffffffffULL })
	{
		bad = minimal;
		put(bad, affect_count_offset, count, 4);
		reframe(bad);
		rejected_decode(bad, "affect extent or count bound");
	}
	bad = realistic;
	std::swap_ranges(bad.begin() + affects_offset, bad.begin() + affects_offset + affect_bytes,
			 bad.begin() + affects_offset + affect_bytes);
	reframe(bad);
	rejected_decode(bad, "checksum-valid noncanonical affect order");
	for (const uint64_t length : std::array<uint64_t, 5>{ 0, 3, 5, PLAYER_SNAPSHOT_MAX_BYTES,
							      PLAYER_SNAPSHOT_MAX_BYTES + 1 })
	{
		bad = minimal;
		put(bad, affects_offset, length, 4);
		reframe(bad);
		rejected_decode(bad, "item byte extent or length bound");
	}
	bad = minimal;
	put(bad, affects_offset + 4, PLAYER_SNAPSHOT_MAX_OBJECTS + 1, 4);
	reframe(bad);
	rejected_decode(bad, "forged item count allocation bound");
	const size_t item_length_offset = affects_offset + 2 * affect_bytes;
	const size_t first_item_offset = item_length_offset + 4 + 4;
	for (const auto &mutation : std::array<std::tuple<size_t, uint64_t, size_t>, 3>{
		     std::tuple{ first_item_offset, uint64_t{ 0 }, size_t{ 4 } },
		     std::tuple{ first_item_offset + 4, uint64_t{ 0xffff }, size_t{ 2 } },
		     std::tuple{ first_item_offset + 6, uint64_t{ 0 }, size_t{ 8 } } })
	{
		bad = realistic;
		put(bad, std::get<0>(mutation), std::get<1>(mutation), std::get<2>(mutation));
		reframe(bad);
		rejected_decode(bad, "checksum-valid invalid item parent, slot or UID");
	}
	bad.assign(realistic.begin(), realistic.begin() + first_item_offset + 13);
	put(bad, item_length_offset, bad.size() - item_length_offset - 4, 4);
	reframe(bad);
	rejected_decode(bad, "checksum-valid short item UID field");
	bad = minimal;
	bad.insert(bad.end(), minimal.begin() + record_offset, minimal.end());
	put(bad, count_offset, 2, 4);
	reframe(bad);
	rejected_decode(bad, "complete two-record catalog is not one INITIAL record");
	bad = minimal;
	bad.push_back(0);
	rejected_decode(bad, "trailing byte without updated header");
	reframe(bad);
	rejected_decode(bad, "checksum-valid trailing byte outside item extent");
	put(bad, affects_offset, 5, 4);
	reframe(bad);
	rejected_decode(bad, "checksum-valid trailing byte inside item extent");
	for (const size_t length : { size_t{ 0 }, header_bytes - 1, record_offset + 1,
				     cash_offset + 7, affect_count_offset + 3, minimal.size() - 1 })
	{
		bad.assign(minimal.begin(), minimal.begin() + length);
		if (length >= header_bytes)
			reframe(bad);
		rejected_decode(bad, "short frame or scalar field");
	}
	bad.assign(checkpoint_limit + 1, 0);
	std::copy(minimal.begin(), minimal.end(), bad.begin());
	reframe(bad);
	rejected_decode(bad, "bounded checkpoint byte ceiling plus one");
}

static void check_bounded_limits()
{
	auto record = empty_record();
	record.affects.resize(affect_limit);
	round_trip(record, "4096 duplicate affects at limit");
	record.affects.emplace_back();
	rejected_encode(record, "affect count limit plus one");
	record = empty_record();
	player_item_snapshot simple = {};
	simple.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	simple.vnum = 401;
	for (size_t index = 0; index < PLAYER_SNAPSHOT_MAX_OBJECTS; ++index)
	{
		simple.object_uid = index + 1;
		record.items.push_back(simple);
	}
	round_trip(record, "4096 distinct root objects at limit");
	simple.object_uid++;
	record.items.push_back(simple);
	rejected_encode(record, "object count limit plus one");
	record = empty_record();
	for (size_t index = 0; index < PLAYER_SNAPSHOT_MAX_DEPTH; ++index)
	{
		simple.parent_index = index ? static_cast<int32_t>(index - 1) :
					      PLAYER_SNAPSHOT_NO_PARENT;
		simple.object_uid = index + 1;
		record.items.push_back(simple);
	}
	round_trip(record, "nested forest at depth limit");
	simple.parent_index = static_cast<int32_t>(record.items.size() - 1);
	simple.object_uid++;
	record.items.push_back(simple);
	rejected_encode(record, "nested forest depth limit plus one");
	record = empty_record();
	simple.parent_index = PLAYER_SNAPSHOT_NO_PARENT;
	simple.name.assign(PLAYER_SNAPSHOT_MAX_STRING_BYTES, 'x');
	record.items = { simple };
	round_trip(record, "literal string at byte limit");
	record.items[0].name.push_back('x');
	rejected_encode(record, "literal string byte limit plus one");
}

static void write_fixture(const fs::path &path, const bytes &value)
{
	std::ofstream output(path, std::ios::binary | std::ios::trunc);
	output.write(reinterpret_cast<const char *>(value.data()), value.size());
	output.close();
	require(static_cast<bool>(output), "could not write disposable catalog fixture");
}

static void check_original_catalog_compatibility(const fs::path &root)
{
	fs::create_directories(root / "domains");
	fs::create_directories(root / "players");
	fs::create_directories(root / "identities/names");
	for (const auto &directory : { root, root / "domains", root / "players",
				       root / "identities", root / "identities/names" })
		fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	const auto record = empty_record();
	const auto passive = encode(record);
	require(flatfile_shopkeeper_establish(root.string(), { record }, &error) ==
			flatfile_shopkeeper_result::ok,
		"could not establish disposable original catalog fixture");
	const auto filename = root / "domains/shopkeeper_catalog";
	std::ifstream input(filename, std::ios::binary);
	const bytes established(std::istreambuf_iterator<char>{ input }, {});
	input.close();
	require(established == passive, "INITIAL bytes differ from existing one-record DURSHOPv2");
	std::vector<flatfile_shopkeeper_record> listed;
	require(flatfile_shopkeeper_list(root.string(), &listed, &error) ==
				flatfile_shopkeeper_result::ok &&
			listed.size() == 1,
		"existing catalog reader rejected INITIAL bytes");
	expect_record(listed.front(), record, "ordinary reader compatibility");
	auto later = passive;
	put(later, 16, 9, 8); // Actual fixture catalog clock intentionally differs from framing.
	put(later, revision_offset, 2, 8);
	reframe(later);
	write_fixture(filename, later);
	require(flatfile_shopkeeper_list(root.string(), &listed, &error) ==
				flatfile_shopkeeper_result::ok &&
			listed.size() == 1 && listed.front().revision == 2,
		"ordinary catalog reader lost non-INITIAL revision support");
	rejected_decode(later, "ordinary later catalog is not a passive INITIAL checkpoint");
	auto legacy = passive;
	legacy.erase(legacy.begin() + cash_offset, legacy.begin() + roaming_offset + 1);
	put(legacy, 8, 1, 4);
	reframe(legacy);
	write_fixture(filename, legacy);
	require(flatfile_shopkeeper_list(root.string(), &listed, &error) ==
				flatfile_shopkeeper_result::ok &&
			listed.size() == 1 && listed.front().cash == -1 && !listed.front().roaming,
		"existing v1 compatibility or unknown-cash state changed");
	rejected_decode(legacy, "valid original v1 catalog is not an observed-cash checkpoint");
}

int main(int argc, char **argv)
{
	require(argc == 2, "disposable fixture root argument required");
	check_round_trips();
	check_invalid_records();
	check_malformed_frames();
	check_bounded_limits();
	check_original_catalog_compatibility(fs::path(argv[1]) / "compatibility");
	std::cout << "flatfile shopkeeper INITIAL checkpoint regression passed\n";
	return 0;
}
