#include <cerrno>
#include <openssl/sha.h>
#include "world/native_mobile_birth_procedure.h"

#include "core/prototypes.h"
#include "core/structs.h"
#include "economy/shop.h"
#include "mob/studioproc.h"
#include "world/events.h"
#include "world/specs.prototypes.h"

#include <algorithm>
#include <cstddef>
#include <limits>
#include <memory>
#include <openssl/evp.h>

#if defined(__linux__) && defined(__ELF__) && defined(__x86_64__)
#include <elf.h>
#include <link.h>
#endif

extern P_index mob_index;
extern int top_of_mobt;

namespace
{
class procedure_hash
{
	std::unique_ptr<EVP_MD_CTX, decltype(&EVP_MD_CTX_free)> context{ EVP_MD_CTX_new(),
									 EVP_MD_CTX_free };
	bool healthy = context && EVP_DigestInit_ex(context.get(), EVP_sha256(), nullptr) == 1;

    public:
	bool bytes(const void *data, size_t length) noexcept
	{
		healthy = healthy && (!length || data) &&
			  EVP_DigestUpdate(context.get(), data, length) == 1;
		return healthy;
	}
	bool integer(uint64_t value) noexcept
	{
		std::array<uint8_t, 8> wire{};
		for (size_t i = 0; i < wire.size(); ++i)
			wire[i] = static_cast<uint8_t>(value >> (i * 8));
		return bytes(wire.data(), wire.size());
	}
	bool finish(native_mobile_birth_procedure_digest *output) noexcept
	{
		native_mobile_birth_procedure_digest candidate{};
		unsigned int size = 0;
		if (!healthy || EVP_DigestFinal_ex(context.get(), candidate.data(), &size) != 1 ||
		    size != candidate.size())
			return false;
		*output = candidate;
		return true;
	}
};

#if defined(__linux__) && defined(__ELF__) && defined(__x86_64__)
struct main_entry_lookup
{
	uintptr_t address = 0;
	uint64_t relative = 0;
	bool found = false;
};
int main_entry_visit(dl_phdr_info *info, size_t, void *opaque) noexcept
{
	if (!info || (info->dlpi_name && *info->dlpi_name))
		return 0;
	auto &lookup = *static_cast<main_entry_lookup *>(opaque);
	const uintptr_t bias = static_cast<uintptr_t>(info->dlpi_addr);
	if (!info->dlpi_phdr)
		return 1;
	for (size_t i = 0; i < info->dlpi_phnum; ++i)
	{
		const auto &segment = info->dlpi_phdr[i];
		if (segment.p_type != PT_LOAD || !(segment.p_flags & PF_X) ||
		    segment.p_vaddr > std::numeric_limits<uintptr_t>::max() - bias)
			continue;
		const uintptr_t start = bias + static_cast<uintptr_t>(segment.p_vaddr);
		if (lookup.address < start || lookup.address - start >= segment.p_memsz ||
		    lookup.address < bias)
			continue;
		lookup.relative = static_cast<uint64_t>(lookup.address - bias);
		lookup.found = lookup.relative != 0;
		break;
	}
	return 1;
}
#endif

bool main_entry(native_mobile_birth_procedure_function function, uint64_t *relative) noexcept
{
	if (!function || !relative)
		return false;
#if defined(__linux__) && defined(__ELF__) && defined(__x86_64__)
	// GNU/Linux x86_64 function-entry representation; never convert a saved
	// integer back to a function pointer. The loader's actual main PT_LOAD
	// ranges reject shared-library entries under a main-executable witness.
	main_entry_lookup lookup{ reinterpret_cast<uintptr_t>(function), 0, false };
	dl_iterate_phdr(main_entry_visit, &lookup);
	if (!lookup.found)
		return false;
	*relative = lookup.relative;
	return true;
#else
	return false;
#endif
}

bool procedure_chain(procedure_hash &hash, int32_t vnum,
		     native_mobile_birth_procedure_function function, unsigned int seen)
{
	uint64_t entry = 0;
	if (function && !main_entry(function, &entry))
		return false;
	if (!hash.integer(function ? 1 : 0) || !hash.integer(entry))
		return false;
	if (!function)
		return hash.integer(0);
	if (function == studioproc_mob)
	{
		if (seen & 1U)
			return false; // Actual recursive dispatcher chain, not a saved address.
		native_mobile_birth_procedure_digest definition{};
		mob_proc_type predecessor = nullptr;
		return studioproc_native_mobile_birth_definition(vnum, &definition, &predecessor) &&
		       hash.integer(1) && hash.bytes(definition.data(), definition.size()) &&
		       procedure_chain(hash, vnum, predecessor, seen | 1U);
	}
	if (function == shop_keeper)
	{
		if (seen & 2U)
			return false;
		std::vector<shop_native_mobile_birth_predecessor> predecessors;
		if (!shop_native_mobile_birth_predecessors(vnum, &predecessors) ||
		    !hash.integer(2) || !hash.integer(predecessors.size()))
			return false;
		for (const auto &selected : predecessors)
		{
			if (!hash.integer(static_cast<uint64_t>(
				    static_cast<int64_t>(selected.shop_slot))) ||
			    !procedure_chain(hash, vnum, selected.function, seen | 2U))
				return false;
		}
		return true;
	}
	return hash.integer(0);
}
} // namespace

bool native_mobile_birth_procedure_capture(
	int32_t mobile_vnum, const native_mobile_birth_procedure_digest &actual_build_digest,
	native_mobile_birth_procedure_digest *output) noexcept
{
	if (!output || mobile_vnum <= 0 || !nevent_is_game_thread() || !mob_index ||
	    std::all_of(actual_build_digest.begin(), actual_build_digest.end(),
			[](uint8_t byte) { return byte == 0; }))
		return false;
	try
	{
		const int rnum = real_mobile(mobile_vnum);
		if (rnum < 0 || rnum > top_of_mobt || mob_index[rnum].virtual_number != mobile_vnum)
			return false;
		const auto main = mob_index[rnum].func.mob;
		const auto quest = mob_index[rnum].qst_func;
		procedure_hash hash;
		constexpr char domain[] = "NMP1";
		native_mobile_birth_procedure_digest candidate{};
		if (!hash.bytes(domain, sizeof(domain) - 1) ||
		    !hash.bytes(actual_build_digest.data(), actual_build_digest.size()) ||
		    !hash.integer(static_cast<uint64_t>(mobile_vnum)) ||
		    !procedure_chain(hash, mobile_vnum, main, 0) ||
		    !procedure_chain(hash, mobile_vnum, quest, 0) || !hash.finish(&candidate))
			return false;
		*output = candidate;
		return true;
	}
	catch (...)
	{
		return false;
	}
}

bool native_mobile_birth_procedure_matches(
	int32_t mobile_vnum, const native_mobile_birth_procedure_digest &actual_build_digest,
	const native_mobile_birth_procedure_digest &expected) noexcept
{
	native_mobile_birth_procedure_digest actual{};
	return native_mobile_birth_procedure_capture(mobile_vnum, actual_build_digest, &actual) &&
	       actual == expected;
}

extern shop_data *shop_index;
extern int number_of_shops;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
namespace
{

// Exact authenticated OpenSSL3.0.13 source inventory shared with the original
// native reference/artifact companions: AVX2 block workspace/alignment and
// C small/normal leaves, with reached Init/Update/Final source carriers.
constexpr size_t native_procedure_sha_assembly =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t native_procedure_sha_c_small = 16 * sizeof(unsigned) + 12 * sizeof(unsigned) +
						sizeof(unsigned) + sizeof(int) + sizeof(void *);
constexpr size_t native_procedure_sha_c_normal =
	16 * sizeof(unsigned) + 11 * sizeof(unsigned) + 2 * sizeof(int) + 2 * sizeof(void *);
constexpr size_t native_procedure_sha_update =
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned) + sizeof(int);
constexpr size_t native_procedure_sha_final = 3 * sizeof(void *) + sizeof(size_t) +
					      sizeof(unsigned long) + sizeof(unsigned) +
					      sizeof(int);
constexpr size_t native_procedure_sha_leaf_frames =
	std::max(native_procedure_sha_assembly,
		 std::max(native_procedure_sha_c_small, native_procedure_sha_c_normal)) +
	std::max(sizeof(void *) + sizeof(int),
		 std::max(native_procedure_sha_update, native_procedure_sha_final));

#if defined(__linux__) && defined(__ELF__) && defined(__x86_64__)
constexpr size_t native_procedure_entry_frames = sizeof(main_entry_lookup);
#else
constexpr size_t native_procedure_entry_frames = 0;
#endif
class procedure_bounded_hash
{
	SHA256_CTX context{};
	bool healthy = SHA256_Init(&context) == 1;

    public:
	bool bytes(const void *data, size_t length) noexcept
	{
		healthy = healthy && (!length || data) &&
			  SHA256_Update(&context, data, length) == 1;
		return healthy;
	}
	bool integer(uint64_t value) noexcept
	{
		std::array<uint8_t, 8> wire{};
		for (size_t i = 0; i < wire.size(); ++i)
			wire[i] = static_cast<uint8_t>(value >> (i * 8));
		return bytes(wire.data(), wire.size());
	}
	bool finish(native_mobile_birth_procedure_digest *output) noexcept
	{
		native_mobile_birth_procedure_digest candidate{};
		if (!healthy || SHA256_Final(candidate.data(), &context) != 1)
			return false;
		*output = candidate;
		return true;
	}
};

bool procedure_chain_bounded(procedure_bounded_hash &hash, int32_t vnum,
			     native_mobile_birth_procedure_function function, unsigned int seen,
			     bool (*reserve)(size_t, void *) noexcept, void *context,
			     size_t outer_live) noexcept
{
	constexpr size_t frames = native_procedure_sha_leaf_frames + 16 * sizeof(void *) +
				  8 * sizeof(size_t) + 6 * sizeof(uint64_t) + 8 * sizeof(int) +
				  6 * sizeof(bool) + sizeof(native_mobile_birth_procedure_digest) +
				  native_procedure_entry_frames;
	if (!reserve || frames > SIZE_MAX - outer_live || !reserve(outer_live + frames, context))
		return false;
	const size_t live = outer_live + frames;
	uint64_t entry = 0;
	if (function && !main_entry(function, &entry))
		return false;
	if (!hash.integer(function ? 1 : 0) || !hash.integer(entry))
		return false;
	if (!function)
		return hash.integer(0);
	if (function == studioproc_mob)
	{
		if (seen & 1U)
			return false; // Actual recursive dispatcher chain, not a saved address.
		native_mobile_birth_procedure_digest definition{};
		mob_proc_type predecessor = nullptr;
		return studioproc_native_mobile_birth_definition_bounded(
			       vnum, &definition, &predecessor, reserve, context, live) &&
		       hash.integer(1) && hash.bytes(definition.data(), definition.size()) &&
		       procedure_chain_bounded(hash, vnum, predecessor, seen | 1U, reserve, context,
					       live);
	}
	if (function == shop_keeper)
	{
		if (seen & 2U)
			return false;
		// Complete original shop-provider selection, streamed from the actual
		// records in original order. No temporary predecessor vector is needed.
		if (vnum <= 0 || !nevent_is_game_thread() || !mob_index || !shop_index ||
		    number_of_shops <= 0)
			return false;
		const int keeper = real_mobile(vnum);
		if (keeper < 0 || mob_index[keeper].virtual_number != vnum)
			return false;
		size_t count = 0;
		for (int shop = 0; shop < number_of_shops; ++shop)
			if (SHOP_KEEPER(shop) == keeper)
				++count;
		if (!count || !hash.integer(2) || !hash.integer(count))
			return false;
		for (int shop = 0; shop < number_of_shops; ++shop)
		{
			if (SHOP_KEEPER(shop) != keeper)
				continue;
			if (!hash.integer(static_cast<uint64_t>(static_cast<int64_t>(shop))) ||
			    !procedure_chain_bounded(hash, vnum, SHOP_FUNC(shop), seen | 2U,
						     reserve, context, live))
				return false;
		}
		return true;
	}
	return hash.integer(0);
}
}

#pragma GCC diagnostic pop
#endif

bool native_mobile_birth_procedure_capture_bounded(
	int32_t mobile_vnum, const native_mobile_birth_procedure_digest &actual_build_digest,
	native_mobile_birth_procedure_digest *output, bool (*reserve)(size_t, void *) noexcept,
	void *context, size_t outer_live) noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(SHA_LONG) != 4 ||
	    sizeof(unsigned int) != 4 || sizeof(unsigned long) != 8)
	{
		errno = ENOTSUP;
		return false;
	}
	constexpr size_t frames =
		native_procedure_sha_leaf_frames + sizeof(procedure_bounded_hash) +
		sizeof(native_mobile_birth_procedure_digest) + sizeof(std::array<uint8_t, 8>) +
		12 * sizeof(void *) + 8 * sizeof(size_t) + 4 * sizeof(int) + 4 * sizeof(bool) +
		sizeof("NMP1");
	if (!reserve || frames > SIZE_MAX - outer_live || !reserve(outer_live + frames, context))
		return false;
	const size_t live = outer_live + frames;
	if (!output || mobile_vnum <= 0 || !nevent_is_game_thread() || !mob_index ||
	    std::all_of(actual_build_digest.begin(), actual_build_digest.end(),
			[](uint8_t byte) { return byte == 0; }))
		return false;
	try
	{
		const int rnum = real_mobile(mobile_vnum);
		if (rnum < 0 || rnum > top_of_mobt || mob_index[rnum].virtual_number != mobile_vnum)
			return false;
		const auto main = mob_index[rnum].func.mob;
		const auto quest = mob_index[rnum].qst_func;
		procedure_bounded_hash hash;
		constexpr char domain[] = "NMP1";
		native_mobile_birth_procedure_digest candidate{};
		if (!hash.bytes(domain, sizeof(domain) - 1) ||
		    !hash.bytes(actual_build_digest.data(), actual_build_digest.size()) ||
		    !hash.integer(static_cast<uint64_t>(mobile_vnum)) ||
		    !procedure_chain_bounded(hash, mobile_vnum, main, 0, reserve, context, live) ||
		    !procedure_chain_bounded(hash, mobile_vnum, quest, 0, reserve, context, live) ||
		    !hash.finish(&candidate))
			return false;
		*output = candidate;
		return true;
	}
	catch (...)
	{
		return false;
	}
#else
	(void)mobile_vnum;
	(void)actual_build_digest;
	(void)output;
	(void)reserve;
	(void)context;
	(void)outer_live;
	errno = ENOTSUP;
	return false;
#endif
}
