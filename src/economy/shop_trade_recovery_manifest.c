#include "economy/shop_trade_recovery_manifest.h"
#include "player/player_snapshot_codec.h"

#include <algorithm>
#include <new>
#include <type_traits>
#include <utility>

#include <openssl/evp.h>

namespace
{
constexpr std::array<uint8_t, 4> empty_item_list{};
constexpr char digest_domain[] = "DURIS-SHOP-RECOVERY-FOREST-V8";

bool valid_role(shop_trade_recovery_forest_role role) noexcept
{
	return role >= shop_trade_recovery_forest_role::player_before &&
	       role <= shop_trade_recovery_forest_role::live_target_after;
}

template <typename T> void append_le(std::vector<uint8_t> *out, T value)
{
	using unsigned_type = std::make_unsigned_t<T>;
	const auto encoded = static_cast<unsigned_type>(value);
	for (size_t index = 0; index < sizeof(T); ++index)
		out->push_back(static_cast<uint8_t>(encoded >> (index * 8)));
}

template <typename T> bool read_le(const uint8_t **cursor, const uint8_t *end, T *out)
{
	if (static_cast<size_t>(end - *cursor) < sizeof(T))
		return false;
	using unsigned_type = std::make_unsigned_t<T>;
	unsigned_type value = 0;
	for (size_t index = 0; index < sizeof(T); ++index)
		value |= static_cast<unsigned_type>((*cursor)[index]) << (index * 8);
	*cursor += sizeof(T);
	*out = static_cast<T>(value);
	return true;
}

bool digest(std::span<const uint8_t> bytes, shop_trade_recovery_forest_role role, uint32_t count,
	    std::array<uint8_t, 32> *out) noexcept
{
	if (!out || !valid_role(role) || bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return false;
	std::array<uint8_t, 11> header{};
	header[0] = static_cast<uint8_t>(SHOP_TRADE_RECOVERY_MANIFEST_VERSION);
	header[1] = static_cast<uint8_t>(SHOP_TRADE_RECOVERY_MANIFEST_VERSION >> 8);
	header[2] = static_cast<uint8_t>(role);
	for (size_t index = 0; index < sizeof(uint32_t); ++index)
	{
		header[3 + index] = static_cast<uint8_t>(count >> (index * 8));
		header[7 + index] = static_cast<uint8_t>(bytes.size() >> (index * 8));
	}
	auto *context = EVP_MD_CTX_new();
	if (!context)
		return false;
	std::array<uint8_t, 32> candidate{};
	unsigned int size = 0;
	const bool success = EVP_DigestInit_ex(context, EVP_sha256(), nullptr) == 1 &&
			     EVP_DigestUpdate(context, digest_domain, sizeof(digest_domain)) == 1 &&
			     EVP_DigestUpdate(context, header.data(), header.size()) == 1 &&
			     EVP_DigestUpdate(context, bytes.data(), bytes.size()) == 1 &&
			     EVP_DigestFinal_ex(context, candidate.data(), &size) == 1 &&
			     size == candidate.size();
	EVP_MD_CTX_free(context);
	if (!success)
		return false;
	*out = candidate;
	return true;
}

std::array<const shop_trade_recovery_forest_binding *, SHOP_TRADE_RECOVERY_FOREST_COUNT>
bindings(const shop_trade_recovery_manifest &manifest) noexcept
{
	return {
		&manifest.player_before, &manifest.player_after,       &manifest.keeper_before,
		&manifest.keeper_after,	 &manifest.live_target_before, &manifest.live_target_after
	};
}

std::array<shop_trade_recovery_forest_binding *, SHOP_TRADE_RECOVERY_FOREST_COUNT>
bindings(shop_trade_recovery_manifest &manifest) noexcept
{
	return {
		&manifest.player_before, &manifest.player_after,       &manifest.keeper_before,
		&manifest.keeper_after,	 &manifest.live_target_before, &manifest.live_target_after
	};
}

bool binding_empty(const shop_trade_recovery_forest_binding &binding) noexcept
{
	return !binding.present && !binding.canonical_bytes && binding.ordered_item_uids.empty() &&
	       binding.canonical_digest == std::array<uint8_t, 32>{};
}

bool binding_valid(const shop_trade_recovery_forest_binding &binding,
		   shop_trade_recovery_forest_role role) noexcept
{
	if (!binding.present)
		return binding_empty(binding);
	const auto count = binding.ordered_item_uids.size();
	if (count > SHOP_TRADE_RECOVERY_MAX_UIDS ||
	    binding.canonical_bytes < empty_item_list.size() ||
	    binding.canonical_bytes > PLAYER_SNAPSHOT_MAX_BYTES)
		return false;
	if (!count)
	{
		std::array<uint8_t, 32> expected{};
		return binding.canonical_bytes == empty_item_list.size() &&
		       digest(empty_item_list, role, 0, &expected) &&
		       binding.canonical_digest == expected;
	}
	if (binding.canonical_bytes == empty_item_list.size() ||
	    binding.canonical_digest == std::array<uint8_t, 32>{})
		return false;
	std::array<uint64_t, SHOP_TRADE_RECOVERY_MAX_UIDS> ordered{};
	std::copy(binding.ordered_item_uids.begin(), binding.ordered_item_uids.end(),
		  ordered.begin());
	std::sort(ordered.begin(), ordered.begin() + count);
	return ordered[0] && ordered[count - 1] != UINT64_MAX &&
	       std::adjacent_find(ordered.begin(), ordered.begin() + count) ==
		       ordered.begin() + count;
}

bool contiguous_dfs(const std::vector<player_item_snapshot> &items) noexcept
{
	std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH> path{};
	size_t depth = 0;
	for (size_t index = 0; index < items.size(); ++index)
	{
		if (items[index].parent_index == PLAYER_SNAPSHOT_NO_PARENT)
		{
			path[0] = static_cast<int32_t>(index);
			depth = 1;
			continue;
		}
		while (depth && path[depth - 1] != items[index].parent_index)
			--depth;
		if (!depth || depth == path.size())
			return false;
		path[depth++] = static_cast<int32_t>(index);
	}
	return true;
}
} // namespace

bool shop_trade_recovery_forest_freeze(std::span<const uint8_t> canonical_bytes,
				       shop_trade_recovery_forest_role role,
				       shop_trade_recovery_forest_binding *out)
{
	if (!out || !valid_role(role) || canonical_bytes.empty() ||
	    canonical_bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return false;
	const uint8_t *cursor = canonical_bytes.data();
	const uint8_t *end = cursor + canonical_bytes.size();
	uint32_t count = 0;
	if (!read_le(&cursor, end, &count) || count > SHOP_TRADE_RECOVERY_MAX_UIDS)
		return false;
	try
	{
		std::vector<player_item_snapshot> items;
		std::vector<uint8_t> canonical;
		if (player_item_snapshot_list_decode(canonical_bytes.data(), canonical_bytes.size(),
						     &items) != player_snapshot_codec_result::ok ||
		    items.size() > SHOP_TRADE_RECOVERY_MAX_UIDS || !contiguous_dfs(items) ||
		    player_item_snapshot_list_encode(items, &canonical) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() != canonical_bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), canonical_bytes.begin()))
			return false;
		shop_trade_recovery_forest_binding candidate;
		candidate.present = true;
		candidate.canonical_bytes = static_cast<uint32_t>(canonical.size());
		candidate.ordered_item_uids.reserve(items.size());
		for (const auto &item : items)
			candidate.ordered_item_uids.push_back(item.object_uid);
		if (!digest(canonical, role, static_cast<uint32_t>(items.size()),
			    &candidate.canonical_digest) ||
		    !binding_valid(candidate, role))
			return false;
		*out = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_recovery_forest_verify(std::span<const uint8_t> canonical_bytes,
				       shop_trade_recovery_forest_role role,
				       const shop_trade_recovery_forest_binding &binding)
{
	shop_trade_recovery_forest_binding expected;
	return binding.present &&
	       shop_trade_recovery_forest_freeze(canonical_bytes, role, &expected) &&
	       expected == binding;
}

bool shop_trade_recovery_manifest_is_empty(const shop_trade_recovery_manifest &manifest) noexcept
{
	const auto values = bindings(manifest);
	return std::all_of(values.begin(), values.end(),
			   [](const auto *binding) { return binding_empty(*binding); });
}

bool shop_trade_recovery_manifest_shape_valid(const shop_trade_recovery_manifest &manifest) noexcept
{
	const auto values = bindings(manifest);
	for (size_t index = 0; index < values.size(); ++index)
		if ((index < 4 && !values[index]->present) ||
		    !binding_valid(*values[index],
				   static_cast<shop_trade_recovery_forest_role>(index + 1)))
			return false;
	return manifest.live_target_before.present == manifest.live_target_after.present;
}

bool shop_trade_recovery_manifest_encode(const shop_trade_recovery_manifest &manifest,
					 std::vector<uint8_t> *out)
{
	if (!out || !shop_trade_recovery_manifest_shape_valid(manifest))
		return false;
	try
	{
		std::vector<uint8_t> candidate{ 'S', 'R', 'M', '8' };
		append_le<uint16_t>(&candidate, SHOP_TRADE_RECOVERY_MANIFEST_VERSION);
		append_le<uint16_t>(&candidate, 0);
		const auto values = bindings(manifest);
		for (size_t index = 0; index < values.size(); ++index)
		{
			const auto &binding = *values[index];
			append_le<uint8_t>(&candidate, static_cast<uint8_t>(index + 1));
			append_le<uint8_t>(&candidate, binding.present ? 1 : 0);
			append_le<uint16_t>(&candidate, static_cast<uint16_t>(
								binding.ordered_item_uids.size()));
			append_le<uint32_t>(&candidate, binding.canonical_bytes);
			candidate.insert(candidate.end(), binding.canonical_digest.begin(),
					 binding.canonical_digest.end());
			for (const auto uid : binding.ordered_item_uids)
				append_le<uint64_t>(&candidate, uid);
		}
		if (candidate.size() > SHOP_TRADE_RECOVERY_MANIFEST_MAX_BYTES)
			return false;
		*out = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_recovery_manifest_decode(std::span<const uint8_t> bytes,
					 shop_trade_recovery_manifest *out)
{
	if (!out || bytes.size() < SHOP_TRADE_RECOVERY_MANIFEST_MIN_BYTES ||
	    bytes.size() > SHOP_TRADE_RECOVERY_MANIFEST_MAX_BYTES)
		return false;
	try
	{
		const uint8_t *cursor = bytes.data();
		const uint8_t *end = cursor + bytes.size();
		const std::array<uint8_t, 4> magic{ 'S', 'R', 'M', '8' };
		if (!std::equal(magic.begin(), magic.end(), cursor))
			return false;
		cursor += magic.size();
		uint16_t version = 0, padding = 0;
		if (!read_le(&cursor, end, &version) ||
		    version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
		    !read_le(&cursor, end, &padding) || padding)
			return false;
		shop_trade_recovery_manifest candidate;
		const auto values = bindings(candidate);
		for (size_t index = 0; index < values.size(); ++index)
		{
			uint8_t role = 0, present = 0;
			uint16_t count = 0;
			auto &binding = *values[index];
			if (!read_le(&cursor, end, &role) ||
			    role != static_cast<uint8_t>(index + 1) ||
			    !read_le(&cursor, end, &present) || present > 1 ||
			    !read_le(&cursor, end, &count) ||
			    count > SHOP_TRADE_RECOVERY_MAX_UIDS ||
			    !read_le(&cursor, end, &binding.canonical_bytes) ||
			    static_cast<size_t>(end - cursor) <
				    binding.canonical_digest.size() + count * sizeof(uint64_t))
				return false;
			binding.present = present != 0;
			std::copy(cursor, cursor + binding.canonical_digest.size(),
				  binding.canonical_digest.begin());
			cursor += binding.canonical_digest.size();
			binding.ordered_item_uids.resize(count);
			for (auto &uid : binding.ordered_item_uids)
				if (!read_le(&cursor, end, &uid))
					return false;
		}
		if (cursor != end || !shop_trade_recovery_manifest_shape_valid(candidate))
			return false;
		std::vector<uint8_t> canonical;
		if (!shop_trade_recovery_manifest_encode(candidate, &canonical) ||
		    canonical.size() != bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
			return false;
		*out = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
