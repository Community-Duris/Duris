#include "economy/native_mobile_birth_result.h"

#include <algorithm>
#include <new>
#include <openssl/sha.h>

namespace
{
bool valid(const native_mobile_birth_result &result) noexcept
{
	const auto nonzero = [](const economic_digest &digest) {
		return std::any_of(digest.begin(), digest.end(),
				   [](uint8_t byte) { return byte != 0; });
	};
	return result.mobile_instance_id && result.mobile_instance_id != UINT64_MAX &&
	       result.wallet_mapping_id && result.mobile_revision == 1 &&
	       result.stock_revision == 1 && result.cash_revision == 1 &&
	       result.item_owner_revision == 1 && nonzero(result.image_digest) &&
	       nonzero(result.plan_digest);
}

void put(uint8_t *output, uint64_t value) noexcept
{
	for (size_t index = 0; index != 8; ++index)
		output[index] = static_cast<uint8_t>(value >> (8 * index));
}

uint64_t get(const uint8_t *input) noexcept
{
	uint64_t value = 0;
	for (size_t index = 0; index != 8; ++index)
		value |= static_cast<uint64_t>(input[index]) << (8 * index);
	return value;
}

bool equal(const native_mobile_birth_result &left, const native_mobile_birth_result &right) noexcept
{
	return left.mobile_instance_id == right.mobile_instance_id &&
	       left.mobile_revision == right.mobile_revision &&
	       left.stock_revision == right.stock_revision &&
	       left.cash_revision == right.cash_revision &&
	       left.wallet_mapping_id == right.wallet_mapping_id &&
	       left.item_owner_revision == right.item_owner_revision &&
	       left.image_digest == right.image_digest && left.plan_digest == right.plan_digest;
}
}

bool native_mobile_birth_result_encode(
	const native_mobile_birth_result &result,
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> *output) noexcept
{
	if (!output || !valid(result))
		return false;
	std::array<uint8_t, NATIVE_MOBILE_BIRTH_RESULT_BYTES> candidate{};
	candidate[0] = 'M';
	candidate[1] = 'B';
	candidate[2] = 'R';
	candidate[3] = '1';
	candidate[4] = 1; // Little-endian version1; remaining header bytes are zero.
	const uint64_t fields[] = { result.mobile_instance_id, result.mobile_revision,
				    result.stock_revision,     result.cash_revision,
				    result.wallet_mapping_id,  result.item_owner_revision };
	for (size_t index = 0; index != 6; ++index)
		put(candidate.data() + 8 + 8 * index, fields[index]);
	std::copy(result.image_digest.begin(), result.image_digest.end(), candidate.begin() + 56);
	std::copy(result.plan_digest.begin(), result.plan_digest.end(), candidate.begin() + 88);
	*output = candidate;
	return true;
}

bool native_mobile_birth_result_decode(std::span<const uint8_t> input,
				       native_mobile_birth_result *output) noexcept
{
	if (!output || input.size() != NATIVE_MOBILE_BIRTH_RESULT_BYTES || input[0] != 'M' ||
	    input[1] != 'B' || input[2] != 'R' || input[3] != '1' || input[4] != 1 || input[5] ||
	    input[6] || input[7])
		return false;
	native_mobile_birth_result candidate{};
	candidate.mobile_instance_id = get(input.data() + 8);
	candidate.mobile_revision = get(input.data() + 16);
	candidate.stock_revision = get(input.data() + 24);
	candidate.cash_revision = get(input.data() + 32);
	candidate.wallet_mapping_id = get(input.data() + 40);
	candidate.item_owner_revision = get(input.data() + 48);
	std::copy_n(input.begin() + 56, candidate.image_digest.size(),
		    candidate.image_digest.begin());
	std::copy_n(input.begin() + 88, candidate.plan_digest.size(),
		    candidate.plan_digest.begin());
	if (!valid(candidate))
		return false;
	*output = candidate;
	return true;
}

economic_accounting_error native_mobile_birth_result_build(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, native_mobile_birth_result *output) noexcept
{
	using error = economic_accounting_error;
	if (!output)
		return error::invalid_identity;
	try
	{
		quest_mobile_native_image image;
		auto status = native_mobile_birth_command_decode(command, &image);
		if (status != error::ok)
			return status;
		economic_accounting_plan expected;
		status = native_mobile_birth_accounting_compile(command, wallet, &expected);
		if (status != error::ok)
			return status;
		economic_digest expected_digest{};
		status = economic_plan_digest(expected, &expected_digest);
		if (status != error::ok)
			return status;
		native_mobile_birth_result candidate{};
		status = economic_plan_digest(plan, &candidate.plan_digest);
		if (status != error::ok)
			return status;
		if (candidate.plan_digest != expected_digest)
			return error::payload_conflict;
		candidate.mobile_instance_id = image.reference.mobile_instance_id;
		candidate.mobile_revision = image.reference.mobile_revision;
		candidate.stock_revision = image.reference.stock_revision;
		candidate.cash_revision = image.cash->revision;
		candidate.wallet_mapping_id = wallet.authority_id;
		candidate.item_owner_revision = 1;
		if (!SHA256(command.payload.data(), command.payload.size(),
			    candidate.image_digest.data()) ||
		    !valid(candidate))
			return error::corrupt_evidence;
		*output = candidate;
		return error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return error::capacity;
	}
	catch (...)
	{
		return error::corrupt_evidence;
	}
}

bool native_mobile_birth_result_matches(const critical_command &command,
					const economic_account_key &wallet,
					const economic_accounting_plan &plan,
					const native_mobile_birth_result &result) noexcept
{
	native_mobile_birth_result expected{};
	return native_mobile_birth_result_build(command, wallet, plan, &expected) ==
		       economic_accounting_error::ok &&
	       equal(expected, result);
}

#include <type_traits>
#include <iterator>

namespace
{
using result_reserve_fn = bool (*)(size_t, void *) noexcept;
using result_error = economic_accounting_error;

bool result_profile_supported() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 &&    \
	!defined(__clang__) && __cplusplus == 202002L && defined(_GLIBCXX_RELEASE) &&      \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&                       \
	_GLIBCXX_USE_CXX11_ABI == 1 && !defined(_GLIBCXX_DEBUG) &&                         \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                    \
	!(defined(_GLIBCXX_SANITIZE_STD_ALLOCATOR) && defined(_GLIBCXX_SANITIZE_VECTOR) && \
	  _GLIBCXX_SANITIZE_STD_ALLOCATOR && _GLIBCXX_SANITIZE_VECTOR) &&                  \
	!defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256) &&              \
	defined(OPENSSL_VERSION_MAJOR) && OPENSSL_VERSION_MAJOR == 3 &&                    \
	defined(OPENSSL_VERSION_MINOR) && OPENSSL_VERSION_MINOR == 0
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(unsigned int) == 4 && sizeof(unsigned long) == 8 && sizeof(SHA_LONG) == 4;
#else
	return false;
#endif
}

bool result_add(size_t &total, size_t bytes) noexcept
{
	if (bytes > SIZE_MAX - total)
		return false;
	total += bytes;
	return true;
}

bool result_admit(size_t base, size_t bytes, result_reserve_fn reserve, void *context) noexcept
{
	return result_add(base, bytes) && reserve && reserve(base, context);
}

struct result_build_workspace
{
	quest_mobile_native_image image;
	economic_accounting_plan expected;
	economic_digest expected_digest{};
	native_mobile_birth_result candidate{};
	size_t image_heap = 0, plan_heap = 0;
	size_t child_query = 0, child_source = 0, child_inline = 0;
};

struct result_build_budget
{
	result_build_workspace &work;
	size_t base;
	bool live(size_t *output) const noexcept
	{
		if (!output)
			return false;
		size_t value = base;
		if (!result_add(value, work.image_heap) || !result_add(value, work.plan_heap))
			return false;
		*output = value;
		return true;
	}
};

// Genuine selected child interfaces. Each preflight admits its pure query
// before accessing source/inline metadata, then transiently admits the child
// supplement. The child itself owns the same supplement through its execution.
bool result_command_preflight(result_build_workspace &work, size_t current,
			      result_reserve_fn reserve, void *context) noexcept
{
	constexpr size_t query_frames = native_mobile_birth_command_source_query_frame_bytes();
	work.child_query = query_frames;
	if (!result_admit(current, work.child_query, reserve, context) ||
	    !native_mobile_birth_command_general_decode_source_frame_bytes(&work.child_source) ||
	    !native_mobile_birth_command_general_decode_initial_inline_bytes(&work.child_inline))
		return false;
	return result_add(current, work.child_source) &&
	       result_admit(current, work.child_inline, reserve, context);
}

bool result_compile_preflight(result_build_workspace &work, size_t current,
			      result_reserve_fn reserve, void *context) noexcept
{
	work.child_query = native_mobile_birth_accounting_compile_profile_query_frame_bytes();
	if (!result_admit(current, work.child_query, reserve, context) ||
	    !native_mobile_birth_accounting_compile_own_source_frame_bytes(&work.child_source) ||
	    !native_mobile_birth_accounting_compile_initial_inline_bytes(&work.child_inline))
		return false;
	return result_add(current, work.child_source) &&
	       result_admit(current, work.child_inline, reserve, context);
}

bool result_digest_preflight(result_build_workspace &work, size_t current,
			     result_reserve_fn reserve, void *context) noexcept
{
	work.child_query = economic_plan_bounded_profile_query_frame_bytes();
	if (!result_admit(current, work.child_query, reserve, context) ||
	    !economic_plan_digest_source_frame_bytes(&work.child_source) ||
	    !economic_plan_digest_initial_inline_bytes(&work.child_inline))
		return false;
	return result_add(current, work.child_source) &&
	       result_admit(current, work.child_inline, reserve, context);
}

// These are direct owner/source profiles, separate from objects and heap.
// The named selected command/compiler/plan-digest children preadmit their own
// complete source and private objects. Their live returned payload belongs here.
constexpr size_t result_control_frames =
	// result_admit parameters/result, result_add parameters/result, pure profile
	// result, live receiver/output/value/result; callback-private source is foreign.
	2 * sizeof(size_t) + sizeof(result_reserve_fn) + sizeof(void *) + sizeof(bool) +
	sizeof(size_t *) + sizeof(size_t) + sizeof(bool) + sizeof(bool) +
	sizeof(result_build_budget *) + sizeof(size_t *) + sizeof(size_t) + sizeof(bool);

constexpr size_t result_child_preflight_frames =
	// The three real preflight functions have identical signatures and no
	// unnamed heap or local objects. Child getter source is separately admitted
	// from its authentic declared query before each call; the complete command
	// query is required constexpr and its named local uses the same single N.
	// Other scalar outputs reside in workspace fields; branches are sequential.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(size_t);

constexpr size_t result_valid_frames =
	// Original valid(result), nonzero(digest), empty local closure, any_of ->
	// none_of -> find_if -> both __find_if levels, RA trip count/tag, wrapper
	// predicate construction/call and actual byte/results; array begin/end.
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(bool) +
	3 * (2 * sizeof(void *) + sizeof(char) + sizeof(bool)) +
	2 * (3 * sizeof(void *) + sizeof(char)) + sizeof(std::ptrdiff_t) +
	sizeof(std::random_access_iterator_tag) + 6 * sizeof(void *) + 4 * sizeof(char) +
	3 * sizeof(bool) + sizeof(uint8_t) + 4 * (2 * sizeof(void *));

constexpr size_t result_equal_frames =
	// Original equal(left,right) and array operator==; std::equal/_equal_aux/
	// _equal_aux1/_equal byte specialization, iterator wrappers and memcmp.
	4 * sizeof(void *) + 2 * sizeof(bool) + 4 * (3 * sizeof(void *) + sizeof(bool)) +
	sizeof(size_t) + sizeof(bool) + 3 * (sizeof(void *) + sizeof(void *)) + 2 * sizeof(void *) +
	sizeof(size_t) + sizeof(int) + 3 * (2 * sizeof(void *));

constexpr size_t result_copy_frames =
	// Original copy_n -> copy_n(RA) -> copy/copy_move_a/a1/a2/copy_m plus
	// iterator normalization/wrapping and the actual memcpy/memmove leaf.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(std::random_access_iterator_tag) +
	2 * sizeof(size_t) + 5 * (3 * sizeof(void *) + sizeof(void *)) +
	2 * (sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	2 * sizeof(void *) + sizeof(void *) + 2 * sizeof(void *) + 3 * sizeof(void *) +
	sizeof(size_t) + sizeof(std::ptrdiff_t) +
	// Span input size/data/begin and both actual array begin/size accessors.
	4 * (sizeof(void *) + sizeof(size_t)) + 8 * (2 * sizeof(void *));

template <class T> constexpr size_t result_vector_cleanup_frames() noexcept
{
	using V = std::vector<T>;
	using A = std::allocator<T>;
	constexpr size_t destruction = sizeof(V *) + sizeof(V *) + sizeof(A *) + 2 * sizeof(T *) +
				       sizeof(A *) + 2 * sizeof(T *) + 2 * sizeof(T *) +
				       sizeof(bool);
	constexpr size_t element = std::is_trivially_destructible_v<T> ? 0 : 4 * sizeof(T *);
	constexpr size_t deallocation =
		sizeof(void *) + sizeof(V *) + sizeof(T *) + sizeof(size_t) + sizeof(A *) +
		sizeof(T *) + sizeof(size_t) + sizeof(A *) + sizeof(T *) + sizeof(size_t) +
		sizeof(A *) + sizeof(T *) + sizeof(size_t) + sizeof(void *) + sizeof(size_t) +
		sizeof(bool) + sizeof(A *);
	return destruction + element + deallocation;
}

template <class T> constexpr size_t result_vector_lifetime_frames() noexcept
{
	return sizeof(std::vector<T> *) + 2 * sizeof(void *) + 2 * sizeof(std::allocator<T> *) +
	       sizeof(void *) + result_vector_cleanup_frames<T>();
}

constexpr size_t result_string_cleanup_frames =
	// basic_string dtor/dispose/is_local plus actual retained _M_data left
	// comparison result during local_data -> pointer_traits -> addressof.
	3 * sizeof(void *) + 11 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
	// destroy(n), get_allocator, traits/deallocate, allocator/new_allocator
	// and sized operator delete, including genuine constant-evaluation result.
	2 * sizeof(void *) + sizeof(size_t) + 2 * sizeof(void *) +
	3 * (2 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) + sizeof(size_t) + sizeof(bool);

constexpr size_t result_workspace_lifetime_frames =
	// Generated workspace/image/plan ctor+dtor receiver paths; the seven direct
	// vectors really default-construct. Nested row vectors only destruct here:
	// their copied construction belongs to the selected decode child.
	2 * sizeof(result_build_workspace *) + 2 * sizeof(quest_mobile_native_image *) +
	2 * sizeof(economic_accounting_plan *) +
	result_vector_lifetime_frames<player_item_snapshot>() +
	result_vector_lifetime_frames<economic_account_effect>() +
	result_vector_lifetime_frames<economic_coin_posting>() +
	result_vector_lifetime_frames<economic_child_link>() +
	2 * result_vector_lifetime_frames<economic_item_snapshot>() +
	result_vector_lifetime_frames<economic_item_event>() +
	// Real item and extra-description generated destruction; complete string
	// member paths are sequential actual calls, never multiplied by row count.
	sizeof(player_item_snapshot *) + 4 * result_string_cleanup_frames +
	result_vector_cleanup_frames<player_item_dynamic_affect_snapshot>() +
	result_vector_cleanup_frames<player_item_extra_description_snapshot>() +
	sizeof(player_item_extra_description_snapshot *) + 2 * result_string_cleanup_frames +
	result_vector_cleanup_frames<int32_t>() +
	// Both trivial optionals: optional, base, base_impl, payload, payload_base,
	// storage, _Empty_byte and Enable_copy_move are actual default-constructed
	// subobjects. Their generated cleanup receiver paths are priced too; stored
	// engaged flags are inside the real optional object, not separate live bytes.
	2 * (8 * sizeof(void *) + 8 * sizeof(void *)) +
	// Actual metadata and operation-metadata generated ctor/dtor receivers.
	4 * sizeof(void *) +
	// Generated result/digest array aggregate construction and copy receivers.
	6 * sizeof(void *);

constexpr size_t result_cash_access_frames =
	// optional::operator-> -> __addressof plus base_impl::_M_get and
	// payload_base::_M_get. Assertions are excluded by the genuine profile;
	// receivers and returned pointer/reference carriers remain real.
	4 * (sizeof(void *) + sizeof(void *));

// SHA leaf constants are copied byte-for-byte (renamed only) from the reviewed
// full native image fixed-context provider and authenticated OpenSSL sources.
// The actual definitions are appended by the private builder, not a margin.
constexpr size_t result_fixed_sha_assembly_frames =
	2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) + 2 * sizeof(void *);
constexpr size_t result_fixed_sha_c_small_frames =
	16 * sizeof(unsigned int) + 12 * sizeof(unsigned int) + sizeof(unsigned int) + sizeof(int) +
	sizeof(void *);
constexpr size_t result_fixed_sha_c_normal_frames = 16 * sizeof(unsigned int) +
						    11 * sizeof(unsigned int) + 2 * sizeof(int) +
						    2 * sizeof(void *);
constexpr size_t result_fixed_sha_init_frames = sizeof(void *) + sizeof(int);
constexpr size_t result_fixed_sha_update_frames = 2 * sizeof(void *) + sizeof(size_t) +
						  2 * sizeof(void *) + sizeof(unsigned int) +
						  sizeof(size_t) + sizeof(int);
constexpr size_t result_fixed_sha_final_frames = 3 * sizeof(void *) + sizeof(size_t) +
						 sizeof(unsigned long) + sizeof(unsigned int) +
						 sizeof(int);
// Real SHA256_Init/Update/Final memcpy/memset call arguments and result;
// OPENSSL_cleanse(buf,len) and the x86_64 leaf's return address. C fallback
// cleanse's actual ptr/len/pointer-result carriers are included as well.
constexpr size_t result_fixed_sha_memory_frames =
	2 * sizeof(void *) + sizeof(size_t) + sizeof(int) + sizeof(void *);
constexpr size_t result_fixed_sha_cleanse_frames =
	// mem_clr.c ptr/len and loaded volatile function pointer remain live
	// through its authentic indirect memset leaf; asm fallback is smaller.
	2 * sizeof(void *) + sizeof(size_t) + result_fixed_sha_memory_frames;
constexpr size_t result_fixed_sha_block_frames =
	// C compression ctx/in/num plus its actual typed locals; assembly term
	// already includes its own real caller return address.
	std::max(result_fixed_sha_assembly_frames,
		 2 * sizeof(void *) + sizeof(size_t) +
			 std::max(result_fixed_sha_c_small_frames,
				  result_fixed_sha_c_normal_frames));
constexpr size_t result_fixed_sha_frames =
	std::max(result_fixed_sha_init_frames,
		 std::max(result_fixed_sha_update_frames, result_fixed_sha_final_frames)) +
	std::max(result_fixed_sha_block_frames,
		 std::max(result_fixed_sha_memory_frames, result_fixed_sha_cleanse_frames));

#if !defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256)
constexpr size_t result_sha_context_bytes = sizeof(SHA256_CTX);
#else
constexpr size_t result_sha_context_bytes = 0;
#endif

constexpr size_t result_entry_frames =
	// build's eight argument carriers, status/current and try/catch results;
	// matches' nine args, status/result and generated expected-result lifetime;
	// decode's by-value span/five other args, status and original get loop;
	// hash helper's span/output plus original SHA calls/array vector accessors.
	6 * sizeof(void *) + sizeof(result_reserve_fn) + sizeof(size_t) + sizeof(result_error) +
	sizeof(size_t) + sizeof(bool) + 7 * sizeof(void *) + sizeof(result_reserve_fn) +
	sizeof(size_t) + sizeof(result_error) + sizeof(bool) +
	2 * sizeof(native_mobile_birth_result *) + sizeof(std::span<const uint8_t>) +
	3 * sizeof(void *) + sizeof(result_reserve_fn) + sizeof(size_t) + sizeof(result_error) +
	// Original decode/get/candidate-generated assignment source receivers. Its
	// fixed candidate is an inline object, accounted separately below.
	sizeof(std::span<const uint8_t>) + 2 * sizeof(void *) + 2 * sizeof(size_t) +
	2 * sizeof(uint64_t) + sizeof(bool) + sizeof(const std::vector<uint8_t> *) +
	sizeof(economic_digest *) + sizeof(bool) + 4 * (sizeof(void *) + sizeof(size_t)) +
	4 * (2 * sizeof(void *));

constexpr size_t result_owner_source_frames =
	result_control_frames + result_entry_frames + result_child_preflight_frames +
	result_workspace_lifetime_frames +
	std::max(result_valid_frames,
		 std::max(result_equal_frames,
			  std::max(result_copy_frames,
				   std::max(result_cash_access_frames, result_fixed_sha_frames))));

#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 && \
	!defined(__clang__) && !defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
bool result_payload_hash(const std::vector<uint8_t> &bytes, economic_digest *output) noexcept
{
	SHA256_CTX state;
	return SHA256_Init(&state) == 1 && SHA256_Update(&state, bytes.data(), bytes.size()) == 1 &&
	       SHA256_Final(output->data(), &state) == 1;
}
#pragma GCC diagnostic pop
#endif
}

bool native_mobile_birth_result_source_frame_bytes(size_t *output) noexcept
{
	if (!output || !result_profile_supported())
		return false;
	*output = result_owner_source_frames;
	return true;
}

bool native_mobile_birth_result_initial_inline_bytes(size_t *output) noexcept
{
	if (!output || !result_profile_supported())
		return false;
	// matches' fixed expected stays alive while build's real workspace/budget
	// and temporary SHA context live. Decode's fixed candidate is smaller.
	*output = sizeof(native_mobile_birth_result) + sizeof(result_build_workspace) +
		  sizeof(result_build_budget) + result_sha_context_bytes;
	return true;
}

economic_accounting_error native_mobile_birth_result_decode_bounded(
	std::span<const uint8_t> input, native_mobile_birth_result *output,
	result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!output)
		return result_error::corrupt_evidence;
	if (!result_profile_supported() ||
	    !result_admit(outer, result_owner_source_frames + sizeof(native_mobile_birth_result),
			  reserve, context))
		return result_error::capacity;
	// Complete original fixed120 header/field/digest validity and strong output.
	return native_mobile_birth_result_decode(input, output) ? result_error::ok :
								  result_error::corrupt_evidence;
}

economic_accounting_error native_mobile_birth_result_build_bounded(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, native_mobile_birth_result *output,
	result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!output)
		return result_error::invalid_identity;
	if (!result_profile_supported())
		return result_error::capacity;
#if defined(__linux__) && defined(__x86_64__) && defined(__GNUC__) && __GNUC__ == 13 && \
	!defined(__clang__) && !defined(OPENSSL_NO_DEPRECATED_3_0) && !defined(OPENSSL_NO_SHA256)
	size_t base = outer;
	if (!result_add(base, result_owner_source_frames) ||
	    !result_add(base, sizeof(result_build_workspace) + sizeof(result_build_budget) +
				      sizeof(SHA256_CTX)) ||
	    !reserve || !reserve(base, context))
		return result_error::capacity;
	try
	{
		result_build_workspace work;
		result_build_budget budget{ work, base };
		size_t current = base;
		if (!result_command_preflight(work, current, reserve, context))
			return result_error::capacity;
		auto status = native_mobile_birth_command_decode_bounded(
			command, &work.image, reserve, context, current, &work.image_heap);
		if (status != result_error::ok)
			return status;
		if (!budget.live(&current) ||
		    !result_compile_preflight(work, current, reserve, context))
			return result_error::capacity;
		status = native_mobile_birth_accounting_compile_bounded(command, wallet,
									&work.expected, reserve,
									context, current,
									&work.plan_heap);
		if (status != result_error::ok)
			return status;
		if (!budget.live(&current) ||
		    !result_digest_preflight(work, current, reserve, context))
			return result_error::capacity;
		status = economic_plan_digest_bounded(work.expected, &work.expected_digest, reserve,
						      context, current);
		if (status != result_error::ok)
			return status;
		if (!result_digest_preflight(work, current, reserve, context))
			return result_error::capacity;
		status = economic_plan_digest_bounded(plan, &work.candidate.plan_digest, reserve,
						      context, current);
		if (status != result_error::ok)
			return status;
		if (work.candidate.plan_digest != work.expected_digest)
			return result_error::payload_conflict;
		work.candidate.mobile_instance_id = work.image.reference.mobile_instance_id;
		work.candidate.mobile_revision = work.image.reference.mobile_revision;
		work.candidate.stock_revision = work.image.reference.stock_revision;
		work.candidate.cash_revision = work.image.cash->revision;
		work.candidate.wallet_mapping_id = wallet.authority_id;
		work.candidate.item_owner_revision = 1;
		if (!reserve(current, context))
			return result_error::capacity;
		if (!result_payload_hash(command.payload, &work.candidate.image_digest) ||
		    !valid(work.candidate))
			return result_error::corrupt_evidence;
		*output = work.candidate;
		return result_error::ok;
	}
	catch (const std::bad_alloc &)
	{
		return result_error::capacity;
	}
	catch (...)
	{
		return result_error::corrupt_evidence;
	}
#else
	(void)command;
	(void)wallet;
	(void)plan;
	(void)reserve;
	(void)context;
	(void)outer;
	return result_error::capacity;
#endif
}

economic_accounting_error native_mobile_birth_result_matches_bounded(
	const critical_command &command, const economic_account_key &wallet,
	const economic_accounting_plan &plan, const native_mobile_birth_result &result,
	bool *matches, result_reserve_fn reserve, void *context, size_t outer) noexcept
{
	if (!matches)
		return result_error::invalid_identity;
	if (!result_profile_supported() ||
	    !result_admit(outer, result_owner_source_frames + sizeof(native_mobile_birth_result),
			  reserve, context))
		return result_error::capacity;
	native_mobile_birth_result expected{};
	if (!result_add(outer, sizeof(native_mobile_birth_result)))
		return result_error::capacity;
	const auto status = native_mobile_birth_result_build_bounded(
		command, wallet, plan, &expected, reserve, context, outer);
	if (status != result_error::ok)
		return status;
	*matches = equal(expected, result);
	return result_error::ok;
}
