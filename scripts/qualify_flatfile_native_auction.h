// Independent auction catalog decoding. No native provider or recovery calls.
#ifndef DURIS_QUALIFY_FLATFILE_NATIVE_AUCTION_H
#define DURIS_QUALIFY_FLATFILE_NATIVE_AUCTION_H

#include "qualify_flatfile_native_domains.h"
#include <bit>
#include <functional>

namespace restore_native_auction
{
using namespace restore_native_domains;
constexpr size_t catalog_limit = 128 * 1024 * 1024, source_limit = 32 * 1024 * 1024;
struct money
{
	uint16_t kind;
	uint32_t id;
	int64_t amount;
	uint64_t revision;
};
struct catalog
{
	uint64_t revision = 0;
	size_t listings = 0, operations = 0;
	std::vector<money> holdings;
};
struct sources
{
	uint64_t revision = 0;
	size_t rows = 0;
};
struct source_row
{
	identity operation = {}, lineage = {}, consumed = {};
	uint16_t slot = 0;
	uint32_t pid = 0;
	uint64_t mapping = 0, amount = 0;
};
inline int64_t signed_number(reader &in)
{
	return std::bit_cast<int64_t>(in.number(8));
}
inline void text(reader &in, size_t maximum, bool canonical = false)
{
	const auto length = in.number(4);
	need(length <= maximum && (!canonical || length));
	for (auto c : in.take(length))
		need(c && (!canonical || (c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') ||
			   c == '_' || c == '-'));
}
inline void receipt(std::span<const uint8_t> body)
{
	reader in{ body };
	const auto action = in.number(1);
	need(action >= 1 && action <= 6);
	(void)in.take(141); // Event, identities, signed money and revision clocks.
	const auto items = in.number(2);
	need(items <= 9);
	(void)in.take(items * 16);
	need(signed_number(in) >= 0);
	// The original result codec permits arbitrary trailing padding.
}
inline catalog decode_catalog(std::span<const uint8_t> encoded)
{
	const auto file = unwrap(encoded, "DURAUCT", catalog_limit, 2);
	reader in{ file.body };
	catalog result;
	result.revision = file.revision;
	result.listings = in.number(4);
	need(result.listings <= 262144);
	uint64_t previous = 0;
	for (size_t index = 0; index < result.listings; ++index)
	{
		const auto id = in.number(4), seller = in.number(4), winner = in.number(4),
			   status = in.number(4);
		const auto price = signed_number(in);
		(void)signed_number(in); // Buy price is not a current holding.
		const auto revision = in.number(8);
		(void)in.number(8);
		need(id > previous && seller && revision && status >= 1 && status <= 3);
		previous = id;
		text(in, 50, true);
		for (auto maximum : { 32, 32, 255, 1024, 8192 })
			text(in, maximum);
		const auto blob = in.number(4);
		// Native raw decoding rejects the null buffer of an empty object blob.
		need(blob && blob <= 32768);
		(void)in.take(blob);
		const auto items = in.number(2);
		need(items && items <= 9);
		std::set<uint64_t> uids;
		for (size_t item = 0; item < items; ++item)
		{
			const auto uid = in.number(8), item_revision = in.number(8),
				   vnum = in.number(4);
			(void)in.number(4);
			need(uid && item_revision && vnum <= INT32_MAX && in.number(1) <= 1 &&
			     uids.insert(uid).second);
		}
		if (status == 1)
			result.holdings.push_back(
				{ 4, static_cast<uint32_t>(id), winner ? price : 0, revision });
	}
	const auto pickups = in.number(4);
	need(pickups <= 262144);
	previous = 0;
	for (size_t index = 0; index < pickups; ++index)
	{
		const auto pid = in.number(4);
		const auto amount = signed_number(in);
		const auto revision = in.number(8);
		need(pid > previous && amount >= 0 && revision);
		previous = pid;
		result.holdings.push_back({ 5, static_cast<uint32_t>(pid), amount, revision });
	}
	result.operations = in.number(4);
	need(result.operations <= 1048576);
	std::set<identity> operations;
	for (size_t index = 0; index < result.operations; ++index)
	{
		const auto operation = in.fixed<16>();
		need(nonzero(operation) && operations.insert(operation).second);
		(void)in.take(36); // Digest and native result code.
		receipt(in.take(320));
		if (file.version == 2)
			need(in.number(1) <= 1);
	}
	in.done();
	return result;
}
inline sources decode_sources(std::span<const uint8_t> encoded,
			      const std::function<void(const source_row &)> &observe = {})
{
	const auto file = unwrap(encoded, "DURAUSR", source_limit, 1);
	reader in{ file.body };
	sources result{ file.revision, static_cast<size_t>(in.number(4)) };
	need(result.rows <= 262144 && file.body.size() == 4 + result.rows * 70);
	std::pair<identity, uint16_t> previous = {};
	for (size_t index = 0; index < result.rows; ++index)
	{
		const auto operation = in.fixed<16>();
		const auto slot = in.number(2);
		const auto lineage = in.fixed<16>();
		const auto pid = in.number(4), mapping = in.number(8), amount = in.number(8);
		const auto consumed = in.fixed<16>();
		const auto key = std::pair{ operation, static_cast<uint16_t>(slot) };
		need(nonzero(operation) && slot && nonzero(lineage) && pid && mapping && amount &&
		     amount <= INT32_MAX && consumed != operation && key > previous);
		previous = key;
		if (observe)
			observe({ operation, lineage, consumed, static_cast<uint16_t>(slot),
				  static_cast<uint32_t>(pid), mapping, amount });
	}
	in.done();
	return result;
}
} // namespace restore_native_auction
#endif
