#include "world/quest_mobile_native_reference.h"
#include "player/player_snapshot_codec.h"
#include "economy/economic_accounting_types.h"

#include <algorithm>
#include <bit>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>

namespace
{
constexpr std::array<uint8_t, 8> reference_magic = { 'Q', 'M', 'N', 'R', 'E', 'F', 0, 0 };

bool nonzero(const critical_operation_id &id)
{
	return std::any_of(id.bytes.begin(), id.bytes.end(),
			   [](uint8_t value) { return value != 0; });
}

template <typename T> void put(uint8_t *bytes, size_t &offset, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t n = 0; n < sizeof(T); ++n)
	{
		bytes[offset++] = static_cast<uint8_t>(bits & 0xff);
		bits >>= 8;
	}
}

template <typename T> T get(const uint8_t *bytes, size_t &offset)
{
	using U = std::make_unsigned_t<T>;
	U bits = 0;
	for (size_t n = 0; n < sizeof(T); ++n)
		bits |= static_cast<U>(bytes[offset++]) << (8 * n);
	return std::bit_cast<T>(bits);
}

bool checksum(std::span<const uint8_t> bytes)
{
	if (bytes.size() < SHA256_DIGEST_LENGTH)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	const size_t sealed = bytes.size() - digest.size();
	return SHA256(bytes.data(), sealed, digest.data()) &&
	       CRYPTO_memcmp(digest.data(), bytes.data() + sealed, digest.size()) == 0;
}

} // namespace

bool quest_mobile_native_reference_valid(const quest_mobile_native_reference &value) noexcept
{
	return value.mobile_instance_id && value.mobile_instance_id != UINT64_MAX &&
	       nonzero(value.birth_operation) && economic_source_event_valid(value.birth_source) &&
	       value.mobile_vnum >= 0 && value.mobile_revision && value.stock_revision &&
	       ((value.provenance == quest_mobile_birth_provenance::reset &&
		 value.reset_zone_vnum >= 0) ||
		(value.provenance == quest_mobile_birth_provenance::spawn &&
		 value.reset_zone_vnum == -1));
}

player_snapshot_codec_result quest_mobile_native_reference_encode(
	const quest_mobile_native_reference &value,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output) noexcept
{
	if (!output || !quest_mobile_native_reference_valid(value))
		return player_snapshot_codec_result::invalid_value;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> candidate = {};
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
	if (economic_source_event_encode(value.birth_source, &source) !=
	    economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	std::copy(reference_magic.begin(), reference_magic.end(), candidate.begin());
	size_t offset = reference_magic.size();
	put<uint16_t>(candidate.data(), offset, QUEST_MOBILE_NATIVE_VERSION);
	put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(value.provenance));
	put<uint8_t>(candidate.data(), offset, 0);
	put<uint32_t>(candidate.data(), offset, candidate.size());
	put<uint64_t>(candidate.data(), offset, value.mobile_instance_id);
	std::copy(value.birth_operation.bytes.begin(), value.birth_operation.bytes.end(),
		  candidate.begin() + offset);
	offset += value.birth_operation.bytes.size();
	std::copy(source.begin(), source.end(), candidate.begin() + offset);
	offset += source.size();
	put<int32_t>(candidate.data(), offset, value.mobile_vnum);
	put<int32_t>(candidate.data(), offset, value.birthplace_vnum);
	put<int32_t>(candidate.data(), offset, value.reset_zone_vnum);
	put<uint64_t>(candidate.data(), offset, value.mobile_revision);
	put<uint64_t>(candidate.data(), offset, value.stock_revision);
	if (offset != candidate.size() - SHA256_DIGEST_LENGTH ||
	    !SHA256(candidate.data(), offset, candidate.data() + offset))
		return player_snapshot_codec_result::invalid_value;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result
quest_mobile_native_reference_decode(std::span<const uint8_t> bytes,
				     quest_mobile_native_reference *output) noexcept
{
	if (!output || bytes.size() != QUEST_MOBILE_NATIVE_REFERENCE_BYTES)
		return player_snapshot_codec_result::invalid_value;
	if (!std::equal(reference_magic.begin(), reference_magic.end(), bytes.begin()) ||
	    !checksum(bytes))
		return player_snapshot_codec_result::invalid_value;
	size_t offset = reference_magic.size();
	if (get<uint16_t>(bytes.data(), offset) != QUEST_MOBILE_NATIVE_VERSION)
		return player_snapshot_codec_result::unsupported_version;
	quest_mobile_native_reference candidate;
	candidate.provenance =
		static_cast<quest_mobile_birth_provenance>(get<uint8_t>(bytes.data(), offset));
	if (get<uint8_t>(bytes.data(), offset) ||
	    get<uint32_t>(bytes.data(), offset) != bytes.size())
		return player_snapshot_codec_result::invalid_value;
	candidate.mobile_instance_id = get<uint64_t>(bytes.data(), offset);
	std::copy_n(bytes.data() + offset, candidate.birth_operation.bytes.size(),
		    candidate.birth_operation.bytes.begin());
	offset += candidate.birth_operation.bytes.size();
	if (economic_source_event_decode(bytes.subspan(offset, ECONOMIC_SOURCE_EVENT_BYTES),
					 &candidate.birth_source) != economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	offset += ECONOMIC_SOURCE_EVENT_BYTES;
	candidate.mobile_vnum = get<int32_t>(bytes.data(), offset);
	candidate.birthplace_vnum = get<int32_t>(bytes.data(), offset);
	candidate.reset_zone_vnum = get<int32_t>(bytes.data(), offset);
	candidate.mobile_revision = get<uint64_t>(bytes.data(), offset);
	candidate.stock_revision = get<uint64_t>(bytes.data(), offset);
	if (!quest_mobile_native_reference_valid(candidate))
		return player_snapshot_codec_result::invalid_value;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

namespace
{
using native_ref_reserve_fn = bool (*)(size_t, void *) noexcept;
bool native_ref_add(size_t &total, size_t bytes) noexcept
{
	if (bytes > SIZE_MAX - total)
		return false;
	total += bytes;
	return true;
}
player_snapshot_codec_result native_ref_admit(size_t outer, size_t request,
					      native_ref_reserve_fn reserve, void *context) noexcept
{
	if (!native_ref_add(outer, request))
		return player_snapshot_codec_result::limit_exceeded;
	if (!reserve || !reserve(outer, context))
		return player_snapshot_codec_result::allocation_failure;
	return player_snapshot_codec_result::ok;
}
bool native_ref_profile() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(SHA_LONG) == 4 &&
	       sizeof(unsigned int) == 4 && sizeof(unsigned long) == 8;
#else
	return false;
#endif
}
// Authenticated OpenSSL 3.0.13 sha512-x86_64.pl, SHA256 SZ=4/rounds=64:
// AVX2 transient reservation (before its add-back), four saved pointers,
// six pushes, maximum 1024-byte alignment loss, caller return and transient
// pushq/red-zone secondary frame pointer. Other dispatched/SHAEXT leaves
// are smaller; this is source-declared leaf storage, not emitted C stack.
constexpr size_t native_ref_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t native_ref_sha_c_small_frames = 16 * sizeof(unsigned int) +
						 12 * sizeof(unsigned int) + sizeof(unsigned int) +
						 sizeof(int) + sizeof(void *);
constexpr size_t native_ref_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						  11 * sizeof(unsigned int) + 2 * sizeof(int) +
						  2 * sizeof(void *);
constexpr size_t native_ref_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t native_ref_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						2 * sizeof(void *) + sizeof(unsigned int) +
						sizeof(size_t) + sizeof(int);
constexpr size_t native_ref_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
					       sizeof(unsigned long) + sizeof(unsigned int) +
					       sizeof(int);
// Real SHA256_Init/Update/Final memcpy/memset call arguments and result;
// OPENSSL_cleanse(buf,len) and the x86_64 leaf's return address. C fallback
// cleanse's actual ptr/len/pointer-result carriers are included as well.
constexpr size_t native_ref_sha_memory_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(void *);
constexpr size_t native_ref_sha_cleanse_frames =
	// mem_clr.c ptr/len and loaded volatile function pointer remain live
	// through its authentic indirect memset leaf; asm fallback is smaller.
	2 * sizeof(void *) + sizeof(size_t) + native_ref_sha_memory_frames;
constexpr size_t native_ref_sha_block_frames =
	// C compression ctx/in/num plus its actual typed locals; assembly term
	// already includes its own real caller return address.
	std::max(native_ref_sha_assembly_frames,
		 2 * sizeof(void *) + sizeof(size_t) +
			 std::max(native_ref_sha_c_small_frames, native_ref_sha_c_normal_frames));
constexpr size_t native_ref_sha_frames =
	std::max(native_ref_sha_init_frames,
		 std::max(native_ref_sha_update_frames, native_ref_sha_final_frames)) +
	std::max(native_ref_sha_block_frames,
		 std::max(native_ref_sha_memory_frames, native_ref_sha_cleanse_frames));
constexpr size_t native_ref_control_frames =
	// admit outer/request/reserve/context/result; add total/bytes/result;
	// profile's bool result. Callback-private frames remain callback-owned.
	3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(player_snapshot_codec_result) +
	2 * sizeof(bool);
constexpr size_t native_ref_copy_frames =
	// copy/copy_move_a/a1/a2/copy_m actual three iterator args and return;
	// miter/niter/wrap, real length/Num and runtime memcpy/memmove args/result.
	5 * (3 * sizeof(void *) + sizeof(void *)) + 2 * (sizeof(void *) + sizeof(void *)) +
	3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) + sizeof(void *) +
	2 * sizeof(void *) + 3 * sizeof(void *) + sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// copy_n, actual n conversion, forward copy_n random access tag.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(size_t) +
	// Array/span begin/end/data/size, _S_ptr and subspan true declarations.
	8 * (2 * sizeof(void *)) + 4 * (sizeof(void *) + sizeof(size_t)) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + sizeof(void *);
constexpr size_t native_ref_equal_frames = 4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) +
					   sizeof(size_t) + 3 * (sizeof(void *) + sizeof(void *)) +
					   2 * sizeof(void *) + sizeof(size_t) + sizeof(int);
constexpr size_t native_ref_valid_frames =
	// nonzero(id), original any_of->none_of->find_if->two __find_if calls:
	// empty closure params/returned wrapper and real RA trip count/tag.
	sizeof(void *) + sizeof(bool) + 3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	2 * (3 * sizeof(void *) + sizeof(char)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) +
	// __pred_iter/_Iter_pred source ctor/operator and lambda(this,byte)/result,
	// real move refs; array pointer begin/end needs no heap.
	6 * sizeof(void *) + 4 * sizeof(char) + 3 * sizeof(bool) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *)) +
	// economic_source_event_valid and actual critical_operation_id_is_zero:
	// references/results, range begin/end/byte and fixed-array query scopes.
	2 * (sizeof(void *) + sizeof(bool)) + 2 * sizeof(void *) + sizeof(uint8_t) +
	4 * (2 * sizeof(void *));
constexpr size_t native_ref_source_encode_frames =
	// Complete original source-event encode result array/kind/byte and
	// input/output pointers/result; nested fixed valid/zero/copy closure.
	sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) + sizeof(uint16_t) +
	sizeof(size_t) + 2 * sizeof(void *) + sizeof(economic_accounting_error) +
	native_ref_valid_frames + native_ref_copy_frames;
constexpr size_t native_ref_source_decode_scalars =
	// Decoder span/event pointer/return; reader integer this, take(count)
	// this/count, integer value/index and typed by-value result; block output
	// this/reference; id this; original validity and bit_cast arg/result.
	sizeof(std::span<const uint8_t>) + sizeof(void *) + sizeof(economic_accounting_error) +
	2 * sizeof(void *) + sizeof(size_t) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint64_t) +
	2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(uint64_t) + native_ref_valid_frames +
	native_ref_copy_frames;
constexpr size_t native_ref_put_get_frames =
	// Original put/get: bytes/offset refs, largest value/bits, n and returned
	// integer/bit_cast input/result. These are fixed, allocation-free helpers.
	4 * sizeof(void *) + 5 * sizeof(uint64_t) + 2 * sizeof(size_t);
constexpr size_t native_ref_memcmp_frames =
	// Pinned cpuid.c fallback: in_a/in_b/len/i/a/b/x + int result. Real
	// x86_64cpuid.pl CRYPTO_memcmp has no pushes/sub/spill or nested call;
	// only its true caller return address is an explicit assembly stack term.
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned char) + sizeof(int) +
	sizeof(void *);
player_snapshot_codec_result native_ref_valid_result(const quest_mobile_native_reference &value,
						     native_ref_reserve_fn reserve, void *context,
						     size_t outer) noexcept
{
	constexpr size_t frames =
		3 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(player_snapshot_codec_result) +
		// Original validator reference/result and all nested leaves.
		sizeof(void *) + sizeof(bool) + native_ref_valid_frames + native_ref_control_frames;
	if (!native_ref_profile())
		return player_snapshot_codec_result::allocation_failure;
	const auto admitted = native_ref_admit(outer, frames, reserve, context);
	if (admitted != player_snapshot_codec_result::ok)
		return admitted;
	return quest_mobile_native_reference_valid(value) ?
		       player_snapshot_codec_result::ok :
		       player_snapshot_codec_result::invalid_value;
}
player_snapshot_codec_result native_ref_hash(const uint8_t *bytes, size_t length, uint8_t *output,
					     native_ref_reserve_fn reserve, void *context,
					     size_t outer) noexcept
{
	if (!bytes || !output)
		return player_snapshot_codec_result::invalid_value;
	if (!native_ref_profile())
		return player_snapshot_codec_result::allocation_failure;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) && defined(__linux__) &&      \
	defined(__x86_64__) && !defined(_WIN32) && defined(OPENSSL_VERSION_MAJOR) &&          \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                       \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                       \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	struct workspace
	{
		SHA256_CTX digest;
		std::array<uint8_t, 32> result{};
		bool hashed = false;
	};
	constexpr size_t frames = sizeof(workspace) + 4 * sizeof(void *) + 2 * sizeof(size_t) +
				  2 * sizeof(player_snapshot_codec_result) + native_ref_sha_frames +
				  native_ref_copy_frames + native_ref_control_frames;
	const auto admitted = native_ref_admit(outer, frames, reserve, context);
	if (admitted != player_snapshot_codec_result::ok)
		return admitted;
	workspace work;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	work.hashed = SHA256_Init(&work.digest) == 1 &&
		      SHA256_Update(&work.digest, bytes, length) == 1 &&
		      SHA256_Final(work.result.data(), &work.digest) == 1;
#pragma GCC diagnostic pop
	if (!work.hashed)
		return player_snapshot_codec_result::invalid_value;
	std::copy(work.result.begin(), work.result.end(), output);
	return player_snapshot_codec_result::ok;
#else
	(void)length;
	(void)reserve;
	(void)context;
	(void)outer;
	return player_snapshot_codec_result::allocation_failure;
#endif
}
player_snapshot_codec_result native_ref_checksum_bounded(std::span<const uint8_t> bytes,
							 native_ref_reserve_fn reserve,
							 void *context, size_t outer) noexcept
{
	if (bytes.size() < SHA256_DIGEST_LENGTH)
		return player_snapshot_codec_result::invalid_value;
	constexpr size_t frames = sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) +
				  3 * sizeof(size_t) + 3 * sizeof(player_snapshot_codec_result) +
				  sizeof(std::array<uint8_t, SHA256_DIGEST_LENGTH>) +
				  native_ref_copy_frames + native_ref_memcmp_frames +
				  native_ref_control_frames;
	size_t base = outer;
	if (!native_ref_add(base, frames))
		return player_snapshot_codec_result::limit_exceeded;
	const auto admitted = native_ref_admit(base, 0, reserve, context);
	if (admitted != player_snapshot_codec_result::ok)
		return admitted;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	const size_t sealed = bytes.size() - digest.size();
	const auto hashed =
		native_ref_hash(bytes.data(), sealed, digest.data(), reserve, context, base);
	if (hashed != player_snapshot_codec_result::ok)
		return hashed;
	return CRYPTO_memcmp(digest.data(), bytes.data() + sealed, digest.size()) == 0 ?
		       player_snapshot_codec_result::ok :
		       player_snapshot_codec_result::invalid_value;
}
} // namespace
bool quest_mobile_native_reference_valid_bounded(const quest_mobile_native_reference &value,
						 bool (*reserve)(size_t, void *) noexcept,
						 void *context, size_t outer) noexcept
{
	// Public bool compatibility is unchanged; typed codecs call the private
	// result helper directly so a pre-callback failure cannot lose its class.
	constexpr size_t frames =
		3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + native_ref_control_frames;
	if (!native_ref_add(outer, frames))
		return false;
	return native_ref_valid_result(value, reserve, context, outer) ==
	       player_snapshot_codec_result::ok;
}

player_snapshot_codec_result quest_mobile_native_reference_encode_bounded(
	const quest_mobile_native_reference &value,
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	constexpr size_t parameters =
		4 * sizeof(void *) + 2 * sizeof(size_t) + 4 * sizeof(player_snapshot_codec_result);
	size_t base = outer;
	if (!native_ref_profile())
		return player_snapshot_codec_result::allocation_failure;
	if (!native_ref_add(base, parameters))
		return player_snapshot_codec_result::limit_exceeded;
	if (!output)
		return player_snapshot_codec_result::invalid_value;
	const auto valid = native_ref_valid_result(value, reserve, context, base);
	if (valid != player_snapshot_codec_result::ok)
		return valid;
	constexpr size_t frames = sizeof(std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES>) +
				  sizeof(std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES>) +
				  sizeof(size_t) + native_ref_copy_frames +
				  native_ref_put_get_frames + native_ref_source_encode_frames +
				  native_ref_control_frames;
	if (!native_ref_add(base, frames))
		return player_snapshot_codec_result::limit_exceeded;
	const auto admitted = native_ref_admit(base, 0, reserve, context);
	if (admitted != player_snapshot_codec_result::ok)
		return admitted;
	std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> candidate = {};
	std::array<uint8_t, ECONOMIC_SOURCE_EVENT_BYTES> source = {};
	if (economic_source_event_encode(value.birth_source, &source) !=
	    economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	std::copy(reference_magic.begin(), reference_magic.end(), candidate.begin());
	size_t offset = reference_magic.size();
	put<uint16_t>(candidate.data(), offset, QUEST_MOBILE_NATIVE_VERSION);
	put<uint8_t>(candidate.data(), offset, static_cast<uint8_t>(value.provenance));
	put<uint8_t>(candidate.data(), offset, 0);
	put<uint32_t>(candidate.data(), offset, candidate.size());
	put<uint64_t>(candidate.data(), offset, value.mobile_instance_id);
	std::copy(value.birth_operation.bytes.begin(), value.birth_operation.bytes.end(),
		  candidate.begin() + offset);
	offset += value.birth_operation.bytes.size();
	std::copy(source.begin(), source.end(), candidate.begin() + offset);
	offset += source.size();
	put<int32_t>(candidate.data(), offset, value.mobile_vnum);
	put<int32_t>(candidate.data(), offset, value.birthplace_vnum);
	put<int32_t>(candidate.data(), offset, value.reset_zone_vnum);
	put<uint64_t>(candidate.data(), offset, value.mobile_revision);
	put<uint64_t>(candidate.data(), offset, value.stock_revision);
	if (offset != candidate.size() - SHA256_DIGEST_LENGTH)
		return player_snapshot_codec_result::invalid_value;
	const auto hashed = native_ref_hash(candidate.data(), offset, candidate.data() + offset,
					    reserve, context, base);
	if (hashed != player_snapshot_codec_result::ok)
		return hashed;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

player_snapshot_codec_result quest_mobile_native_reference_decode_bounded(
	std::span<const uint8_t> bytes, quest_mobile_native_reference *output,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer) noexcept
{
	if (!output || bytes.size() != QUEST_MOBILE_NATIVE_REFERENCE_BYTES)
		return player_snapshot_codec_result::invalid_value;
	constexpr size_t frames = sizeof(std::span<const uint8_t>) + 3 * sizeof(void *) +
				  2 * sizeof(size_t) + 4 * sizeof(player_snapshot_codec_result) +
				  sizeof(quest_mobile_native_reference) + sizeof(size_t) +
				  native_ref_equal_frames + native_ref_copy_frames +
				  native_ref_put_get_frames + native_ref_source_decode_scalars +
				  sizeof(size_t) + native_ref_control_frames;
	size_t base = outer;
	if (!native_ref_profile())
		return player_snapshot_codec_result::allocation_failure;
	if (!native_ref_add(base, frames) ||
	    !native_ref_add(base, economic_source_event_decode_object_bytes()))
		return player_snapshot_codec_result::limit_exceeded;
	const auto admitted = native_ref_admit(base, 0, reserve, context);
	if (admitted != player_snapshot_codec_result::ok)
		return admitted;
	if (!std::equal(reference_magic.begin(), reference_magic.end(), bytes.begin()))
		return player_snapshot_codec_result::invalid_value;
	const auto checked = native_ref_checksum_bounded(bytes, reserve, context, base);
	if (checked != player_snapshot_codec_result::ok)
		return checked;
	size_t offset = reference_magic.size();
	if (get<uint16_t>(bytes.data(), offset) != QUEST_MOBILE_NATIVE_VERSION)
		return player_snapshot_codec_result::unsupported_version;
	quest_mobile_native_reference candidate;
	candidate.provenance =
		static_cast<quest_mobile_birth_provenance>(get<uint8_t>(bytes.data(), offset));
	if (get<uint8_t>(bytes.data(), offset) ||
	    get<uint32_t>(bytes.data(), offset) != bytes.size())
		return player_snapshot_codec_result::invalid_value;
	candidate.mobile_instance_id = get<uint64_t>(bytes.data(), offset);
	std::copy_n(bytes.data() + offset, candidate.birth_operation.bytes.size(),
		    candidate.birth_operation.bytes.begin());
	offset += candidate.birth_operation.bytes.size();
	if (economic_source_event_decode(bytes.subspan(offset, ECONOMIC_SOURCE_EVENT_BYTES),
					 &candidate.birth_source) != economic_accounting_error::ok)
		return player_snapshot_codec_result::invalid_value;
	offset += ECONOMIC_SOURCE_EVENT_BYTES;
	candidate.mobile_vnum = get<int32_t>(bytes.data(), offset);
	candidate.birthplace_vnum = get<int32_t>(bytes.data(), offset);
	candidate.reset_zone_vnum = get<int32_t>(bytes.data(), offset);
	candidate.mobile_revision = get<uint64_t>(bytes.data(), offset);
	candidate.stock_revision = get<uint64_t>(bytes.data(), offset);
	const auto valid = native_ref_valid_result(candidate, reserve, context, base);
	if (valid != player_snapshot_codec_result::ok)
		return valid;
	*output = candidate;
	return player_snapshot_codec_result::ok;
}

// Pure source contracts for the actual fixed bounded reference codecs.
// Complete source is a transient preentry contract. Existing bounded helpers
// retain their original source/inline admissions; only the explicitly uncovered
// supplement remains in caller outer. No codec or admission algorithm changes.
namespace
{
bool native_reference_source_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                  \
	defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&                           \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0 &&                           \
	defined(OPENSSL_VERSION_PATCH) && OPENSSL_VERSION_PATCH == 13 &&                          \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(SHA_LONG) == 4 &&
	       sizeof(unsigned int) == 4 && sizeof(unsigned long) == 8 && sizeof(bool) == 1;
#else
	return false;
#endif
}
// Actual native_ref_valid_result formals/return/admitted and frames constant;
// original validator and its real any_of/source-event/zero descendants.
constexpr size_t native_reference_validation_source =
	3 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(player_snapshot_codec_result) +
	sizeof(void *) + sizeof(bool) + native_ref_valid_frames + native_ref_control_frames;
// Actual native_ref_hash bytes/output/reserve/context,length/outer, frames
// constant, admitted/result. Its local workspace is original-admission-owned.
constexpr size_t native_reference_hash_source =
	4 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(player_snapshot_codec_result) +
	native_ref_sha_frames + native_ref_copy_frames + native_ref_control_frames;
// Source-event encoder's scalar/source calls; its real result array is admitted
// by native_ref_source_encode_frames and is deliberately not SOURCE twice.
constexpr size_t native_reference_event_encode_source =
	sizeof(uint16_t) + sizeof(size_t) + 2 * sizeof(void *) + sizeof(economic_accounting_error) +
	native_ref_valid_frames + native_ref_copy_frames;
constexpr size_t native_reference_encode_source =
	// Actual public signature/base/parameters/frames/offset, valid/admitted/
	// hashed/return. Candidate and source arrays are original frame-owned.
	4 * sizeof(void *) + 5 * sizeof(size_t) + 4 * sizeof(player_snapshot_codec_result) +
	native_reference_validation_source + native_ref_copy_frames + native_ref_put_get_frames +
	native_reference_event_encode_source + native_ref_control_frames +
	native_reference_hash_source;
constexpr size_t native_reference_checksum_source =
	// span,reserve/context,outer/base/frames/sealed,admitted/hashed/return.
	sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) + 4 * sizeof(size_t) +
	3 * sizeof(player_snapshot_codec_result) + native_ref_copy_frames +
	native_ref_memcmp_frames + native_ref_control_frames + native_reference_hash_source;
constexpr size_t native_reference_decode_source =
	// Actual public span/output/reserve/context,outer/base/frames/offset,
	// economic-source object getter return, admitted/checked/valid/return.
	sizeof(std::span<const uint8_t>) + 3 * sizeof(void *) + 5 * sizeof(size_t) +
	4 * sizeof(player_snapshot_codec_result) + native_ref_equal_frames +
	native_ref_copy_frames + native_ref_put_get_frames + native_ref_source_decode_scalars +
	native_ref_control_frames + native_reference_checksum_source +
	native_reference_validation_source;
// Each term names an actual lexical constexpr size_t whose carrier is absent
// from that helper's original parameter/control count. Encode: public
// parameters+frames, valid_result frames, hash frames. Decode: public frames,
// checksum frames, hash frames, valid_result frames. No numerical baseline or
// current-observer subtraction is involved; original controller formulas stay.
constexpr size_t native_reference_encode_supplement = 4 * sizeof(size_t);
constexpr size_t native_reference_decode_supplement = 4 * sizeof(size_t);
constexpr size_t native_reference_valid_source = 3 * sizeof(void *) + 2 * sizeof(size_t) +
						 sizeof(bool) + native_ref_control_frames +
						 native_reference_validation_source;
}

bool quest_mobile_native_reference_valid_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = native_reference_valid_source;
	return true;
}
bool quest_mobile_native_reference_valid_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = sizeof(size_t); // Genuine valid_result's frames constant only.
	return true;
}
bool quest_mobile_native_reference_valid_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = 0; // No aggregate is constructed by the bool wrapper.
	return true;
}
bool quest_mobile_native_reference_encode_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = native_reference_encode_source;
	return true;
}
bool quest_mobile_native_reference_decode_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = native_reference_decode_source;
	return true;
}
bool quest_mobile_native_reference_encode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = native_reference_encode_supplement;
	return true;
}
bool quest_mobile_native_reference_decode_source_supplement_frame_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	*output = native_reference_decode_supplement;
	return true;
}
bool quest_mobile_native_reference_encode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	// Before its first real admission encode has no local aggregate. Its real
	// candidate/source arrays and hash workspace are constructed afterwards.
	*output = 0;
	return true;
}
bool quest_mobile_native_reference_decode_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !native_reference_source_policy())
		return false;
	// Decode likewise constructs candidate/checksum/hash only after admission.
	*output = 0;
	return true;
}
