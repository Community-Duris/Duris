// Independent retained-baseline proof. Never call native baseline codecs,
// storage readers, journal recovery, initialization, or lifecycle owners here.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_BASELINE_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_BASELINE_H
#include "qualify_flatfile_economic_authority.h"
#include <map>
#include <tuple>

namespace restore_economic_baseline
{
using namespace restore_economic_authority;
constexpr size_t witness_limit = 192 + 3071 * 112 + 6000 * 96;
constexpr size_t reservation_capacity = 65536, reservation_limit = 88 + reservation_capacity * 32;
constexpr char hex_digits[] = "0123456789abcdef";
inline std::string hex(std::span<const uint8_t> value)
{
	std::string result;
	for (auto byte : value)
	{
		result += hex_digits[byte >> 4];
		result += hex_digits[byte & 15];
	}
	return result;
}
inline identity unhex(const std::string &value)
{
	need(value.size() == 32);
	identity result = {};
	for (size_t i = 0; i < value.size(); ++i)
	{
		auto c = value[i];
		need((c >= '0' && c <= '9') || (c >= 'a' && c <= 'f'));
		result[i / 2] = static_cast<uint8_t>((result[i / 2] << 4) |
						     (c <= '9' ? c - '0' : c - 'a' + 10));
	}
	need(nonzero(result));
	return result;
}
inline std::string prefix(const identity &lineage, const identity &epoch)
{
	return "baseline-" + hex(lineage) + "-" + hex(epoch) + "-";
}
inline auto account(std::span<const uint8_t> key, const identity &lineage)
{
	need(key.size() == 40 && same(key.first(16), lineage) && number(key, 16, 2) == 1 &&
	     !nonzero(key.subspan(36, 4)));
	auto kind = number(key, 18, 2), id = number(key, 20, 8), context = number(key, 28, 8);
	need(kind >= 1 && kind <= 11 && id);
	return std::tuple{ kind, id, context };
}
inline void append(bytes &out, std::span<const uint8_t> value)
{
	out.insert(out.end(), value.begin(), value.end());
}
inline size_t item_bytes(std::span<const uint8_t> witness)
{
	need(witness.size() >= 192 && witness.size() <= witness_limit);
	auto version = number(witness, 4, 2);
	need((version == 1 &&
	      same(witness.first(4), { reinterpret_cast<const uint8_t *>("EAB1"), 4 })) ||
	     (version == 2 &&
	      same(witness.first(4), { reinterpret_cast<const uint8_t *>("EAB2"), 4 })));
	need(number(witness, 6, 2) == 192 && number(witness, 8, 4) == witness.size() &&
	     !number(witness, 12, 4));
	auto holdings = number(witness, 184, 4), items = number(witness, 188, 4);
	const size_t stride = version == 1 ? 88 : 96;
	need(holdings <= 3071 && items <= 6000 &&
	     witness.size() == 192 + holdings * 112 + items * stride);
	return stride;
}
// Baseline item positions are unchanged, but their complete live forest must
// still be canonical. Destroyed rows retain former edges without following them.
inline void forest(std::span<const uint8_t> items, size_t stride)
{
	need((stride == 88 || stride == 96) && items.size() % stride == 0);
	const size_t count = items.size() / stride;
	auto row = [&](size_t index) { return items.subspan(index * stride, stride); };
	std::vector<uint64_t> uids;
	for (size_t i = 0; i < count; ++i)
	{
		auto value = row(i);
		auto uid = number(value, 0, 8);
		auto type = value[8], state = value[9];
		auto owner = number(value, 16, 8), context = number(value, 24, 8),
		     root = number(value, 32, 8), parent = number(value, 40, 8),
		     revision = number(value, 48, 8);
		auto slot = stride == 96 ? number(value, 56, 2) : 0;
		need(uid && (uids.empty() || uids.back() < uid) && type >= 1 && type <= 12 &&
		     state >= 1 && state <= 3 && !nonzero(value.subspan(10, 6)) &&
		     (stride == 88 || !nonzero(value.subspan(58, 6))) &&
		     nonzero(value.subspan(stride - 32, 32)) && root && parent != uid);
		if (type == 7 || type == 8)
			need(!owner && !context);
		else
			need(owner && (type != 10 || !context) &&
			     (type != 11 || (context && context <= INT32_MAX)) &&
			     (type != 12 || (owner < UINT64_MAX && !context)));
		need(type != 12 || slot <= 43);
		need(!slot || ((type == 1 || type == 12) && !parent && state == 1 &&
			       (type != 12 || root == uid)));
		need(state == 2 ? type == 8 && revision : type != 8 && (parent || root == uid));
		uids.push_back(uid);
	}
	std::vector<uint8_t> colors(count);
	std::vector<size_t> path;
	for (size_t start = 0; start < count; ++start)
	{
		if (colors[start] == 2 || row(start)[9] == 2)
			continue;
		path.clear();
		size_t current = start;
		while (colors[current] != 2)
		{
			need(colors[current] != 1);
			colors[current] = 1;
			path.push_back(current);
			auto value = row(current);
			auto parent = number(value, 40, 8);
			if (!parent)
				break;
			auto found = std::lower_bound(uids.begin(), uids.end(), parent);
			need(found != uids.end() && *found == parent);
			auto ancestor = row(static_cast<size_t>(found - uids.begin()));
			need(ancestor[9] != 2 && ancestor[8] == value[8] &&
			     same(ancestor.subspan(16, 24), value.subspan(16, 24)));
			current = static_cast<size_t>(found - uids.begin());
		}
		for (auto index : path)
			colors[index] = 2;
	}
}
struct root
{
	identity operation;
	uint64_t revision;
	digest witness;
};
struct reservation
{
	uint64_t kind, id;
	identity operation;
};
struct book_control
{
	std::array<uint8_t, 40> opening;
	uint64_t revision;
	identity terminal;
	std::array<digest, 16> checksums;
};
inline bool less(const reservation &first, const reservation &second)
{
	return std::tie(first.kind, first.id) < std::tie(second.kind, second.id);
}
class checker
{
	std::filesystem::path directory;
	// At most 256 * 4096 roots: 56 bytes of descriptor fields per root,
	// plus bounded vector capacity/metadata. Rebuild at most 16 * 65536
	// 32-byte reservations for one book at a time. These are structural
	// bounds, not measured workload, memory or latency qualification.
	std::map<identity, std::vector<root>> books;
	size_t roots = 0;

    public:
	explicit checker(const std::filesystem::path &path)
		: directory(path / "economic-evidence")
	{
	}
	book_control control(const identity &lineage, const epoch_marker &marker,
			     digest *body_digest = nullptr) const
	{
		need(marker.initialization != baseline_initialization::never_initialized);
		auto encoded = frame(directory, prefix(lineage, marker.epoch) + "head.ebc",
				     "DUREBC1", 656);
		reader in{ encoded };
		need(in.fixed<16>() == lineage && in.fixed<16>() == marker.epoch);
		book_control result;
		result.opening = in.fixed<40>();
		need(std::get<0>(account(result.opening, lineage)) == 9);
		result.revision = in.number(8);
		result.terminal = in.fixed<16>();
		need(nonzero(result.terminal) && result.revision <= buckets * bucket_capacity);
		if (marker.initialization == baseline_initialization::initialized)
			need(same(result.opening, marker.opening) &&
			     (result.revision != 0 ||
			      result.terminal == marker.initializing_operation));
		for (auto &checksum : result.checksums)
		{
			checksum = in.fixed<32>();
			need(nonzero(checksum));
		}
		in.done();
		if (body_digest)
			*body_digest = hash(encoded);
		return result;
	}
	std::vector<reservation> reservations(const identity &lineage, const identity &epoch,
					      size_t slot, const digest &checksum) const
	{
		need(slot < 16);
		auto body = frame(directory, prefix(lineage, epoch) + hex_digits[slot] + ".ebi",
				  "DUREBI1", reservation_limit, checksum);
		reader in{ body };
		need(in.fixed<16>() == lineage && in.fixed<16>() == epoch && in.number(4) == slot);
		const auto count = in.number(4);
		need(count <= reservation_capacity);
		std::vector<reservation> result;
		for (size_t i = 0; i < count; ++i)
		{
			reservation value{ in.number(8), in.number(8), in.fixed<16>() };
			need((value.kind == 1 || value.kind == 2) && value.id &&
			     value.id % 16 == slot && nonzero(value.operation) &&
			     (result.empty() || less(result.back(), value)));
			result.push_back(value);
		}
		in.done();
		return result;
	}
	bytes witness(const identity &lineage, const identity &epoch, const root &entry) const
	{
		return file_bytes(directory, prefix(lineage, epoch) + hex(entry.operation) + ".eab",
				  witness_limit, entry.witness);
	}
	const std::vector<root> &observed(const identity &epoch) const
	{
		need(books.size() == 1 && books.contains(epoch));
		return books.at(epoch);
	}
	const std::map<identity, std::vector<root>> &observed_books() const { return books; }
	std::array<std::vector<reservation>, 16>
	expected_reservations(const identity &lineage, const identity &epoch,
			      std::span<const uint8_t> opening,
			      std::span<const root> retained) const
	{
		std::array<std::vector<reservation>, 16> result;
		for (const auto &entry : retained)
		{
			auto encoded = witness(lineage, epoch, entry);
			const auto stride = item_bytes(encoded);
			need(same(std::span<const uint8_t>(encoded).subspan(80, 40), opening));
			auto holdings = number(encoded, 184, 4), items = number(encoded, 188, 4);
			auto reserve = [&](uint64_t kind, uint64_t id)
			{
				auto &slot = result[id % 16];
				need(slot.size() < reservation_capacity);
				slot.push_back({ kind, id, entry.operation });
			};
			for (size_t n = 0; n < holdings; ++n)
				reserve(1, number(encoded, 192 + n * 112 + 20, 8));
			for (size_t n = 0; n < items; ++n)
				reserve(2, number(encoded, 192 + holdings * 112 + n * stride, 8));
		}
		return result;
	}
	bytes observe(const identity &lineage, const identity &epoch, const identity &operation,
		      std::span<const uint8_t> intent, std::span<const uint8_t> payload,
		      std::span<const uint8_t> plan, uint64_t revision)
	{
		need(revision && payload.size() == 48 && intent.size() == 256 &&
		     number(intent, 16, 4) == 1 && number(intent, 20, 4) == 1);
		digest checksum;
		std::copy_n(payload.begin() + 16, 32, checksum.begin());
		auto encoded = file_bytes(directory,
					  prefix(lineage, epoch) + hex(operation) + ".eab",
					  witness_limit, checksum);
		std::span<const uint8_t> witness = encoded;
		const auto stride = item_bytes(witness);
		need(witness.size() == number(payload, 8, 4) &&
		     same(witness.subspan(16, 16), lineage) &&
		     same(witness.subspan(32, 16), epoch) && nonzero(witness.subspan(48, 16)) &&
		     number(witness, 64, 8) == number(intent, 96, 8) &&
		     same(witness.subspan(48, 16), intent.subspan(116, 16)) &&
		     same(witness.subspan(72, 8), intent.subspan(148, 8)) &&
		     nonzero(witness.subspan(120, 32)) && nonzero(witness.subspan(152, 32)));
		need(std::get<0>(account(witness.subspan(80, 40), lineage)) == 9);
		bytes derived(witness.begin() + 48, witness.begin() + 64);
		put(derived, 0x42415345, 4);
		append(derived, witness.subspan(72, 8));
		need(same(std::span<const uint8_t>(hash(derived)).first(16), operation));
		auto holdings = number(witness, 184, 4), items = number(witness, 188, 4);
		bytes body, postings;
		struct equity
		{
			std::array<uint64_t, 4> amounts;
			uint64_t value;
		};
		std::vector<equity> opposites;
		std::tuple<uint64_t, uint64_t, uint64_t> previous;
		std::set<uint64_t> lifetimes;
		constexpr std::array<uint64_t, 4> units = { 1, 10, 100, 1000 };
		for (size_t i = 0; i < holdings; ++i)
		{
			auto value = witness.subspan(192 + i * 112, 112);
			auto key = account(value.first(40), lineage);
			need(std::get<0>(key) <= 6 && (!i || previous < key) &&
			     lifetimes.insert(std::get<1>(key)).second &&
			     nonzero(value.subspan(80, 32)));
			previous = key;
			equity money = {};
			__int128_t total = 0;
			for (size_t part = 0; part < 4; ++part)
			{
				money.amounts[part] = number(value, 40 + part * 8, 8);
				need(money.amounts[part] <= INT64_MAX);
				total += __int128_t(money.amounts[part]) * units[part];
			}
			need(total <= INT64_MAX);
			money.value = static_cast<uint64_t>(total);
			append(body, value.first(40));
			body.insert(body.end(), 32, 0);
			append(body, value.subspan(40, 32));
			put(body, 0, 8);
			put(body, 1, 8);
			if (money.value)
			{
				put(postings, opposites.size(), 4);
				put(postings, i, 2);
				put(postings, 0, 2);
				append(postings, value.subspan(40, 32));
				put(postings, money.value, 8);
				opposites.push_back(money);
			}
		}
		if (!opposites.empty())
		{
			append(body, witness.subspan(80, 40));
			body.insert(body.end(), 80, 0);
			for (size_t i = 0; i < opposites.size(); ++i)
			{
				put(postings, opposites.size() + i, 4);
				put(postings, holdings, 2);
				put(postings, 0, 2);
				for (auto amount : opposites[i].amounts)
					put(postings, uint64_t{ 0 } - amount, 8);
				put(postings, uint64_t{ 0 } - opposites[i].value, 8);
			}
		}
		append(body, postings);
		auto positions = witness.subspan(192 + holdings * 112);
		forest(positions, stride);
		bytes snapshots;
		for (size_t i = 0; i < items; ++i)
		{
			append(snapshots, positions.subspan(i * stride, stride == 96 ? 64 : 56));
			if (stride == 88)
				snapshots.insert(snapshots.end(), 8, 0);
		}
		append(body, snapshots);
		append(body, snapshots);
		const std::array<uint64_t, 6> counts = {
			holdings + !opposites.empty(), 2 * opposites.size(), 0, items, items, 0
		};
		for (size_t i = 0; i < counts.size(); ++i)
			need(number(plan, 216 + 4 * i, 4) == counts[i]);
		need(same(plan.subspan(256), body));
		need(roots++ < buckets * bucket_capacity);
		books[epoch].push_back({ operation, revision, checksum });
		return encoded;
	}
	void finish(const identity &lineage, const std::vector<epoch_marker> &catalog)
	{
		std::map<identity, epoch_marker> markers;
		for (const auto &entry : catalog)
		{
			markers.emplace(entry.epoch, entry);
			if (entry.initialization == baseline_initialization::initialized)
				books.try_emplace(entry.epoch);
		}
		// Legacy files remain structurally readable; unknown initialization is
		// reported by the caller and cannot earn complete-book-loss qualification.
		for (const auto &file : std::filesystem::directory_iterator(directory))
		{
			audit_directory_entry();
			auto name = file.path().filename().string();
			if (!name.starts_with("baseline"))
				continue;
			need(name.size() >= 80 && name.starts_with("baseline-") &&
			     name[41] == '-' && name[74] == '-');
			need(unhex(name.substr(9, 32)) == lineage);
			auto epoch = unhex(name.substr(42, 32));
			need(markers.contains(epoch));
			books.try_emplace(epoch);
		}
		for (auto &[epoch, retained] : books)
		{
			const auto &marker = markers.at(epoch);
			need(marker.initialization != baseline_initialization::never_initialized);
			const auto book = control(lineage, marker);
			need(book.revision == retained.size());
			std::sort(retained.begin(), retained.end(),
				  [](const auto &a, const auto &b)
				  { return a.revision < b.revision; });
			for (size_t i = 0; i < retained.size(); ++i)
			{
				const auto &entry = retained[i];
				need(entry.revision == i + 1 && (i + 1 != retained.size() ||
								 entry.operation == book.terminal));
			}
			auto reservations =
				expected_reservations(lineage, epoch, book.opening, retained);
			for (size_t slot = 0; slot < 16; ++slot)
			{
				auto &expected = reservations[slot];
				std::sort(expected.begin(), expected.end(), less);
				const auto actual = this->reservations(lineage, epoch, slot,
								       book.checksums[slot]);
				need(actual.size() == expected.size());
				for (size_t i = 0; i < expected.size(); ++i)
				{
					need((!i || less(expected[i - 1], expected[i])) &&
					     actual[i].kind == expected[i].kind &&
					     actual[i].id == expected[i].id &&
					     actual[i].operation == expected[i].operation);
				}
			}
			std::sort(retained.begin(), retained.end(),
				  [](const auto &a, const auto &b)
				  { return a.operation < b.operation; });
		}
		// Namespace closure uses bounded root descriptors, not a second map of
		// every long witness filename or all retained payloads.
		for (const auto &file : std::filesystem::directory_iterator(directory))
		{
			audit_directory_entry();
			auto name = file.path().filename().string();
			if (!name.starts_with("baseline"))
				continue;
			const auto &retained = books.at(unhex(name.substr(42, 32)));
			auto suffix = name.substr(75);
			if (suffix == "head.ebc")
				continue;
			if (suffix.size() == 5 && suffix.ends_with(".ebi") &&
			    std::strchr(hex_digits, suffix[0]) && suffix[0])
				continue;
			need(suffix.size() == 36 && suffix.ends_with(".eab"));
			auto operation = unhex(suffix.substr(0, 32));
			auto found = std::lower_bound(retained.begin(), retained.end(), operation,
						      [](const auto &entry, const auto &id)
						      { return entry.operation < id; });
			need(found != retained.end() && found->operation == operation);
		}
	}
};
} // namespace restore_economic_baseline
#endif
