// Independent native wallet/bank decoding. No repository or recovery calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_DOMAINS_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_DOMAINS_H

#include "qualify_flatfile_economic_authority.h"

namespace restore_native_domains
{
using namespace restore_economic_authority;
constexpr size_t domain_limit = 64 * 1024;
struct holding
{
	uint64_t native_id = 0, revision = 0;
	std::string account;
	int8_t racewar = 0;
	std::array<uint64_t, 4> balance = {};
};
struct envelope
{
	std::span<const uint8_t> body;
	uint64_t revision;
	uint32_t version;
};
inline envelope unwrap(std::span<const uint8_t> encoded, const char *magic,
		       size_t limit = domain_limit, uint32_t maximum_version = 4)
{
	need(encoded.size() >= 56 && encoded.size() <= limit &&
	     same(encoded.first(8), { reinterpret_cast<const uint8_t *>(magic), 8 }));
	const auto version = number(encoded, 8, 4), size = number(encoded, 12, 4),
		   revision = number(encoded, 16, 8);
	need(version >= 1 && version <= maximum_version && revision && size == encoded.size() - 56);
	auto body = encoded.subspan(56);
	need(same(encoded.subspan(24, 32), hash(body)));
	return { body, revision, static_cast<uint32_t>(version) };
}
inline std::string name(reader &in, bool canonical)
{
	const auto length = in.number(4);
	need(length && length <= 50);
	auto value = in.take(length);
	std::string result;
	for (auto c : value)
	{
		if (!canonical && c >= 'A' && c <= 'Z')
			c = static_cast<uint8_t>(c - 'A' + 'a');
		need((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-');
		result += static_cast<char>(c);
	}
	return result;
}
inline holding bank(std::span<const uint8_t> encoded, const std::string &account, int8_t racewar)
{
	auto file = unwrap(encoded, "DURBANK");
	reader in{ file.body };
	holding result;
	result.account = name(in, false);
	result.racewar = static_cast<int8_t>(in.number(1));
	for (auto &balance : result.balance)
		balance = in.number(8);
	in.done();
	need(result.account == account && result.racewar == racewar);
	result.revision = file.revision;
	return result;
}
inline holding wallet(std::span<const uint8_t> encoded, uint32_t pid)
{
	need(pid && pid <= INT32_MAX);
	auto file = unwrap(encoded, "DURPDOM");
	reader in{ file.body };
	holding result;
	result.native_id = in.number(4);
	need(result.native_id == pid);
	result.account = name(in, true);
	result.racewar = static_cast<int8_t>(in.number(1));
	result.revision = in.number(8);
	const auto epic_revision = in.number(8), frag_revision = in.number(8);
	for (auto &balance : result.balance)
		balance = in.number(8);
	(void)in.take(24); // Epics, frags and old frags are signed native counters.
	const auto recent_count = in.number(4);
	need(recent_count <= 20);
	uint64_t previous = INT64_MAX;
	for (size_t i = 0; i < recent_count; ++i)
	{
		const auto value = in.number(8);
		need(value && value <= previous);
		previous = value;
	}
	const auto zone_count = in.number(4);
	need(zone_count <= 1024);
	previous = 0;
	for (size_t i = 0; i < zone_count; ++i)
	{
		const auto value = in.number(4);
		need(value > previous && value <= INT32_MAX);
		previous = value;
	}
	uint64_t stat_revision = 0;
	if (file.version >= 3)
	{
		stat_revision = in.number(8);
		for (size_t i = 0; i < 10; ++i)
		{
			const auto value = in.number(2);
			need(value <= 100 && (stat_revision || !value));
		}
	}
	if (file.version >= 2)
	{
		const auto count = in.number(4);
		need(count <= 512);
		std::set<identity> operations;
		for (size_t i = 0; i < count; ++i)
		{
			const auto operation = in.fixed<16>();
			need(nonzero(operation) && operations.insert(operation).second);
			(void)in.take(32);
			const auto code = in.number(4), size = in.number(2);
			need(size <= 2048);
			(void)in.take(size);
			if (file.version >= 4)
			{
				const auto slot = in.number(4), amount = in.number(4);
				need((slot == 0) == (amount == 0) && slot <= 64 &&
				     (!slot || (!code && size == 80)));
			}
		}
	}
	in.done();
	need(file.revision == std::max({ result.revision, epic_revision, frag_revision,
					 stat_revision, uint64_t{ 1 } }));
	return result;
}
} // namespace restore_native_domains
#endif
