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

bool shop_trade_recovery_forest_shape_valid(const shop_trade_recovery_forest_binding &binding,
					    shop_trade_recovery_forest_role role) noexcept
{
	return valid_role(role) && binding_valid(binding, role);
}

bool shop_trade_recovery_forest_encode(const shop_trade_recovery_forest_binding &binding,
				       shop_trade_recovery_forest_role role,
				       std::vector<uint8_t> *out)
{
	if (!out || !shop_trade_recovery_forest_shape_valid(binding, role))
		return false;
	try
	{
		std::vector<uint8_t> candidate;
		candidate.reserve(SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
				  binding.ordered_item_uids.size() * sizeof(uint64_t));
		append_le<uint8_t>(&candidate, static_cast<uint8_t>(role));
		append_le<uint8_t>(&candidate, binding.present ? 1 : 0);
		append_le<uint16_t>(&candidate,
				    static_cast<uint16_t>(binding.ordered_item_uids.size()));
		append_le<uint32_t>(&candidate, binding.canonical_bytes);
		candidate.insert(candidate.end(), binding.canonical_digest.begin(),
				 binding.canonical_digest.end());
		for (const auto uid : binding.ordered_item_uids)
			append_le<uint64_t>(&candidate, uid);
		*out = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_recovery_forest_decode(std::span<const uint8_t> bytes,
				       shop_trade_recovery_forest_role expected_role,
				       shop_trade_recovery_forest_binding *out)
{
	if (!out || !valid_role(expected_role) ||
	    bytes.size() < SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES ||
	    bytes.size() > SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
				   SHOP_TRADE_RECOVERY_MAX_UIDS * sizeof(uint64_t))
		return false;
	try
	{
		const uint8_t *cursor = bytes.data(), *end = cursor + bytes.size();
		uint8_t role = 0, present = 0;
		uint16_t count = 0;
		shop_trade_recovery_forest_binding candidate;
		if (!read_le(&cursor, end, &role) || role != static_cast<uint8_t>(expected_role) ||
		    !read_le(&cursor, end, &present) || present > 1 ||
		    !read_le(&cursor, end, &count) || count > SHOP_TRADE_RECOVERY_MAX_UIDS ||
		    !read_le(&cursor, end, &candidate.canonical_bytes) ||
		    static_cast<size_t>(end - cursor) !=
			    candidate.canonical_digest.size() + count * sizeof(uint64_t))
			return false;
		candidate.present = present != 0;
		std::copy(cursor, cursor + candidate.canonical_digest.size(),
			  candidate.canonical_digest.begin());
		cursor += candidate.canonical_digest.size();
		candidate.ordered_item_uids.resize(count);
		for (auto &uid : candidate.ordered_item_uids)
			if (!read_le(&cursor, end, &uid))
				return false;
		if (cursor != end ||
		    !shop_trade_recovery_forest_shape_valid(candidate, expected_role))
			return false;
		*out = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
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

#include <limits>
#include <iterator>
#include <openssl/sha.h>

namespace
{
using shop_manifest_reserve_fn = bool (*)(size_t, void *) noexcept;
bool shop_manifest_storage_profile() noexcept
{
#if defined(__GLIBCXX__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG)
	return true;
#else
	return false;
#endif
}
bool shop_manifest_add(size_t &total, size_t bytes) noexcept
{
	if (bytes > std::numeric_limits<size_t>::max() - total)
		return false;
	total += bytes;
	return true;
}
bool shop_manifest_admit(size_t current, size_t request, shop_manifest_reserve_fn reserve,
			 void *context) noexcept
{
	return reserve && request <= std::numeric_limits<size_t>::max() - current &&
	       reserve(current + request, context);
}
bool shop_manifest_heap(const shop_trade_recovery_manifest &manifest, size_t *output) noexcept
{
	if (!output)
		return false;
	size_t total = 0;
	const auto values = bindings(manifest);
	for (const auto *binding : values)
	{
		if (binding->ordered_item_uids.capacity() >
			    std::numeric_limits<size_t>::max() / sizeof(uint64_t) ||
		    !shop_manifest_add(total,
				       binding->ordered_item_uids.capacity() * sizeof(uint64_t)))
			return false;
	}
	*output = total;
	return true;
}

// CRYPTO_SOURCE_CONSTANTS is replaced by the exact already-reviewed pinned
// OpenSSL3.0.13 fixed-context source profile during preparation.
constexpr size_t shop_manifest_sha256_assembly_frame =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t shop_manifest_sha256_c_small_frame =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *);
constexpr size_t shop_manifest_sha256_c_normal_frame =
	16 * sizeof(unsigned int) + 11 * sizeof(unsigned int) + sizeof(int) +
	sizeof(const uint8_t *) + sizeof(int) + sizeof(const unsigned int *);
constexpr size_t shop_manifest_sha256_c_frame =
	std::max(shop_manifest_sha256_c_small_frame, shop_manifest_sha256_c_normal_frame);
constexpr size_t shop_manifest_sha256_init_frame = sizeof(void *) + sizeof(int);
constexpr size_t shop_manifest_sha256_update_frame =
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(const uint8_t *) + sizeof(unsigned int) +
	sizeof(size_t) + sizeof(int);
constexpr size_t shop_manifest_sha256_final_frame = 2 * sizeof(void *) + sizeof(uint8_t *) +
						    sizeof(size_t) + sizeof(unsigned long) +
						    sizeof(unsigned int) + sizeof(int);
[[maybe_unused]] constexpr size_t shop_manifest_sha256_nested_frame =
	std::max(shop_manifest_sha256_assembly_frame, shop_manifest_sha256_c_frame) +
	std::max(shop_manifest_sha256_init_frame,
		 std::max(shop_manifest_sha256_update_frame, shop_manifest_sha256_final_frame));

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool shop_manifest_digest_bounded(std::span<const uint8_t> bytes,
				  shop_trade_recovery_forest_role role, uint32_t count,
				  std::array<uint8_t, 32> *out, shop_manifest_reserve_fn reserve,
				  void *context, size_t outer_live) noexcept
{
	if (!out || !valid_role(role) || bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return false;
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) &&     \
	defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&  \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&  \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned long) != 8 || sizeof(unsigned int) != 4)
		return false;
	struct workspace
	{
		SHA256_CTX digest;
		std::array<uint8_t, 11> header{};
		std::array<uint8_t, 32> candidate{};
		size_t index = 0;
	};
	constexpr size_t parameters = sizeof(std::span<const uint8_t>) +
				      sizeof(shop_trade_recovery_forest_role) + sizeof(uint32_t) +
				      3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
	if (!shop_manifest_admit(outer_live,
				 sizeof(workspace) + parameters + shop_manifest_sha256_nested_frame,
				 reserve, context))
		return false;
	workspace work;
	work.header[0] = static_cast<uint8_t>(SHOP_TRADE_RECOVERY_MANIFEST_VERSION);
	work.header[1] = static_cast<uint8_t>(SHOP_TRADE_RECOVERY_MANIFEST_VERSION >> 8);
	work.header[2] = static_cast<uint8_t>(role);
	for (work.index = 0; work.index < sizeof(uint32_t); ++work.index)
	{
		work.header[3 + work.index] = static_cast<uint8_t>(count >> (work.index * 8));
		work.header[7 + work.index] =
			static_cast<uint8_t>(bytes.size() >> (work.index * 8));
	}
	if (SHA256_Init(&work.digest) != 1 ||
	    SHA256_Update(&work.digest, digest_domain, sizeof(digest_domain)) != 1 ||
	    SHA256_Update(&work.digest, work.header.data(), work.header.size()) != 1 ||
	    SHA256_Update(&work.digest, bytes.data(), bytes.size()) != 1 ||
	    SHA256_Final(work.candidate.data(), &work.digest) != 1)
		return false;
	*out = work.candidate;
	return true;
#else
	(void)count;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#endif
}
#pragma GCC diagnostic pop

// Actual installed GCC13 sort carrier expressions. Introsort's only recursive
// arm decrements real depth_limit. This bounds source-declared carriers only;
// emitted native stack qualification remains a separate acceptance check.
constexpr size_t shop_manifest_sort_recursive_frame =
	3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(char);
constexpr size_t shop_manifest_sort_leaf_frames =
	// sort/__sort and median/partition/iter_swap carrier scopes.
	18 * sizeof(void *) + 7 * sizeof(char) + sizeof(uint64_t) +
	// final/insertion/unguarded-insertion/linear-insert and move-backward.
	16 * sizeof(void *) + 6 * sizeof(char) + 2 * sizeof(uint64_t) +
	// partial_sort/heap_select/make_heap/adjust_heap/push_heap/pop_heap/sort_heap.
	23 * sizeof(void *) + 11 * sizeof(std::ptrdiff_t) + 7 * sizeof(char) +
	4 * sizeof(uint64_t) +
	// comparator adapters and scalar compare/result carriers.
	8 * sizeof(void *) + 5 * sizeof(char) + 4 * sizeof(bool) +
	// Original copy/copy_move_a/a1/a2/copy_m: five 3-iterator parameter
	// scopes and their actual returned iterator carriers, then miter/niter,
	// niter-wrap, assign-one, memmove parameters/result and real Num/length.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// adjacent_find's 2 input+returned iterators; __adjacent_find's 2
	// input+next+returned; iter-equal's 2 input iterators/boolean result.
	9 * sizeof(void *) + 2 * sizeof(char) + sizeof(bool);
constexpr size_t shop_manifest_equal_frames =
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal, each three
	// iterators/result; simple flag, length, 3 niter-base calls; __memcmp.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	// array operator== has 2 reference parameters and boolean result.
	2 * sizeof(void *) + sizeof(bool);

bool shop_manifest_binding_valid_bounded(const shop_trade_recovery_forest_binding &binding,
					 shop_trade_recovery_forest_role role,
					 shop_manifest_reserve_fn reserve, void *context,
					 size_t outer_live) noexcept
{
	struct workspace
	{
		std::array<uint8_t, 32> expected{};
		std::array<uint64_t, SHOP_TRADE_RECOVERY_MAX_UIDS> ordered{};
		size_t count = 0;
		size_t remaining = 0;
		size_t levels = 0;
		size_t base = 0;
		size_t sort_frames = 0;
	};
	constexpr size_t own_frames = sizeof(workspace) + 3 * sizeof(void *) + sizeof(size_t) +
				      sizeof(shop_trade_recovery_forest_role) + sizeof(bool) +
				      sizeof(std::array<uint8_t, 32>) + shop_manifest_equal_frames;
	if (!shop_manifest_admit(outer_live, own_frames, reserve, context))
		return false;
	workspace work;
	work.base = outer_live + own_frames;
	if (!binding.present)
		return binding_empty(binding);
	work.count = binding.ordered_item_uids.size();
	if (work.count > SHOP_TRADE_RECOVERY_MAX_UIDS ||
	    binding.canonical_bytes < empty_item_list.size() ||
	    binding.canonical_bytes > PLAYER_SNAPSHOT_MAX_BYTES)
		return false;
	if (!work.count)
		return binding.canonical_bytes == empty_item_list.size() &&
		       shop_manifest_digest_bounded(empty_item_list, role, 0, &work.expected,
						    reserve, context, work.base) &&
		       binding.canonical_digest == work.expected;
	if (binding.canonical_bytes == empty_item_list.size() ||
	    binding.canonical_digest == std::array<uint8_t, 32>{})
		return false;
	work.remaining = work.count;
	while (work.remaining > 1)
	{
		work.remaining >>= 1;
		++work.levels;
	}
	// Original __sort depth is exactly 2*floor(log2(count)); account entry
	// depth plus the final exhausted child. No arbitrary recursion cap.
	work.sort_frames = (2 * work.levels + 1) * shop_manifest_sort_recursive_frame +
			   shop_manifest_sort_leaf_frames;
	if (!shop_manifest_admit(work.base, work.sort_frames, reserve, context))
		return false;
	std::copy(binding.ordered_item_uids.begin(), binding.ordered_item_uids.end(),
		  work.ordered.begin());
	std::sort(work.ordered.begin(), work.ordered.begin() + work.count);
	return work.ordered[0] && work.ordered[work.count - 1] != UINT64_MAX &&
	       std::adjacent_find(work.ordered.begin(), work.ordered.begin() + work.count) ==
		       work.ordered.begin() + work.count;
}

// Complete installed GCC13 ordinary allocator/vector declared-carrier closure.
// All terms are genuine source scopes named below; conservative sums include
// mutually exclusive branches, never a guessed cap or emitted-stack claim.
constexpr size_t shop_manifest_allocator_frames =
	// _M_allocate, allocator_traits::allocate, allocator::allocate (C++20):
	// each this/allocator reference, n and returned pointer; new_allocator
	// adds its genuine hint pointer; operator new n and returned pointer.
	3 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(void *) + sizeof(size_t) +
	// _M_deallocate/traits/allocator/new_allocator: allocator/this+p+n,
	// then sized operator delete p+n. Trivial element _Destroy closures.
	4 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) +
	(3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *)) +
	// vector max_size/_S_max_size/traits max_size/new_allocator::_M_max_size
	// references/results and actual diffmax/allocmax locals. C++20 allocator
	// has no max_size member; that inactive C++17 branch is not counted.
	4 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(size_t) +
	// traits::construct -> construct_at -> forward -> placement-new; all
	// constructor arguments here are real references to trivial values.
	3 * sizeof(void *) + 3 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
constexpr size_t shop_manifest_copy_frames =
	// __uninitialized_move_if_noexcept_a and __uninitialized_copy_a: 3
	// iterators+allocator-reference+returned iterator each. Runtime ordinary
	// uninitialized_copy's two boolean locals and __uninit_copy carrier.
	2 * (4 * sizeof(void *) + sizeof(void *)) + 3 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(void *) +
	// copy/copy_move_a/a1/a2/copy_m, each3 iterator params+return; real
	// miter/niter/wrap/assign_one and memmove argument/result scopes.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// distance/__distance and normal-iterator subtraction/base/dereference/
	// ++/comparison/constructor source parameter/return scopes.
	2 * (2 * sizeof(void *) + sizeof(std::ptrdiff_t)) + sizeof(char) +
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) +
	// Fitting forward insert reaches advance(__mid,__elems_after), even zero.
	// advance: iterator-reference, size_t n, real local difference_type __d;
	// __iterator_category: iterator-reference and actual returned RA tag;
	// __advance: iterator-reference, difference n and by-value RA tag;
	// actual += this/n/reference-return, plus source ++/-- alternatives.
	sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + sizeof(void *) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 2 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	4 * sizeof(void *);
constexpr size_t shop_manifest_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t shop_manifest_default_frames =
	// Runtime default_n_a/default_n/default_n_1<true>: real first/n/allocator
	// reference, can_fill and val locals, actual returned pointer carriers.
	(3 * sizeof(void *) + sizeof(size_t)) +
	(2 * sizeof(void *) + sizeof(size_t) + sizeof(bool)) +
	(3 * sizeof(void *) + sizeof(size_t)) +
	// _Construct's real location plus placement-new n/location/result.
	sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// fill_n/__fill_n_a<random_access>: first/n/value/result/tag;
	// __size_to_integer argument/result; __fill_a/__fill_a1 scalar __tmp.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(char) + 2 * sizeof(size_t) +
	2 * (3 * sizeof(void *)) + sizeof(uint64_t);
constexpr size_t shop_manifest_vector_frames =
	shop_manifest_allocator_frames + shop_manifest_copy_frames + shop_manifest_relocate_frames +
	shop_manifest_default_frames +
	// reserve this/n/old_size/tmp; assign public/forward-aux and exact
	// _M_allocate_and_copy's this/n/first/last/result/returned pointer.
	2 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(char) + 5 * sizeof(void *) + sizeof(size_t) +
	// push_back/emplace_back and real realloc_insert old/new start/finish,
	// len/elems_before/position/forward value reference; _M_check_len.
	2 * sizeof(void *) + 3 * sizeof(void *) + 7 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(void *) + 3 * sizeof(size_t) +
	// C++20 forward insert public/range-insert (no old dispatch), offset/elems_after/
	// len/old-start/finish/mid/new-start/finish/iterator return/tag scopes.
	15 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(std::ptrdiff_t) + sizeof(char) +
	// default_append's n/size/navail/len and real old/new/destroy pointers.
	5 * sizeof(void *) + 4 * sizeof(size_t) +
	// begin/end/cbegin/size/capacity/get-allocator declared carriers and
	// iterator-category/std::max arguments/results on the real call paths.
	7 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(char) + 3 * sizeof(void *);
constexpr size_t shop_manifest_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + shop_manifest_allocator_frames;

struct shop_manifest_encode_workspace
{
	std::vector<uint8_t> candidate;
	std::array<uint8_t, 4> magic{ 'S', 'R', 'M', '8' };
	std::array<const shop_trade_recovery_forest_binding *, SHOP_TRADE_RECOVERY_FOREST_COUNT>
		values{};
	size_t base = 0;
	size_t current = 0;
	size_t requested = 0;
	size_t new_capacity = 0;
	size_t index = 0;
	size_t byte = 0;
	shop_manifest_reserve_fn reserve = nullptr;
	void *context = nullptr;

	bool admit(size_t request) noexcept
	{
		current = base;
		return shop_manifest_add(current, candidate.capacity()) &&
		       shop_manifest_admit(current, request, reserve, context);
	}
	bool growth(size_t count) noexcept
	{
		requested = 0;
		if (count > candidate.max_size() - candidate.size())
			return false;
		if (count > candidate.capacity() - candidate.size())
		{
			// Real GCC13 _M_check_len(n): size + max(size,n), not old cap.
			new_capacity = candidate.size() > count ? candidate.size() : count;
			if (new_capacity > candidate.max_size() - candidate.size())
				new_capacity = candidate.max_size();
			else
				new_capacity += candidate.size();
			requested = new_capacity;
		}
		// Entry admission owns all fixed method carriers. A fitting write
		// changes neither heap capacity nor ownership; reserve only before
		// each genuine prospective allocation, not every encoded byte.
		return !requested || admit(requested);
	}
	template <typename T> bool append(T value)
	{
		using unsigned_type = std::make_unsigned_t<T>;
		const unsigned_type encoded = static_cast<unsigned_type>(value);
		for (byte = 0; byte < sizeof(T); ++byte)
		{
			if (!growth(1))
				return false;
			candidate.push_back(static_cast<uint8_t>(encoded >> (byte * 8)));
		}
		return true;
	}
	bool digest_bytes(const std::array<uint8_t, 32> &digest)
	{
		if (!growth(digest.size()))
			return false;
		candidate.insert(candidate.end(), digest.begin(), digest.end());
		return true;
	}
};

struct shop_manifest_decode_workspace
{
	shop_trade_recovery_manifest candidate;
	std::vector<uint8_t> canonical;
	std::array<shop_trade_recovery_forest_binding *, SHOP_TRADE_RECOVERY_FOREST_COUNT> values{};
	const std::array<uint8_t, 4> magic{ 'S', 'R', 'M', '8' };
	const uint8_t *cursor = nullptr;
	const uint8_t *end = nullptr;
	size_t base = 0;
	size_t current = 0;
	size_t manifest_heap = 0;
	size_t index = 0;
	size_t uid_index = 0;
	uint16_t version = 0;
	uint16_t padding = 0;
	uint16_t count = 0;
	uint8_t role = 0;
	uint8_t present = 0;
	shop_manifest_reserve_fn reserve = nullptr;
	void *context = nullptr;

	bool admit(size_t request) noexcept
	{
		current = base;
		return shop_manifest_heap(candidate, &manifest_heap) &&
		       shop_manifest_add(current, manifest_heap) &&
		       shop_manifest_add(current, canonical.capacity()) &&
		       shop_manifest_admit(current, request, reserve, context);
	}
};
} // namespace

bool shop_trade_recovery_manifest_shape_valid_bounded(const shop_trade_recovery_manifest &manifest,
						      bool (*reserve)(size_t, void *) noexcept,
						      void *context, size_t outer_live) noexcept
{
	constexpr size_t own_frames =
		2 * sizeof(std::array<const shop_trade_recovery_forest_binding *,
				      SHOP_TRADE_RECOVERY_FOREST_COUNT>) +
		3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool);
	if (!shop_manifest_storage_profile() ||
	    !shop_manifest_admit(outer_live, own_frames, reserve, context))
		return false;
	const auto values = bindings(manifest);
	for (size_t index = 0; index < values.size(); ++index)
		if ((index < 4 && !values[index]->present) ||
		    !shop_manifest_binding_valid_bounded(
			    *values[index], static_cast<shop_trade_recovery_forest_role>(index + 1),
			    reserve, context, outer_live + own_frames))
			return false;
	return manifest.live_target_before.present == manifest.live_target_after.present;
}

bool shop_trade_recovery_manifest_encode_bounded(const shop_trade_recovery_manifest &manifest,
						 std::vector<uint8_t> *out,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live,
						 size_t *retained_encoded_heap_bytes) noexcept
{
	constexpr size_t parameter_frames = 5 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
	size_t base = outer_live;
	// These genuine caller parameters/base remain alive during the first full
	// shape proof, before the encoder workspace itself is constructed.
	if (!out || !shop_manifest_storage_profile() ||
	    !shop_manifest_add(base, sizeof(base) + parameter_frames) ||
	    !shop_trade_recovery_manifest_shape_valid_bounded(manifest, reserve, context, base))
		return false;
	constexpr size_t method_frames =
		// Actual append's by-value/unsigned scalar, this/result and range
		// insertion/copy carriers; real growth/add/admit call arguments.
		3 * sizeof(uint64_t) + 12 * sizeof(void *) + 10 * sizeof(size_t) +
		4 * sizeof(bool) +
		// Genuine GCC13 reallocation insert/forward-range/allocator argument,
		// old-start/finish/new-start/finish/position and length/result carriers.
		shop_manifest_vector_frames +
		// bindings(manifest) returns a distinct actual pointer-array carrier
		// before assignment into the already-counted workspace destination.
		sizeof(std::array<const shop_trade_recovery_forest_binding *,
				  SHOP_TRADE_RECOVERY_FOREST_COUNT>) +
		sizeof(void *);
	if (!shop_manifest_add(base, sizeof(shop_manifest_encode_workspace)) ||
	    !shop_manifest_add(base, method_frames) ||
	    !shop_manifest_admit(base, 0, reserve, context))
		return false;
	shop_manifest_encode_workspace work;
	work.base = base;
	work.reserve = reserve;
	work.context = context;
	static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
	try
	{
		// Original initializer-list vector owns a real fresh four-byte block.
		// Fresh forward assign requests exactly the same four-byte capacity.
		if (!work.admit(work.magic.size()))
			return false;
		work.candidate.assign(work.magic.begin(), work.magic.end());
		if (!work.append<uint16_t>(SHOP_TRADE_RECOVERY_MANIFEST_VERSION) ||
		    !work.append<uint16_t>(0))
			return false;
		work.values = bindings(manifest);
		for (work.index = 0; work.index < work.values.size(); ++work.index)
		{
			if (!work.append<uint8_t>(static_cast<uint8_t>(work.index + 1)) ||
			    !work.append<uint8_t>(work.values[work.index]->present ? 1 : 0) ||
			    !work.append<uint16_t>(static_cast<uint16_t>(
				    work.values[work.index]->ordered_item_uids.size())) ||
			    !work.append<uint32_t>(work.values[work.index]->canonical_bytes) ||
			    !work.digest_bytes(work.values[work.index]->canonical_digest))
				return false;
			for (const auto uid : work.values[work.index]->ordered_item_uids)
				if (!work.append<uint64_t>(uid))
					return false;
		}
		if (work.candidate.size() > SHOP_TRADE_RECOVERY_MANIFEST_MAX_BYTES)
			return false;
		// Strong output and optional scalar; no fallible callback after move.
		if (!work.admit(shop_manifest_move_frames))
			return false;
		work.requested = work.candidate.capacity();
		*out = std::move(work.candidate);
		if (retained_encoded_heap_bytes)
			*retained_encoded_heap_bytes = work.requested;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_recovery_manifest_decode_bounded(std::span<const uint8_t> bytes,
						 shop_trade_recovery_manifest *out,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer_live,
						 size_t *retained_manifest_heap_bytes) noexcept
{
	if (!out || !shop_manifest_storage_profile() ||
	    bytes.size() < SHOP_TRADE_RECOVERY_MANIFEST_MIN_BYTES ||
	    bytes.size() > SHOP_TRADE_RECOVERY_MANIFEST_MAX_BYTES)
		return false;
	constexpr size_t parameter_frames = sizeof(std::span<const uint8_t>) + 4 * sizeof(void *) +
					    sizeof(size_t) + sizeof(bool);
	constexpr size_t method_frames =
		// Real manifest census: bindings arrays/current binding/total, plus
		// read_le arguments/value/byte and UID range/copy/equality carriers.
		2 * sizeof(std::array<const shop_trade_recovery_forest_binding *,
				      SHOP_TRADE_RECOVERY_FOREST_COUNT>) +
		12 * sizeof(void *) + 10 * sizeof(size_t) + sizeof(uint64_t) +
		// Actual vector fresh default-append allocation/local pointer and
		// allocator/copy/move/result carriers; no arbitrary allocation cap.
		shop_manifest_vector_frames + shop_manifest_equal_frames;
	size_t base = outer_live;
	if (!shop_manifest_add(base, sizeof(shop_manifest_decode_workspace)) ||
	    !shop_manifest_add(base, sizeof(base) + parameter_frames + method_frames) ||
	    !shop_manifest_admit(base, 0, reserve, context))
		return false;
	shop_manifest_decode_workspace work;
	work.base = base;
	work.reserve = reserve;
	work.context = context;
	work.cursor = bytes.data();
	work.end = work.cursor + bytes.size();
	static_assert(std::is_nothrow_move_assignable_v<shop_trade_recovery_manifest>);
	try
	{
		if (!std::equal(work.magic.begin(), work.magic.end(), work.cursor))
			return false;
		work.cursor += work.magic.size();
		if (!read_le(&work.cursor, work.end, &work.version) ||
		    work.version != SHOP_TRADE_RECOVERY_MANIFEST_VERSION ||
		    !read_le(&work.cursor, work.end, &work.padding) || work.padding)
			return false;
		work.values = bindings(work.candidate);
		for (work.index = 0; work.index < work.values.size(); ++work.index)
		{
			if (!read_le(&work.cursor, work.end, &work.role) ||
			    work.role != static_cast<uint8_t>(work.index + 1) ||
			    !read_le(&work.cursor, work.end, &work.present) || work.present > 1 ||
			    !read_le(&work.cursor, work.end, &work.count) ||
			    work.count > SHOP_TRADE_RECOVERY_MAX_UIDS ||
			    !read_le(&work.cursor, work.end,
				     &work.values[work.index]->canonical_bytes) ||
			    static_cast<size_t>(work.end - work.cursor) <
				    work.values[work.index]->canonical_digest.size() +
					    work.count * sizeof(uint64_t))
				return false;
			work.values[work.index]->present = work.present != 0;
			std::copy(work.cursor,
				  work.cursor + work.values[work.index]->canonical_digest.size(),
				  work.values[work.index]->canonical_digest.begin());
			work.cursor += work.values[work.index]->canonical_digest.size();
			// Each actual candidate binding is fresh: original resize(count)
			// requests count*sizeof(uint64_t), and previous real UID capacities
			// stay in the full current prefix during this allocation.
			if (!work.admit(work.count * sizeof(uint64_t)))
				return false;
			work.values[work.index]->ordered_item_uids.resize(work.count);
			for (work.uid_index = 0;
			     work.uid_index < work.values[work.index]->ordered_item_uids.size();
			     ++work.uid_index)
				if (!read_le(&work.cursor, work.end,
					     &work.values[work.index]
						      ->ordered_item_uids[work.uid_index]))
					return false;
		}
		if (work.cursor != work.end || !work.admit(0) ||
		    !shop_trade_recovery_manifest_shape_valid_bounded(work.candidate, reserve,
								      context, work.current))
			return false;
		if (!work.admit(0) ||
		    !shop_trade_recovery_manifest_encode_bounded(work.candidate, &work.canonical,
								 reserve, context, work.current,
								 nullptr) ||
		    work.canonical.size() != bytes.size() ||
		    !std::equal(work.canonical.begin(), work.canonical.end(), bytes.begin()))
			return false;
		// The UID capacities have not changed since their fresh owning resize.
		// canonical remains alive until after transfer, in the admitted encoder
		// prefix. No output mutation/fallible reserve occurs before acceptance.
		if (!work.admit(shop_manifest_move_frames))
			return false;
		*out = std::move(work.candidate);
		if (retained_manifest_heap_bytes)
			*retained_manifest_heap_bytes = work.manifest_heap;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

// Complete original standalone forest companions; unselected.
bool shop_trade_recovery_forest_shape_valid_bounded(
	const shop_trade_recovery_forest_binding &binding, shop_trade_recovery_forest_role role,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer_live) noexcept
{
	constexpr size_t frames = 3 * sizeof(void *) + sizeof(shop_trade_recovery_forest_role) +
				  sizeof(size_t) + sizeof(bool) + sizeof(size_t);
	size_t base = outer_live;
	if (!valid_role(role) || !shop_manifest_storage_profile() ||
	    !shop_manifest_add(base, frames) || !shop_manifest_admit(base, 0, reserve, context))
		return false;
	return shop_manifest_binding_valid_bounded(binding, role, reserve, context, base);
}

bool shop_trade_recovery_forest_encode_bounded(const shop_trade_recovery_forest_binding &binding,
					       shop_trade_recovery_forest_role role,
					       std::vector<uint8_t> *out,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live,
					       size_t *retained_encoded_heap_bytes) noexcept
{
	// Parameters and real base survive the pre-candidate shape proof.
	constexpr size_t parameters = 5 * sizeof(void *) + sizeof(shop_trade_recovery_forest_role) +
				      sizeof(size_t) + sizeof(bool);
	size_t base = outer_live;
	if (!out || !shop_manifest_storage_profile() ||
	    !shop_manifest_add(base, parameters + sizeof(base)) ||
	    !shop_trade_recovery_forest_shape_valid_bounded(binding, role, reserve, context, base))
		return false;
	constexpr size_t method_frames =
		sizeof(std::vector<uint8_t>) + 3 * sizeof(size_t) +
		// append_le: out, value, unsigned encoded, index; largest instantiation.
		sizeof(void *) + 2 * sizeof(uint64_t) + sizeof(size_t) +
		// Range-for begin/end/reference and actual uid by-value carrier.
		3 * sizeof(void *) + sizeof(uint64_t) +
		// Default vector/base/impl/data constructors and genuine by-value allocator.
		4 * sizeof(void *) + sizeof(std::allocator<uint8_t>) +
		// add/admit actual reference/scalar/function/context/result carriers.
		5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool) +
		// Exact already-defined reserve/fitting push/insert/copy/advance/cleanup.
		shop_manifest_vector_frames;
	if (!shop_manifest_add(base, method_frames) ||
	    !shop_manifest_admit(base, 0, reserve, context))
		return false;
	static_assert(std::is_nothrow_move_assignable_v<std::vector<uint8_t>>);
	try
	{
		std::vector<uint8_t> candidate;
		const size_t requested = SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
					 binding.ordered_item_uids.size() * sizeof(uint64_t);
		// Fresh original reserve requests exactly requested bytes. Full fixed
		// STL call profile stays in base for fitting append/insert too.
		if (!shop_manifest_admit(base, requested, reserve, context))
			return false;
		candidate.reserve(SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
				  binding.ordered_item_uids.size() * sizeof(uint64_t));
		append_le<uint8_t>(&candidate, static_cast<uint8_t>(role));
		append_le<uint8_t>(&candidate, binding.present ? 1 : 0);
		append_le<uint16_t>(&candidate,
				    static_cast<uint16_t>(binding.ordered_item_uids.size()));
		append_le<uint32_t>(&candidate, binding.canonical_bytes);
		candidate.insert(candidate.end(), binding.canonical_digest.begin(),
				 binding.canonical_digest.end());
		for (const auto uid : binding.ordered_item_uids)
			append_le<uint64_t>(&candidate, uid);
		size_t current = base;
		if (!shop_manifest_add(current, candidate.capacity()) ||
		    !shop_manifest_admit(current, shop_manifest_move_frames, reserve, context))
			return false;
		const size_t retained = candidate.capacity();
		*out = std::move(candidate);
		if (retained_encoded_heap_bytes)
			*retained_encoded_heap_bytes = retained;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool shop_trade_recovery_forest_decode_bounded(std::span<const uint8_t> bytes,
					       shop_trade_recovery_forest_role expected_role,
					       shop_trade_recovery_forest_binding *out,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live,
					       size_t *retained_forest_heap_bytes) noexcept
{
	if (!out || !shop_manifest_storage_profile() || !valid_role(expected_role) ||
	    bytes.size() < SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES ||
	    bytes.size() > SHOP_TRADE_RECOVERY_FOREST_HEADER_BYTES +
				   SHOP_TRADE_RECOVERY_MAX_UIDS * sizeof(uint64_t))
		return false;
	constexpr size_t frames =
		sizeof(std::span<const uint8_t>) + 4 * sizeof(void *) +
		sizeof(shop_trade_recovery_forest_role) + sizeof(size_t) + sizeof(bool) +
		// Original cursor/end, role/present/count, candidate and UID reference.
		3 * sizeof(void *) + 2 * sizeof(uint8_t) + sizeof(uint16_t) +
		sizeof(shop_trade_recovery_forest_binding) + 3 * sizeof(size_t) +
		// Largest read_le instantiation: cursor/end/out, unsigned value/index/result.
		3 * sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(bool) +
		// Candidate aggregate/vector default construction, no heap allocation.
		5 * sizeof(void *) + sizeof(std::allocator<uint64_t>) +
		// Actual add/admit/shape handoff scalar and pointer carriers.
		5 * sizeof(void *) + 4 * sizeof(size_t) + 2 * sizeof(bool) +
		shop_manifest_vector_frames;
	size_t base = outer_live;
	if (!shop_manifest_add(base, frames) || !shop_manifest_admit(base, 0, reserve, context))
		return false;
	static_assert(std::is_nothrow_move_assignable_v<shop_trade_recovery_forest_binding>);
	try
	{
		const uint8_t *cursor = bytes.data(), *end = cursor + bytes.size();
		uint8_t role = 0, present = 0;
		uint16_t count = 0;
		shop_trade_recovery_forest_binding candidate;
		if (!read_le(&cursor, end, &role) || role != static_cast<uint8_t>(expected_role) ||
		    !read_le(&cursor, end, &present) || present > 1 ||
		    !read_le(&cursor, end, &count) || count > SHOP_TRADE_RECOVERY_MAX_UIDS ||
		    !read_le(&cursor, end, &candidate.canonical_bytes) ||
		    static_cast<size_t>(end - cursor) !=
			    candidate.canonical_digest.size() + count * sizeof(uint64_t))
			return false;
		candidate.present = present != 0;
		std::copy(cursor, cursor + candidate.canonical_digest.size(),
			  candidate.canonical_digest.begin());
		cursor += candidate.canonical_digest.size();
		// Fresh original resize requests count*sizeof(uint64_t), even though
		// the wire only claims count. Actual current capacity is observed.
		size_t current = base;
		if (candidate.ordered_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
		    !shop_manifest_add(current,
				       candidate.ordered_item_uids.capacity() * sizeof(uint64_t)) ||
		    !shop_manifest_admit(current, count * sizeof(uint64_t), reserve, context))
			return false;
		candidate.ordered_item_uids.resize(count);
		for (auto &uid : candidate.ordered_item_uids)
			if (!read_le(&cursor, end, &uid))
				return false;
		current = base;
		if (candidate.ordered_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
		    !shop_manifest_add(current,
				       candidate.ordered_item_uids.capacity() * sizeof(uint64_t)))
			return false;
		if (cursor != end || !shop_trade_recovery_forest_shape_valid_bounded(
					     candidate, expected_role, reserve, context, current))
			return false;
		if (!shop_manifest_admit(current, shop_manifest_move_frames + 2 * sizeof(void *),
					 reserve, context))
			return false;
		// Aggregate move's destination/source references coexist with vector move.
		const size_t retained = candidate.ordered_item_uids.capacity() * sizeof(uint64_t);
		*out = std::move(candidate);
		if (retained_forest_heap_bytes)
			*retained_forest_heap_bytes = retained;
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

namespace
{
constexpr size_t shop_forest_freeze_frames =
	sizeof(std::vector<player_item_snapshot>) + sizeof(std::vector<uint8_t>) +
	sizeof(shop_trade_recovery_forest_binding) +
	sizeof(std::array<int32_t, PLAYER_SNAPSHOT_MAX_DEPTH>) + 15 * sizeof(void *) +
	12 * sizeof(size_t) + sizeof(uint32_t) + 8 * sizeof(bool) +
	sizeof(shop_trade_recovery_forest_role) + 2 * sizeof(player_snapshot_codec_result) +
	shop_manifest_vector_frames + shop_manifest_move_frames + shop_manifest_equal_frames;
struct shop_forest_freeze_budget
{
	shop_manifest_reserve_fn reserve;
	void *context;
	size_t outer;
	const std::vector<player_item_snapshot> *items = nullptr;
	const std::vector<uint8_t> *canonical = nullptr;
	const shop_trade_recovery_forest_binding *binding = nullptr;
	bool prefix(size_t &value, size_t extra = 0) const noexcept
	{
		value = outer;
		size_t heap = 0;
		if (!shop_manifest_add(value, sizeof(*this) + shop_forest_freeze_frames) ||
		    !shop_manifest_add(value, extra))
			return false;
		if (items && (!player_item_snapshot_list_current_heap_bytes(*items, &heap) ||
			      !shop_manifest_add(value, heap)))
			return false;
		if (canonical && !shop_manifest_add(value, canonical->capacity()))
			return false;
		if (binding &&
		    (binding->ordered_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
		     !shop_manifest_add(value,
					binding->ordered_item_uids.capacity() * sizeof(uint64_t))))
			return false;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t value = 0;
		return prefix(value, extra) && reserve && reserve(value, context);
	}
};
}
size_t shop_trade_recovery_forest_freeze_frame_bytes() noexcept
{
	return sizeof(shop_forest_freeze_budget) + shop_forest_freeze_frames;
}
bool shop_trade_recovery_forest_freeze_bounded(std::span<const uint8_t> canonical_bytes,
					       shop_trade_recovery_forest_role role,
					       shop_trade_recovery_forest_binding *out,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept
{
	if (!out || !valid_role(role) || canonical_bytes.empty() ||
	    canonical_bytes.size() > PLAYER_SNAPSHOT_MAX_BYTES)
		return false;
	shop_forest_freeze_budget budget{ reserve, context, outer_live };
	if (!budget.peak())
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
		budget.items = &items;
		budget.canonical = &canonical;
		size_t nested = 0;
		if (!budget.prefix(nested) ||
		    player_item_snapshot_list_decode_bounded(
			    canonical_bytes.data(), canonical_bytes.size(), &items, reserve,
			    context, nested) != player_snapshot_codec_result::ok ||
		    items.size() > SHOP_TRADE_RECOVERY_MAX_UIDS || !contiguous_dfs(items))
			return false;
		if (!budget.prefix(nested) ||
		    player_item_snapshot_list_encode_bounded(items, &canonical, reserve, context,
							     nested) !=
			    player_snapshot_codec_result::ok ||
		    canonical.size() != canonical_bytes.size() ||
		    !std::equal(canonical.begin(), canonical.end(), canonical_bytes.begin()))
			return false;
		shop_trade_recovery_forest_binding candidate;
		budget.binding = &candidate;
		candidate.present = true;
		candidate.canonical_bytes = static_cast<uint32_t>(canonical.size());
		if (items.size() > SIZE_MAX / sizeof(uint64_t) ||
		    !budget.peak(items.size() * sizeof(uint64_t)))
			return false;
		candidate.ordered_item_uids.reserve(items.size());
		for (const auto &item : items)
			candidate.ordered_item_uids.push_back(item.object_uid);
		if (!budget.prefix(nested) ||
		    !shop_manifest_digest_bounded(
			    canonical, role, static_cast<uint32_t>(items.size()),
			    &candidate.canonical_digest, reserve, context, nested) ||
		    !shop_trade_recovery_forest_shape_valid_bounded(candidate, role, reserve,
								    context, nested))
			return false;
		if (!budget.peak())
			return false;
		*out = std::move(candidate);
		return true;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}
bool shop_trade_recovery_forest_verify_bounded(std::span<const uint8_t> canonical_bytes,
					       shop_trade_recovery_forest_role role,
					       const shop_trade_recovery_forest_binding &binding,
					       bool (*reserve)(size_t, void *) noexcept,
					       void *context, size_t outer_live) noexcept
{
	if (!binding.present)
		return false;
	constexpr size_t own = sizeof(shop_trade_recovery_forest_binding) +
			       sizeof(std::span<const uint8_t>) + 7 * sizeof(void *) +
			       4 * sizeof(size_t) + 3 * sizeof(bool) +
			       sizeof(shop_trade_recovery_forest_role) + shop_manifest_equal_frames;
	size_t prefix = outer_live;
	if (!shop_manifest_add(prefix, own) || !shop_manifest_admit(prefix, 0, reserve, context))
		return false;
	shop_trade_recovery_forest_binding expected;
	if (!shop_trade_recovery_forest_freeze_bounded(canonical_bytes, role, &expected, reserve,
						       context, prefix))
		return false;
	if (expected.ordered_item_uids.capacity() > SIZE_MAX / sizeof(uint64_t) ||
	    !shop_manifest_admit(prefix, expected.ordered_item_uids.capacity() * sizeof(uint64_t),
				 reserve, context))
		return false;
	return expected == binding;
}

// Additive selected SRM8 SOURCE contracts. The original implementations above
// are retained byte-for-byte. Objects/heaps in their existing admissions are
// not added to the retained supplement below.
namespace
{
bool shop_manifest_source_supported() noexcept
{
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(SHA_LONG) == 4 && sizeof(std::allocator<uint64_t>) == 1 &&
	       sizeof(std::allocator<uint8_t>) == 1 &&
	       sizeof(std::vector<uint64_t>::iterator) == sizeof(void *) &&
	       sizeof(std::vector<uint8_t>::iterator) == sizeof(void *);
#else
	return false;
#endif
}

#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
// SOURCE_CORRECTION_GUARD
template <typename T> constexpr size_t shop_manifest_complete_move_source() noexcept
{
	// Actual selected C++20 vector operator=, std::move and _M_move_assign(true).
	return 3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + 2 * sizeof(void *) +
	       sizeof(std::true_type) + sizeof(std::vector<T>) +
	       // get_allocator: vector/base/allocator receivers and reference returns,
	       // allocator const-copy/base-copy; genuine returned allocator temporary.
	       7 * sizeof(void *) + sizeof(std::allocator<T>) +
	       // vector(const A&) -> base -> impl -> allocator/base copy -> data default.
	       11 * sizeof(void *) +
	       // BOTH _M_swap_data calls; each this/x, 3-pointer data temporary,
	       // default receiver and three _M_copy_data(this,source) calls.
	       2 * (2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(void *) +
		    3 * (2 * sizeof(void *))) +
	       2 * (2 * sizeof(void *)) +
	       // Direct C++20 __alloc_on_move (two refs), std::move (two refs),
	       // generated allocator/base assignments (three refs each).
	       2 * sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) + 3 * sizeof(void *) +
	       sizeof(void *) + shop_manifest_allocator_frames;
}
constexpr size_t shop_manifest_move_missing =
	// Exact named replacement difference: the old admission already carries
	// shop_manifest_move_frames. No workspace/UID allocation is subtracted.
	shop_manifest_complete_move_source<uint64_t>() - shop_manifest_move_frames;
static_assert(shop_manifest_complete_move_source<uint64_t>() >= shop_manifest_move_frames);
static_assert(shop_manifest_complete_move_source<uint64_t>() ==
	      shop_manifest_complete_move_source<uint8_t>());

constexpr size_t shop_manifest_value_lifetime_source =
	// Genuine manifest/binding/workspace generated default/destructor receivers;
	// six reached UID-vector defaults (vector/base/impl/allocator/base/data).
	3 * sizeof(void *) + 6 * (sizeof(void *) + 6 * sizeof(void *)) +
	// Destruction outside the encoder's growth admission: six UID vectors and
	// the actual canonical/result byte vectors. Each vector/Destroy/base source
	// plus actual selected allocator deallocation source.
	8 * (4 * sizeof(void *) + shop_manifest_allocator_frames) +
	// Manifest generated move(this,source), its six member binding moves,
	// their generated this/source and std::move argument/reference carriers.
	2 * sizeof(void *) + 6 * (2 * sizeof(void *) + 2 * sizeof(void *)) +
	shop_manifest_move_missing +
	// The byte-vector default (same six-this chain) before first growth.
	2 * 6 * sizeof(void *);

constexpr size_t shop_manifest_array_methods_source =
	// Actual array begin/end -> data -> _S_ptr (each receiver/reference/result),
	// indexing -> _S_ref(receiver,index,result), and size(receiver,result).
	// One genuine specialization; selected codec reaches mutable/const pointer
	// arrays, magic4/header11/digest32 and ordered-UID4096 arrays.
	2 * (2 * sizeof(void *)) + 2 * (2 * sizeof(void *)) + 2 * sizeof(void *) +
	2 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t);
constexpr size_t shop_manifest_span_methods_source =
	// Dynamic span pointer/count ctor -> extent-storage ctor; data and
	// size/extent accessors; actual array-range span ctor for empty_item_list.
	2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	2 * (sizeof(void *) + sizeof(size_t)) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
	// pointer/count constructor calls raw to_address and __to_address.
	2 * sizeof(void *) + 2 * sizeof(void *);
constexpr size_t shop_manifest_array_span_source =
	6 * shop_manifest_array_methods_source + shop_manifest_span_methods_source;

constexpr size_t shop_manifest_scalar_missing =
	// Existing binding_valid/shape/codec admissions own their working arrays,
	// read/append scalars, parameters and vector/sort/hash expressions. These
	// are additional actual helper scopes: bindings overload(ref,return array),
	// valid_role(arg,bool), binding_empty(ref,bool,zero digest temporary already
	// present in old shape admission), heap/add/admit and methods this/result.
	2 * (sizeof(void *) + sizeof(std::array<const shop_trade_recovery_forest_binding *, 6>)) +
	sizeof(shop_trade_recovery_forest_role) + sizeof(bool) + sizeof(void *) + sizeof(bool) +
	// shop_manifest_heap: manifest/output, total, binding array, range/begin/end/
	// current binding, returned boolean; original workspace excludes these.
	6 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	sizeof(std::array<const shop_trade_recovery_forest_binding *, 6>) +
	// add(ref,bytes,bool), admit(current,request,reserve,context,bool),
	// encode admit/growth/append/digest_bytes and decode admit receivers/results.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	sizeof(bool) + 5 * (sizeof(void *) + sizeof(bool)) +
	// Real UID range-for reference/begin/end/current value in encoder.
	3 * sizeof(void *) + sizeof(uint64_t) +
	// Generated default/copy assignment for fixed magic/header/digest arrays.
	3 * (2 * sizeof(void *)) +
	// Actual UID vector::empty -> begin/end -> iterator equality/base chain,
	// and shape's size accessor outside the codec vector operation admission.
	11 * sizeof(void *) + 2 * sizeof(bool) + sizeof(void *) + sizeof(size_t) +
	// read_le's returned bool is not in old decode method_frames (12P+10N+u64).
	sizeof(bool) +
	// The two selected manifest codec catch(const std::bad_alloc&) references.
	// They are lower scopes, distinct from the shop command/encoder catches.
	2 * sizeof(void *);

constexpr size_t shop_manifest_sort_missing =
	// Original old sort leaf already owns sort/partition/insertion/heap/copy
	// and adjacent_find public/core params. Actual typed comparator adapter
	// constructors/moves and invocation receiver scopes remain uncovered.
	// _Iter_less_iter, _Val_less_iter, _Iter_less_val are empty real classes.
	3 * (sizeof(void *) + sizeof(char)) + 6 * (2 * sizeof(void *)) +
	3 * (3 * sizeof(void *) + sizeof(bool)) +
	// Actual scalar iter_swap -> swap: two refs, one uint64 move temporary,
	// two move reference chains and iter_swap receiver iterators.
	4 * sizeof(void *) + sizeof(uint64_t) + 2 * (2 * sizeof(void *)) +
	// Normal-iterator constructor/base/deref/arithmetic/comparison wrappers
	// used by copy(begin/end) before the raw-pointer sort, plus category.
	6 * (2 * sizeof(void *)) + sizeof(std::ptrdiff_t) + sizeof(bool) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag);

constexpr size_t shop_manifest_selected_supplement =
	shop_manifest_value_lifetime_source + shop_manifest_array_span_source +
	shop_manifest_scalar_missing + shop_manifest_sort_missing;

// Authentic fixed SHA source has distinct Init/Update/Final memory phases.
// Their memcpy/memset/OPENSSL_cleanse leaves do not coexist with the block
// phase. The existing max(block)+max(Init,Update,Final) therefore dominates
// every reached phase without an extra constant allowance or EVP substitution.
constexpr size_t shop_manifest_sha_memcpy = 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t shop_manifest_sha_memset = 2 * sizeof(void *) + sizeof(int) + sizeof(size_t);
constexpr size_t shop_manifest_sha_cleanse =
	sizeof(void *) + sizeof(size_t) + sizeof(void *) + shop_manifest_sha_memset;
static_assert(shop_manifest_sha256_assembly_frame >= shop_manifest_sha_memcpy);
static_assert(shop_manifest_sha256_assembly_frame >= shop_manifest_sha_memset);
static_assert(shop_manifest_sha256_assembly_frame >= shop_manifest_sha_cleanse);

constexpr size_t shop_manifest_old_shape_source =
	3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(bool) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(shop_trade_recovery_forest_role) + sizeof(bool) +
	shop_manifest_equal_frames +
	// Existing binding proof checks count <=4096 before its original sort.
	// floor(log2(4096))=12, exact introsort entry+exhausted-child depth25.
	25 * shop_manifest_sort_recursive_frame + shop_manifest_sort_leaf_frames +
	sizeof(std::span<const uint8_t>) + sizeof(shop_trade_recovery_forest_role) +
	sizeof(uint32_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	shop_manifest_sha256_nested_frame;
constexpr size_t shop_manifest_old_encode_source =
	5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + 3 * sizeof(uint64_t) +
	12 * sizeof(void *) + 10 * sizeof(size_t) + 4 * sizeof(bool) + shop_manifest_vector_frames +
	shop_manifest_move_frames +
	sizeof(std::array<const shop_trade_recovery_forest_binding *, 6>) + sizeof(void *);
constexpr size_t shop_manifest_old_decode_source =
	sizeof(std::span<const uint8_t>) + 4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) +
	2 * sizeof(std::array<const shop_trade_recovery_forest_binding *, 6>) +
	12 * sizeof(void *) + 10 * sizeof(size_t) + sizeof(uint64_t) + shop_manifest_vector_frames +
	shop_manifest_equal_frames + shop_manifest_move_frames;
constexpr size_t shop_manifest_selected_source =
	shop_manifest_selected_supplement + shop_manifest_old_shape_source +
	shop_manifest_old_encode_source + shop_manifest_old_decode_source;

// Original is_empty is reached directly by the shop legacy builders. It never
// reaches original binding_valid/digest/EVP: its actual leaf is binding_empty.
constexpr size_t shop_manifest_empty_source =
	sizeof(void *) + sizeof(bool) +
	2 * sizeof(std::array<const shop_trade_recovery_forest_binding *, 6>) + sizeof(void *) +
	// all_of -> find_if_not -> __find_if_not -> __find_if(random-access),
	// actual captureless predicate/negation/predicate adapters, tag/tripcount.
	4 * (3 * sizeof(void *) + sizeof(char)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 6 * (sizeof(void *) + sizeof(char)) +
	4 * (2 * sizeof(void *)) + 3 * sizeof(bool) +
	// Original lambda(this,binding,bool) -> binding_empty(ref,bool,zero digest).
	3 * sizeof(void *) + 2 * sizeof(bool) + sizeof(std::array<uint8_t, 32>) +
	// vector::empty(this,bool), begin/end and iterator comparison/base.
	sizeof(void *) + sizeof(bool) + 5 * (2 * sizeof(void *)) + sizeof(bool) +
	shop_manifest_equal_frames + 2 * shop_manifest_array_methods_source;
#endif
}

bool shop_trade_recovery_manifest_codec_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = shop_manifest_selected_source;
	return true;
#else
	return false;
#endif
}
bool shop_trade_recovery_manifest_codec_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = shop_manifest_selected_supplement;
	return true;
#else
	return false;
#endif
}
bool shop_trade_recovery_manifest_codec_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = sizeof(shop_manifest_decode_workspace) + sizeof(shop_manifest_encode_workspace);
	return true;
#else
	return false;
#endif
}
bool shop_trade_recovery_manifest_is_empty_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = shop_manifest_empty_source;
	return true;
#else
	return false;
#endif
}

#include "item/craft_recipe_continuation.h"
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
// PRIVATE exact standalone Forest3 descendants. Original algorithms unchanged.
// Named current recipe primitives describe only matching selected declarations;
// no recipe codec or manifest union is imported. Distinct actual occurrences
// are conservative sums; no assertion of simultaneous physical peak.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace forest_primitive = duris_craft_recipe_bounded_detail;
using forest_byte_I = std::vector<uint8_t>::iterator;
using forest_byte_CI = std::vector<uint8_t>::const_iterator;
using forest_byte_J = const uint8_t *;
using forest_byte_M = std::move_iterator<uint8_t *>;

template <class T> constexpr size_t forest_default_source = 6 * sizeof(void *);
template <class T> constexpr size_t forest_cleanup_source =
	22 * sizeof(void *) + 5 * sizeof(size_t) + 2 * sizeof(bool);
template <class T> constexpr size_t forest_move_source =
	40 * sizeof(void *) + sizeof(bool) + sizeof(std::true_type) + sizeof(std::vector<T>) +
	sizeof(std::allocator<T>);
template <class T> constexpr size_t forest_tail_source =
	forest_move_source<T> + forest_cleanup_source<T> - 2 * sizeof(void *);

template <class T> constexpr size_t forest_fresh_reserve_source =
	forest_primitive::complete_recipe_vector_reserve_own_source<T> +
	forest_primitive::complete_recipe_vector_max_size_source<T> +
	forest_primitive::complete_recipe_vector_capacity_source<T> +
	forest_primitive::complete_recipe_vector_size_source<T> +
	forest_primitive::complete_recipe_vector_allocate_source<T> +
	forest_primitive::complete_recipe_vector_mutable_allocator_source<T> +
	// Fresh empty relocation: three4P+pointer-result bodies and three actual
	// raw niter2P calls; countD0 excludes query/memmove branch. Old-null
	// deallocate still has this,p,n, never descends to free.
	21 * sizeof(void *) + sizeof(std::ptrdiff_t) + 2 * sizeof(void *) + sizeof(size_t);
constexpr size_t forest_fitting_byte_push_source =
	forest_primitive::complete_recipe_vector_push_own_source<uint8_t> +
	forest_primitive::complete_recipe_scalar_move_source<uint8_t> +
	forest_primitive::complete_recipe_vector_emplace_own_source<uint8_t> +
	forest_primitive::complete_recipe_scalar_forward_source<uint8_t> +
	forest_primitive::complete_recipe_vector_single_construct_source<uint8_t> + sizeof(void *) +
	sizeof(forest_byte_I) +
	forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_I> +
	forest_primitive::complete_recipe_vector_back_own_source<uint8_t> +
	forest_primitive::complete_recipe_normal_offset_own_source<forest_byte_I> +
	forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_I> +
	forest_primitive::complete_recipe_normal_dereference_source<forest_byte_I>;

template <class Output> constexpr size_t forest_digest_copy_source =
	forest_primitive::complete_recipe_copy_projected_source<forest_byte_J, Output> +
	2 * forest_primitive::complete_recipe_miter_argument_source<forest_byte_J> +
	forest_primitive::complete_recipe_copy_projected_source<forest_byte_J, Output> +
	2 * forest_primitive::complete_recipe_niter_graph_source<forest_byte_J> +
	forest_primitive::complete_recipe_niter_graph_source<Output> +
	forest_primitive::complete_recipe_raw_copy_a1_source<forest_byte_J, uint8_t *> +
	forest_primitive::complete_recipe_raw_copy_a2_source<forest_byte_J, uint8_t *> +
	forest_primitive::complete_recipe_byte_simple_copy_m_source +
	forest_primitive::complete_recipe_std_constant_evaluated_source +
	(std::is_pointer_v<Output> ?
		 forest_primitive::complete_recipe_raw_niter_wrap_source :
		 forest_primitive::complete_recipe_normal_niter_wrap_own_source<Output> +
			 forest_primitive::complete_recipe_niter_graph_source<Output> +
			 forest_primitive::complete_recipe_normal_offset_argument_source<Output> +
			 forest_primitive::complete_recipe_normal_pointer_constructor_source<Output>);
constexpr size_t forest_digest_uninitialized_copy_source =
	forest_primitive::complete_recipe_uninitialized_copy_a_source<forest_byte_J, uint8_t *> +
	forest_primitive::complete_recipe_std_internal_constant_evaluated_source +
	forest_primitive::complete_recipe_uninitialized_copy_source<forest_byte_J, uint8_t *> +
	forest_primitive::complete_recipe_uninitialized_copy_true_source<forest_byte_J, uint8_t *> +
	forest_digest_copy_source<uint8_t *>;
constexpr size_t forest_zero_move_copy_source =
	forest_primitive::complete_recipe_copy_own_source<forest_byte_M, uint8_t *> +
	2 * (forest_primitive::complete_recipe_move_miter_own_source<uint8_t> +
	     forest_primitive::complete_recipe_move_iterator_base_source<uint8_t> +
	     forest_primitive::complete_recipe_generic_miter_source<uint8_t *>)+forest_primitive::
			complete_recipe_copy_move_a_own_source<uint8_t *, uint8_t *> +
	3 * forest_primitive::complete_recipe_raw_niter_source<uint8_t> +
	forest_primitive::complete_recipe_raw_copy_a1_source<uint8_t *, uint8_t *> +
	forest_primitive::complete_recipe_raw_copy_a2_source<uint8_t *, uint8_t *> +
	forest_primitive::complete_recipe_byte_simple_copy_m_source +
	forest_primitive::complete_recipe_std_constant_evaluated_source +
	forest_primitive::complete_recipe_raw_niter_wrap_source;
constexpr size_t forest_zero_uninitialized_move_source =
	forest_primitive::complete_recipe_uninitialized_move_own_source<uint8_t> +
	2 * forest_primitive::complete_recipe_make_move_iterator_graph_source +
	forest_primitive::complete_recipe_uninitialized_copy_a_source<forest_byte_M, uint8_t *> +
	forest_primitive::complete_recipe_std_internal_constant_evaluated_source +
	forest_primitive::complete_recipe_uninitialized_copy_source<forest_byte_M, uint8_t *> +
	forest_primitive::complete_recipe_uninitialized_copy_true_source<forest_byte_M, uint8_t *> +
	forest_zero_move_copy_source;
constexpr size_t forest_digest_fitting_insert_source =
	forest_primitive::complete_recipe_byte_insert_own_source<forest_byte_J> +
	// range_insert this/oldfinish2P, position I, first/last/mid3J,n/after2N,F.
	2 * sizeof(void *) + sizeof(forest_byte_I) + 3 * sizeof(forest_byte_J) +
	2 * sizeof(size_t) + sizeof(std::forward_iterator_tag) +
	forest_primitive::complete_recipe_byte_const_iterator_conversion_source +
	forest_primitive::complete_recipe_normal_base_source<forest_byte_I> +
	forest_primitive::complete_recipe_byte_cbegin_own_source +
	forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_CI> +
	forest_primitive::complete_recipe_normal_subtract_own_source<forest_byte_CI> +
	2 * forest_primitive::complete_recipe_normal_base_source<forest_byte_CI> +
	2 * (forest_primitive::complete_recipe_wire_begin_own_source +
	     forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_I> +
	     forest_primitive::complete_recipe_normal_offset_argument_source<forest_byte_I> +
	     forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_I>)+
		// Category returns RA, distinct from range_insert's sliced F parameter.
		// No implicit trivial slicing callee; both real tag objects remain.
		forest_primitive::complete_recipe_iterator_category_source<forest_byte_J> +
	forest_primitive::complete_recipe_distance_own_source<forest_byte_J> +
	forest_primitive::complete_recipe_ra_distance_own_source<forest_byte_J> + sizeof(void *) +
	forest_primitive::complete_recipe_zero_advance_own_source<forest_byte_J> + sizeof(void *) +
	forest_primitive::complete_recipe_ra_advance_own_source<forest_byte_J> +
	// Raw pointer arithmetic has no iterator += callee. first!=last likewise
	// primitive; digest32 positive, old-element move0 and final false-copy0.
	forest_digest_uninitialized_copy_source + forest_zero_uninitialized_move_source +
	forest_digest_copy_source<forest_byte_I> +
	2 * forest_primitive::complete_recipe_vector_mutable_allocator_source<uint8_t> +
	forest_primitive::complete_recipe_wire_end_own_source +
	forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_I> +
	forest_primitive::complete_recipe_normal_subtract_own_source<forest_byte_I> +
	3 * forest_primitive::complete_recipe_normal_base_source<forest_byte_I> +
	// Caller end class survives I->CI conversion; digest begin/end are scalar
	// declared array results, separately mapped by the enclosing codec.
	forest_primitive::complete_recipe_wire_end_own_source +
	forest_primitive::complete_recipe_normal_pointer_constructor_source<forest_byte_I>;

constexpr size_t forest_uid_default_n_source =
	// Actual standard allocator specialization ignores allocator, but owns
	// allocator refP,first/result2P,nN. default_n owns first/result2P,nN,
	// declared can_fillB and std::is_constant_evaluated resultB.
	3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(bool) + 3 * sizeof(void *) + sizeof(size_t) +
	// addressof2P; runtime _Construct locationP/queryB/placement2P+N.
	5 * sizeof(void *) + sizeof(bool) + sizeof(size_t) +
	// fill_n3P+N,size_to_integer2N,fill_n_a3P+N+RA; category returned tag
	// is that exact RA formal. fill_a3P, scalar fill_a1 three refs+pointees
	// first/last/value3P plus genuine uint64 __tmp. No byte memset/query.
	3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(size_t) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(std::random_access_iterator_tag) + sizeof(void *) +
	3 * sizeof(void *) + 3 * sizeof(void *) + sizeof(uint64_t);
constexpr size_t forest_uid_empty_resize_source =
	sizeof(void *) + sizeof(size_t) +
	2 * forest_primitive::complete_recipe_vector_size_source<uint64_t> +
	// Selected _M_default_append this/oldstart/oldfinish/newstart4P,n/size/
	// navail/len4N. Nonrelocate __destroy_from and erase_at_end not selected.
	4 * sizeof(void *) + 4 * sizeof(size_t) +
	forest_primitive::complete_recipe_vector_size_source<uint64_t> +
	2 * forest_primitive::complete_recipe_vector_max_size_source<uint64_t> +
	forest_uid_default_n_source +
	forest_primitive::complete_recipe_vector_check_len_own_source<uint64_t> +
	4 * forest_primitive::complete_recipe_vector_size_source<uint64_t> +
	2 * forest_primitive::complete_recipe_vector_max_size_source<uint64_t> +
	forest_primitive::complete_recipe_size_max_source +
	forest_primitive::complete_recipe_vector_allocate_source<uint64_t> +
	forest_primitive::complete_recipe_vector_mutable_allocator_source<uint64_t> +
	21 * sizeof(void *) + sizeof(std::ptrdiff_t) +
	// Old allocation is null, so only _M_deallocate's actual2P+N.
	2 * sizeof(void *) + sizeof(size_t);
#endif
} // namespace
// PRIVATE exact binding_valid_bounded subtree. Conservative sums of distinct
// selected occurrences are Source bounds, not simultaneous physical peaks.
// Original fixed workspace/zero-array INLINE is admitted before construction;
// no original partial library/sort/crypto margin is claimed as credit.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
using forest_sort_C = __gnu_cxx::__ops::_Iter_less_iter;
using forest_sort_V = __gnu_cxx::__ops::_Val_less_iter;
using forest_sort_IV = __gnu_cxx::__ops::_Iter_less_val;
using forest_equal_E = __gnu_cxx::__ops::_Iter_equal_to_iter;
using forest_uid_CI = std::vector<uint64_t>::const_iterator;
constexpr size_t forest_array32_equal_source =
	// array==,three begin/end+data,equal/aux/aux1,three raw niter,
	// equal<true>,declared __memcmp and actual constant-evaluated query.
	// Bare builtin memcmp contributes no invented declaration carriers.
	34 * sizeof(void *) + 2 * sizeof(size_t) + 7 * sizeof(bool) + sizeof(int);
constexpr size_t forest_binding_empty_source =
	sizeof(void *) + sizeof(bool) +
	// vector.empty and its two actual const iterator expression objects.
	13 * sizeof(void *) + 2 * sizeof(forest_uid_CI) + 2 * sizeof(bool) +
	forest_array32_equal_source;
constexpr size_t forest_uid_copy_source =
	// Actual const-normal input and raw-array output: count1 assign_one
	// remains reachable. Class result destinations are actual parameters.
	// Source begin/end and destination begin/data are included here.
	8 * sizeof(forest_uid_CI) + 39 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(bool);
constexpr size_t forest_uid_sort_source =
	// Original4096 guard implies original depth2*floor(log2(count))+1<=25.
	// No new runtime limit or raw-iterator wrapper is introduced.
	25 * (3 * sizeof(void *) + sizeof(std::ptrdiff_t) + sizeof(forest_sort_C)) +
	// Public wrappers; genuine nonrecursive own scopes; bit depth-query;
	// selected comparator/conversion scopes; eighteen scalar move sites;
	// actual raw move_backward (including its reachable assign_one).
	136 * sizeof(void *) + 12 * sizeof(std::ptrdiff_t) + 12 * sizeof(forest_sort_C) +
	sizeof(forest_sort_V) + sizeof(forest_sort_IV) + 7 * sizeof(uint64_t) +
	2 * sizeof(unsigned long) + 8 * sizeof(int) + 4 * sizeof(bool);
constexpr size_t forest_uid_adjacent_source =
	10 * sizeof(void *) + sizeof(forest_equal_E) + sizeof(bool);
constexpr size_t forest_binding_array_access_source =
	// Sort two raw begin/data occurrences; adjacent three; two genuine
	// mutable subscripts. Copy destination begin/data is already above.
	20 * sizeof(void *) + 2 * (2 * sizeof(void *) + sizeof(size_t));

constexpr size_t forest_fixed_sha_init_source = sizeof(void *) + sizeof(int);
constexpr size_t forest_fixed_sha_update_source =
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(uint32_t) + sizeof(int);
constexpr size_t forest_fixed_sha_final_source = 3 * sizeof(void *) + sizeof(size_t) +
						 sizeof(unsigned long) + sizeof(uint32_t) +
						 sizeof(int);
constexpr size_t forest_fixed_sha_assembly_envelope =
	// Authenticated OpenSSL3.0.13 Linux AVX2 schedule/alignment/saved
	// registers, temporary/red-zone pointer slot and incoming-return slot.
	// Explicit assembly storage, not invented C helper signatures.
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (1024 - 1) + 2 * sizeof(void *);
constexpr size_t forest_fixed_sha_c_small_body =
	29 * sizeof(uint32_t) + sizeof(int) + sizeof(void *);
constexpr size_t forest_fixed_sha_c_normal_body =
	27 * sizeof(uint32_t) + 2 * sizeof(int) + sizeof(void *);
constexpr size_t forest_fixed_sha_block_source =
	// Genuine declared block signature, independently of its body variant.
	2 * sizeof(void *) + sizeof(size_t) +
	std::max(forest_fixed_sha_assembly_envelope,
		 std::max(forest_fixed_sha_c_small_body, forest_fixed_sha_c_normal_body));
constexpr size_t forest_fixed_sha_source =
	// Domain30 (including NUL),header11,list4: updates never process block.
	// Final alone processes one block, then cleansing and eight-word output.
	forest_fixed_sha_init_source + 3 * forest_fixed_sha_update_source +
	forest_fixed_sha_final_source + forest_fixed_sha_block_source +
	// Three real C memcpy declarations, Init/Final memset declarations,
	// one cleanse declaration. External implementation/linked ABI OPEN.
	3 * (3 * sizeof(void *) + sizeof(size_t)) +
	2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int)) + sizeof(void *) + sizeof(size_t);
constexpr size_t forest_fixed_digest_own_source =
	// span/role/count/out/reserve/context/outer/return plus constexpr
	// parametersN and generated nontrivial plain workspace receiverP.
	sizeof(std::span<const uint8_t>) + sizeof(shop_trade_recovery_forest_role) +
	sizeof(uint32_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(size_t) +
	sizeof(void *);
constexpr size_t forest_fixed_digest_matched_source =
	// Exactly original parameters literal; workspace stays child INLINE.
	sizeof(std::span<const uint8_t>) + sizeof(shop_trade_recovery_forest_role) +
	sizeof(uint32_t) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(bool);
constexpr size_t forest_fixed_digest_source =
	forest_fixed_digest_own_source +
	// Genuine valid_role and array4->dynamic-span conversion; resulting
	// span is the actual by-value digest parameter, not another S object.
	sizeof(shop_trade_recovery_forest_role) + sizeof(bool) + 11 * sizeof(void *) +
	2 * sizeof(size_t) +
	// Five header subscripts,array data/size,three span size/extent,data.
	23 * sizeof(void *) + 12 * sizeof(size_t) + forest_fixed_sha_source +
	// Explicitly declared span destructor; no implicit extent destructor.
	sizeof(void *);
constexpr size_t forest_binding_own_source =
	3 * sizeof(void *) + sizeof(shop_trade_recovery_forest_role) + sizeof(size_t) +
	sizeof(bool) + sizeof(size_t) + sizeof(void *);
constexpr size_t forest_binding_own_matched_source = 3 * sizeof(void *) +
						     sizeof(shop_trade_recovery_forest_role) +
						     sizeof(size_t) + sizeof(bool);
constexpr size_t forest_binding_source =
	forest_binding_own_source + forest_binding_empty_source + forest_array32_equal_source +
	// Binding size accessor plus three lexical empty-list array size calls.
	4 * (sizeof(void *) + sizeof(size_t)) + forest_uid_copy_source + forest_uid_sort_source +
	forest_uid_adjacent_source + forest_binding_array_access_source +
	forest_fixed_digest_source +
	// Three actual admit sites: initial workspace,sort,fixed digest.
	// Each current/request/reserve/context/return and numeric max result.
	3 * (2 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool));
constexpr size_t forest_binding_matched_source =
	forest_binding_own_matched_source + forest_fixed_digest_matched_source;
static_assert(forest_binding_source >= forest_binding_matched_source);
#endif
} // namespace
// PRIVATE exact standalone target composition. Work/candidate owner INLINE
// remains in the unchanged child admission preceding its construction. None
// exists before the first callback: standalone INITIAL=0. Genuine library
// temporaries/parameters/results remain Source; no input/prior-output heap copy.
namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
constexpr size_t forest_add_admit_source =
	// add(total-ref,bytes,bool), admit(current,request,reserve,context,bool),
	// numeric_limits<size_t>::max real returned scalar once, storage bool.
	3 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool);
constexpr size_t forest_role_source = sizeof(shop_trade_recovery_forest_role) + sizeof(bool);
constexpr size_t forest_shape_own_source = 3 * sizeof(void *) +
					   sizeof(shop_trade_recovery_forest_role) +
					   3 * sizeof(size_t) + sizeof(bool);
constexpr size_t forest_shape_own_matched_source =
	// Original frames matches signature3P,role,outer/base2N,returnedB.
	// Its own constexpr frames declaration remains genuinely new uncoveredN.
	3 * sizeof(void *) + sizeof(shop_trade_recovery_forest_role) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t forest_shape_source = forest_shape_own_source + forest_role_source +
				       forest_add_admit_source + forest_binding_source;
constexpr size_t forest_shape_matched_source =
	forest_shape_own_matched_source + forest_binding_matched_source;

template <class T> constexpr size_t forest_append_own_source =
	sizeof(void *) + 2 * sizeof(T) + sizeof(size_t) + sizeof(uint8_t);
template <class T> constexpr size_t forest_read_own_source =
	3 * sizeof(void *) + sizeof(T) + sizeof(size_t) + sizeof(bool);
constexpr size_t forest_encode_own_source =
	// binding/out/reserve/context/retained5P,catch-refP, role, outer,
	// parameters/base/method_frames/requested/current/retained (7N total),
	// UID range hidden reference/begin/end3P and real copied UID.
	9 * sizeof(void *) + sizeof(shop_trade_recovery_forest_role) + 7 * sizeof(size_t) +
	sizeof(uint64_t) + sizeof(bool);
constexpr size_t forest_encode_own_matched_source =
	// First persistent original parameters+base only. Late method/library
	// allowances are preserved but deliberately unclaimed as credit.
	5 * sizeof(void *) + sizeof(shop_trade_recovery_forest_role) + 2 * sizeof(size_t) +
	sizeof(bool);
constexpr size_t forest_encode_source =
	forest_encode_own_source + forest_add_admit_source + forest_shape_source +
	forest_append_own_source<uint8_t> + forest_append_own_source<uint16_t> +
	forest_append_own_source<uint32_t> + forest_append_own_source<uint64_t> +
	forest_fitting_byte_push_source + forest_fresh_reserve_source<uint8_t> +
	forest_digest_fitting_insert_source + forest_default_source<uint8_t> +
	forest_tail_source<uint8_t> + 2 * sizeof(void *) +
	// The const UID range methods: begin/end receivers2P, pointer ctor2P,
	// deref2P,++2P,==2P+B,two actual base() occurrences4P. Their
	// returned references coexist; hidden iterator objects are own3P above.
	14 * sizeof(void *) + sizeof(bool) +
	// UID size accessor P+N, const digest begin/end each begin/data4P.
	sizeof(void *) + sizeof(size_t) + 8 * sizeof(void *);
constexpr size_t forest_encode_matched_source =
	forest_encode_own_matched_source + forest_shape_matched_source;

constexpr size_t forest_decode_own_source =
	// out/reserve/context/retained4P; cursor/end2P; mutable UID hidden
	// range/begin/end/reference4P; catchP. span by-value is real parameter.
	// outer/frames/base/current/retained5N; role/present2u8/countu16.
	sizeof(std::span<const uint8_t>) + 11 * sizeof(void *) +
	sizeof(shop_trade_recovery_forest_role) + 5 * sizeof(size_t) + 2 * sizeof(uint8_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t forest_decode_own_matched_source =
	// Only literal original frames' named public signature and caller fields.
	// Candidate owner INLINE is not silently renamed Source or subtracted.
	sizeof(std::span<const uint8_t>) + 7 * sizeof(void *) +
	sizeof(shop_trade_recovery_forest_role) + 4 * sizeof(size_t) + 2 * sizeof(uint8_t) +
	sizeof(uint16_t) + sizeof(bool);
constexpr size_t forest_decode_source =
	forest_decode_own_source + forest_add_admit_source + forest_role_source +
	forest_shape_source + forest_read_own_source<uint8_t> + forest_read_own_source<uint16_t> +
	forest_read_own_source<uint32_t> + forest_read_own_source<uint64_t> +
	forest_uid_empty_resize_source +
	// Actual UID capacity observations before/after resize and final result;
	// this typed getter is not the byte encoder's reserve specialization.
	forest_primitive::complete_recipe_vector_capacity_source<uint64_t> +
	// Real plain forest default receiverP, nontrivial move3P,stdmove2P,
	// cleanupP. Digest/scalar/array operations have no implicit trivial body.
	7 * sizeof(void *) + forest_default_source<uint64_t> + forest_tail_source<uint64_t> +
	// Mutable UID range methods14P+B: both actual base() returns coexist.
	14 * sizeof(void *) + sizeof(bool) +
	// span data2P, size/extent2P+2N; mutable digest sizeP+N and begin/data4P;
	// fixed32 raw const-byte copy, no Num==1 or bare builtin callee frame.
	9 * sizeof(void *) + 3 * sizeof(size_t) + forest_digest_copy_source<uint8_t *> +
	// The actual by-value span has an explicitly declared defaulted dtor.
	// Caller lvalue-copy construction remains at the external call site.
	sizeof(void *);
constexpr size_t forest_decode_matched_source =
	forest_decode_own_matched_source + forest_shape_matched_source;
static_assert(forest_shape_source >= forest_shape_matched_source);
static_assert(forest_encode_source >= forest_encode_matched_source);
static_assert(forest_decode_source >= forest_decode_matched_source);
#endif
}

#endif

bool shop_trade_recovery_forest_shape_valid_bounded_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = forest_shape_source;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_shape_valid_bounded_source_supplement_frame_bytes(
	size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = forest_shape_source - forest_shape_matched_source;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_shape_valid_bounded_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = 0;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_encode_bounded_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = forest_encode_source;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_encode_bounded_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = forest_encode_source - forest_encode_matched_source;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_encode_bounded_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = 0;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_decode_bounded_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = forest_decode_source;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_decode_bounded_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = forest_decode_source - forest_decode_matched_source;
	return true;
#else
	return false;
#endif
}

bool shop_trade_recovery_forest_decode_bounded_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !shop_manifest_source_supported())
		return false;
#if __cplusplus == 202002L && defined(__GNUG__) && !defined(__clang__) && defined(__linux__) &&    \
	defined(__x86_64__) && defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 &&              \
	defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI == 1 &&                          \
	!defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&                               \
	!defined(_GLIBCXX_PARALLEL) && !defined(__SANITIZE_ADDRESS__) &&                           \
	!defined(__SANITIZE_THREAD__) && !defined(__SANITIZE_UNDEFINED__) &&                       \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	*output = 0;
	return true;
#else
	return false;
#endif
}
