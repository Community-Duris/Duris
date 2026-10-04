// Independent physical retained-operation scan. No mutation/recovery codecs.
#ifndef DURIS_QUALIFY_FLATFILE_ECONOMIC_RECORDS_H
#define DURIS_QUALIFY_FLATFILE_ECONOMIC_RECORDS_H
#include "qualify_flatfile_economic_authority.h"
#include <tuple>

namespace restore_economic_records
{
using namespace restore_economic_authority;
constexpr size_t index_limit = 80 + 4096 * 64, segment_limit = 8 * 1024 * 1024;
constexpr size_t command_limit = 512 * 1024, plan_limit = 4 * 1024 * 1024;
constexpr size_t record_limit = 48 + 26 + command_limit + plan_limit + 4096;
constexpr uint64_t bucket_limit = uint64_t{ 256 } << 20;
constexpr size_t segment_count_limit = bucket_limit / (segment_limit - record_limit - 80) + 1;
inline bool same(std::span<const uint8_t> first, std::span<const uint8_t> second)
{
	return first.size() == second.size() &&
	       std::equal(first.begin(), first.end(), second.begin());
}
inline uint64_t number(std::span<const uint8_t> value, size_t offset, size_t width)
{
	reader in{ value };
	(void)in.take(offset);
	return in.number(width);
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
struct entry
{
	identity operation;
	digest checksum;
	uint64_t segment, offset, size;
};
class checker
{
	std::filesystem::path root, directory;
	identity lineage = {};
	std::set<identity> epochs;
	std::set<std::string> expected_files;

	void record(std::span<const uint8_t> encoded, const identity &operation) const
	{
		need(encoded.size() >= 48 && encoded.size() <= record_limit);
		reader in{ unwrap(encoded, "DURECR2") };
		auto command_size = in.number(4), plan_size = in.number(4),
		     result_size = in.number(4);
		auto result_code = in.number(4);
		(void)in.number(
			8); // Full unsigned durable revision, with no inferred native counter.
		auto failure_stage = in.number(2);
		need(command_size <= command_limit && plan_size <= plan_limit &&
		     result_size <= 4096 && failure_stage <= 0x3fff &&
		     (result_code || failure_stage == 0) &&
		     (result_code ? plan_size == 0 : plan_size >= 256));
		auto command = in.take(command_size), plan = in.take(plan_size);
		(void)in.take(result_size);
		in.done();
		reader cmd{ command };
		need(same(cmd.take(4), { reinterpret_cast<const uint8_t *>("CCM1"), 4 }) &&
		     cmd.number(4) == 2 && cmd.fixed<16>() == operation);
		auto type = cmd.number(2), payload_version = cmd.number(2), source = cmd.number(2);
		auto deadline = cmd.number(1), publication = cmd.number(1),
		     accepted = cmd.number(8);
		auto keys = cmd.number(4), revisions = cmd.number(4), payload_size = cmd.number(4);
		need(type >= 1 && type <= 20 && payload_version && source >= 1 && source <= 6 &&
		     deadline >= 1 && deadline <= 4 && publication <= 1 &&
		     (!publication || type == 3 || type == 5 || type == 17) && accepted &&
		     keys > 0 && keys <= 3003 && revisions <= 3003 && payload_size <= 384 * 1024);
		using key = std::pair<uint64_t, uint64_t>;
		std::vector<key> identities;
		auto read_key = [&]
		{
			auto kind = cmd.number(1);
			need(!nonzero(cmd.take(7)));
			auto id = cmd.number(8);
			need(kind >= 1 && kind <= 14 && id);
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
	}
	void bucket(size_t bucket)
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
		uint64_t total = 0, last_segment = 0;
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
			last_segment = std::max(last_segment, row.segment);
			entries.push_back(row);
		}
		in.done();
		need(total == stored_total);
		if (entries.empty())
			return;
		std::sort(entries.begin(), entries.end(),
			  [](const auto &first, const auto &second) {
				  return std::tie(first.segment, first.offset) <
					 std::tie(second.segment, second.offset);
			  });
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

    public:
	explicit checker(const std::filesystem::path &path)
		: root(path)
		, directory(path / "economic-evidence")
	{
	}
	void run()
	{
		// Pure metadata validation, distinct from candidate journal recovery.
		restore_economic_authority::checker(root).run();
		if (!std::filesystem::exists(directory) || std::filesystem::is_empty(directory))
			return;
		auto control = frame(directory, "authority.eal", "DURECA1");
		need(control.size() == 16552);
		std::copy_n(control.begin(), 16, lineage.begin());
		auto catalog = frame(directory, "epochs.eae", "DURECE1");
		reader in{ catalog };
		need(in.fixed<16>() == lineage);
		auto count = in.number(4);
		need(count <= 4096 && in.number(4) == 0);
		for (size_t i = 0; i < count; ++i)
		{
			epochs.insert(in.fixed<16>());
			(void)in.take(80);
		}
		in.done();
		for (size_t index = 0; index < 256; ++index)
			if (control[16520 + index / 8] & (1u << (index % 8)))
				bucket(index);
		for (const auto &file : std::filesystem::directory_iterator(directory))
			if (file.path().filename().string().starts_with("bucket-"))
				need(expected_files.contains(file.path().filename().string()));
	}
};
} // namespace restore_economic_records
#endif
