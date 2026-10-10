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
