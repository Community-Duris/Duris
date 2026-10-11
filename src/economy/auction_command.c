#include "economy/auction_command.h"
#include "economy/auction_native_command_context.h"

#include <algorithm>
#include <cstring>
#include <limits>
#include <new>

namespace
{
template <typename T> void append_le(std::vector<uint8_t> *output, T value)
{
	using unsigned_type = std::make_unsigned_t<T>;
	const unsigned_type encoded = static_cast<unsigned_type>(value);
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		output->push_back(static_cast<uint8_t>(encoded >> (byte * 8)));
}

template <typename T> bool read_le(const uint8_t **cursor, const uint8_t *end, T *value)
{
	if (!cursor || !*cursor || !value || static_cast<size_t>(end - *cursor) < sizeof(T))
		return false;
	using unsigned_type = std::make_unsigned_t<T>;
	unsigned_type decoded = 0;
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		decoded |= static_cast<unsigned_type>((*cursor)[byte]) << (byte * 8);
	*cursor += sizeof(T);
	*value = static_cast<T>(decoded);
	return true;
}

template <size_t Size>
bool append_string(std::vector<uint8_t> *output, const std::array<char, Size> &value)
{
	const size_t length = strnlen(value.data(), value.size());
	if (length >= value.size() || length > UINT16_MAX)
		return false;
	append_le<uint16_t>(output, static_cast<uint16_t>(length));
	output->insert(output->end(), value.begin(), value.begin() + length);
	return true;
}

template <size_t Size>
bool read_string(const uint8_t **cursor, const uint8_t *end, std::array<char, Size> *value)
{
	uint16_t length = 0;
	if (!read_le(cursor, end, &length) || length >= value->size() ||
	    static_cast<size_t>(end - *cursor) < length)
		return false;
	value->fill(0);
	memcpy(value->data(), *cursor, length);
	*cursor += length;
	return true;
}

uint64_t listing_key(const critical_operation_id &operation_id)
{
	uint64_t key = 0;
	for (size_t index = 0; index < sizeof(key); ++index)
		key |= static_cast<uint64_t>(operation_id.bytes[index]) << (index * 8);
	return key ? key : 1;
}

bool valid_payload(const auction_command_payload &payload)
{
	if (payload.action <= auction_action::unknown || payload.action > auction_action::remove ||
	    payload.item_count > AUCTION_COMMAND_MAX_ITEMS ||
	    payload.object_blob_size >= payload.object_blob.size())
		return false;
	if ((payload.action == auction_action::list ||
	     payload.action == auction_action::claim_item) &&
	    !payload.item_count)
		return false;
	// finalize and remove are authoritative closures: they stage items back to the
	// seller (or to the recorded winner) and never touch an actor wallet, so both
	// run actor-less from expiry and from the DurisWeb removal path.
	if (payload.action != auction_action::finalize &&
	    payload.action != auction_action::remove && !payload.actor_pid)
		return false;
	if (payload.action != auction_action::list && !payload.auction_id &&
	    payload.action != auction_action::claim_money)
		return false;
	if (payload.actor_pid &&
	    (!payload.account_name[0] ||
	     strnlen(payload.account_name.data(), payload.account_name.size()) >=
		     payload.account_name.size()))
		return false;
	for (size_t index = 0; index < payload.item_count; ++index)
		if (!payload.items[index].item_uid || payload.items[index].vnum < 0)
			return false;
	return true;
}

bool matching_fences(const critical_command &left, const critical_command &right)
{
	if (left.keys.size() != right.keys.size() ||
	    left.expected_revisions.size() != right.expected_revisions.size())
		return false;
	for (size_t index = 0; index < left.keys.size(); ++index)
		if (left.keys[index].type != right.keys[index].type ||
		    left.keys[index].id != right.keys[index].id)
			return false;
	for (size_t index = 0; index < left.expected_revisions.size(); ++index)
		if (left.expected_revisions[index].key.type !=
			    right.expected_revisions[index].key.type ||
		    left.expected_revisions[index].key.id !=
			    right.expected_revisions[index].key.id ||
		    left.expected_revisions[index].revision !=
			    right.expected_revisions[index].revision)
			return false;
	return true;
}
} // namespace

bool auction_command_encode_payload(const auction_command_payload &payload,
				    std::vector<uint8_t> *encoded)
{
	if (!encoded || !valid_payload(payload))
		return false;
	try
	{
		encoded->clear();
		encoded->reserve(512 + payload.object_blob_size +
				 strnlen(payload.object_info.data(), payload.object_info.size()));
		append_le<uint8_t>(encoded, static_cast<uint8_t>(payload.action));
		append_le<uint32_t>(encoded, payload.actor_pid);
		append_le<uint32_t>(encoded, payload.auction_id);
		append_le<uint8_t>(encoded, payload.racewar);
		append_le<uint64_t>(encoded, payload.expected_wallet_revision);
		append_le<uint64_t>(encoded, payload.expected_bank_revision);
		append_le<int64_t>(encoded, payload.value);
		append_le<int64_t>(encoded, payload.start_price);
		append_le<int64_t>(encoded, payload.buy_price);
		append_le<int64_t>(encoded, payload.listing_fee);
		append_le<uint32_t>(encoded, payload.closing_fee_basis_points);
		append_le<uint32_t>(encoded, payload.bid_extension_seconds);
		append_le<uint64_t>(encoded, payload.end_time);
		append_le<uint16_t>(encoded, payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			append_le<uint64_t>(encoded, payload.items[index].item_uid);
			append_le<uint64_t>(encoded, payload.items[index].expected_item_revision);
			append_le<int32_t>(encoded, payload.items[index].vnum);
		}
		if (!append_string(encoded, payload.account_name) ||
		    !append_string(encoded, payload.actor_name) ||
		    !append_string(encoded, payload.object_short) ||
		    !append_string(encoded, payload.id_keywords) ||
		    !append_string(encoded, payload.object_info))
			return false;
		append_le<uint32_t>(encoded, payload.object_blob_size);
		encoded->insert(encoded->end(), payload.object_blob.begin(),
				payload.object_blob.begin() + payload.object_blob_size);
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	return encoded->size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

// Structural native projection only. Every original root/player/account fence
// remains exact; additional keys must be paired item fences. SQL authenticates
// their selected-node identity and revisions before admission/execution.
static bool matching_native_item_fences(const critical_command &expected,
					const critical_command &actual)
{
	if (actual.keys.size() < expected.keys.size() ||
	    !std::is_sorted(actual.keys.begin(), actual.keys.end(), critical_entity_key_less) ||
	    std::adjacent_find(actual.keys.begin(), actual.keys.end(), critical_entity_key_equal) !=
		    actual.keys.end() ||
	    actual.expected_revisions.size() !=
		    expected.expected_revisions.size() + actual.keys.size() - expected.keys.size())
		return false;
	for (const auto &key : expected.keys)
		if (!std::binary_search(actual.keys.begin(), actual.keys.end(), key,
					critical_entity_key_less))
			return false;
	for (const auto &key : actual.keys)
	{
		const auto known = std::binary_search(expected.keys.begin(), expected.keys.end(),
						      key, critical_entity_key_less);
		if (!known &&
		    (key.type != critical_entity_type::item || !key.id || key.id == UINT64_MAX))
			return false;
		const auto original = std::find_if(
			expected.expected_revisions.begin(), expected.expected_revisions.end(),
			[&](const auto &r) { return critical_entity_key_equal(r.key, key); });
		const auto count = std::count_if(actual.expected_revisions.begin(),
						 actual.expected_revisions.end(), [&](const auto &r)
						 { return critical_entity_key_equal(r.key, key); });
		if (original == expected.expected_revisions.end())
		{
			if (known ? count != 0 : count != 1)
				return false;
		}
		else if (count != 1)
			return false;
		if (count)
		{
			const auto found =
				std::find_if(actual.expected_revisions.begin(),
					     actual.expected_revisions.end(), [&](const auto &r)
					     { return critical_entity_key_equal(r.key, key); });
			if (original != expected.expected_revisions.end() ?
				    found->revision != original->revision :
				    (!found->revision || found->revision == UINT64_MAX))
				return false;
		}
	}
	return true;
}

static bool decode_original_payload(const critical_command &command,
				    auction_command_payload *payload, bool native_item_fences)
{
	if (!payload || command.type != critical_command_type::auction ||
	    command.payload_version != AUCTION_COMMAND_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &payload->actor_pid) ||
	    !read_le(&cursor, end, &payload->auction_id) ||
	    !read_le(&cursor, end, &payload->racewar) ||
	    !read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_le(&cursor, end, &payload->value) ||
	    !read_le(&cursor, end, &payload->start_price) ||
	    !read_le(&cursor, end, &payload->buy_price) ||
	    !read_le(&cursor, end, &payload->listing_fee) ||
	    !read_le(&cursor, end, &payload->closing_fee_basis_points) ||
	    !read_le(&cursor, end, &payload->bid_extension_seconds) ||
	    !read_le(&cursor, end, &payload->end_time) ||
	    !read_le(&cursor, end, &payload->item_count) ||
	    payload->item_count > payload->items.size())
		return false;
	payload->action = static_cast<auction_action>(action);
	for (size_t index = 0; index < payload->item_count; ++index)
		if (!read_le(&cursor, end, &payload->items[index].item_uid) ||
		    !read_le(&cursor, end, &payload->items[index].expected_item_revision) ||
		    !read_le(&cursor, end, &payload->items[index].vnum))
			return false;
	if (!read_string(&cursor, end, &payload->account_name) ||
	    !read_string(&cursor, end, &payload->actor_name) ||
	    !read_string(&cursor, end, &payload->object_short) ||
	    !read_string(&cursor, end, &payload->id_keywords) ||
	    !read_string(&cursor, end, &payload->object_info) ||
	    !read_le(&cursor, end, &payload->object_blob_size) ||
	    payload->object_blob_size > payload->object_blob.size() ||
	    static_cast<size_t>(end - cursor) != payload->object_blob_size)
		return false;
	memcpy(payload->object_blob.data(), cursor, payload->object_blob_size);
	if (!valid_payload(*payload))
		return false;
	critical_command expected = {};
	if (!auction_command_build(&expected, command.operation_id, *payload, command.source_site,
				   command.deadline_class) ||
	    !(native_item_fences ? matching_native_item_fences(expected, command) :
				   matching_fences(expected, command)))
		return false;
	return true;
}

bool auction_command_decode_payload(const critical_command &command,
				    auction_command_payload *payload)
{
	// Structural native decode grants no legacy execution or publication authority.
	if (payload && command.type == critical_command_type::auction &&
	    command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
	{
		auction_native_command_context context;
		if (auction_native_command_decode(command, &context) !=
		    economic_accounting_error::ok)
			return false;
		*payload = context.payload;
		return true;
	}
	return decode_original_payload(command, payload, false);
}

bool auction_command_decode_native_base(const critical_command &command,
					std::span<const uint8_t> base,
					auction_command_payload *payload)
{
	if (!payload || command.type != critical_command_type::auction ||
	    command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
	    (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
	     command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION))
		return false;
	critical_command original = command;
	original.payload_version = AUCTION_COMMAND_PAYLOAD_VERSION;
	original.payload.assign(base.begin(), base.end());
	auction_command_payload parsed{};
	if (!decode_original_payload(original, &parsed, true))
		return false;
	if (parsed.action == auction_action::claim_money &&
	    !matching_fences(original,
			     [&]()
			     {
				     critical_command expected{};
				     auction_command_build(&expected, command.operation_id, parsed,
							   command.source_site,
							   command.deadline_class);
				     return expected;
			     }()))
		return false;
	*payload = parsed;
	return true;
}

bool auction_command_encode_result(const auction_command_result &result,
				   std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> *encoded)
{
	if (!encoded || result.item_count > result.item_uids.size())
		return false;
	encoded->fill(0);
	std::vector<uint8_t> bytes;
	try
	{
		append_le<uint8_t>(&bytes, static_cast<uint8_t>(result.action));
		append_le<uint8_t>(&bytes, static_cast<uint8_t>(result.event_type));
		append_le<uint32_t>(&bytes, result.auction_id);
		append_le<uint32_t>(&bytes, result.status);
		append_le<uint32_t>(&bytes, result.seller_pid);
		append_le<uint32_t>(&bytes, result.winner_pid);
		append_le<uint32_t>(&bytes, result.previous_bidder_pid);
		append_le<int64_t>(&bytes, result.final_price);
		append_le<int64_t>(&bytes, result.wallet_value_delta);
		for (int64_t value : result.wallet.amount)
			append_le<int64_t>(&bytes, value);
		for (int64_t value : result.bank.amount)
			append_le<int64_t>(&bytes, value);
		append_le<uint64_t>(&bytes, result.wallet_revision);
		append_le<uint64_t>(&bytes, result.bank_revision);
		append_le<uint64_t>(&bytes, result.auction_revision);
		append_le<uint64_t>(&bytes, result.player_owner_revision);
		append_le<uint64_t>(&bytes, result.auction_owner_revision);
		append_le<uint16_t>(&bytes, result.item_count);
		for (size_t index = 0; index < result.item_count; ++index)
		{
			append_le<uint64_t>(&bytes, result.item_uids[index]);
			append_le<uint64_t>(&bytes, result.item_revisions[index]);
		}
		append_le<int64_t>(&bytes, result.claim_credit_used);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (bytes.size() > encoded->size())
		return false;
	std::copy(bytes.begin(), bytes.end(), encoded->begin());
	return true;
}

bool auction_command_decode_result(const uint8_t *encoded, size_t size,
				   auction_command_result *result)
{
	if (!encoded || !result || size != AUCTION_RESULT_PAYLOAD_BYTES)
		return false;
	*result = {};
	const uint8_t *cursor = encoded;
	const uint8_t *end = encoded + size;
	uint8_t action = 0, event = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &event) ||
	    !read_le(&cursor, end, &result->auction_id) ||
	    !read_le(&cursor, end, &result->status) ||
	    !read_le(&cursor, end, &result->seller_pid) ||
	    !read_le(&cursor, end, &result->winner_pid) ||
	    !read_le(&cursor, end, &result->previous_bidder_pid) ||
	    !read_le(&cursor, end, &result->final_price) ||
	    !read_le(&cursor, end, &result->wallet_value_delta))
		return false;
	result->action = static_cast<auction_action>(action);
	result->event_type = static_cast<auction_event_type>(event);
	for (int64_t &value : result->wallet.amount)
		if (!read_le(&cursor, end, &value))
			return false;
	for (int64_t &value : result->bank.amount)
		if (!read_le(&cursor, end, &value))
			return false;
	if (!read_le(&cursor, end, &result->wallet_revision) ||
	    !read_le(&cursor, end, &result->bank_revision) ||
	    !read_le(&cursor, end, &result->auction_revision) ||
	    !read_le(&cursor, end, &result->player_owner_revision) ||
	    !read_le(&cursor, end, &result->auction_owner_revision) ||
	    !read_le(&cursor, end, &result->item_count) ||
	    result->item_count > result->item_uids.size())
		return false;
	for (size_t index = 0; index < result->item_count; ++index)
		if (!read_le(&cursor, end, &result->item_uids[index]) ||
		    !read_le(&cursor, end, &result->item_revisions[index]))
			return false;
	return read_le(&cursor, end, &result->claim_credit_used) &&
	       result->claim_credit_used >= 0 && result->action > auction_action::unknown &&
	       result->action <= auction_action::remove;
}

bool auction_command_build(critical_command *command, critical_operation_id operation_id,
			   const auction_command_payload &payload, critical_source_site source_site,
			   critical_deadline_class deadline_class)
{
	if (!command || !valid_payload(payload))
		return false;
	critical_entity_key account_key = {};
	std::vector<critical_entity_key> keys;
	std::vector<critical_expected_revision> revisions;
	try
	{
		if (payload.actor_pid)
		{
			const critical_entity_key player = { critical_entity_type::player,
							     payload.actor_pid };
			if (!currency_account_key(payload.account_name.data(), payload.racewar,
						  &account_key))
				return false;
			keys.push_back(player);
			keys.push_back(account_key);
			revisions.push_back({ player, payload.expected_wallet_revision });
			revisions.push_back({ account_key, payload.expected_bank_revision });
		}
		keys.push_back(
			{ critical_entity_type::auction,
			  payload.auction_id ? payload.auction_id : listing_key(operation_id) });
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const critical_entity_key item = { critical_entity_type::item,
							   payload.items[index].item_uid };
			keys.push_back(item);
			revisions.push_back({ item, payload.items[index].expected_item_revision });
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::auction,
		     .payload_version = AUCTION_COMMAND_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = std::move(keys),
		     .expected_revisions = std::move(revisions),
		     .payload = {} };
	if (!auction_command_encode_payload(payload, &command->payload))
		return false;
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const critical_expected_revision &left,
		     const critical_expected_revision &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return true;
}

#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)

#include <stdexcept>
namespace
{
// This block is genuine append/forward-copy/vector machinery shared with the
// five auction accounting companions. Full source lineage is recorded by the
// owner packet; the owning command profiles add their real typed carriers.
constexpr size_t auction_codec_allocator_frames =
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
constexpr size_t auction_codec_copy_frames =
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
constexpr size_t auction_codec_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t auction_codec_default_frames =
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
constexpr size_t auction_codec_vector_frames =
	auction_codec_allocator_frames + auction_codec_copy_frames + auction_codec_relocate_frames +
	auction_codec_default_frames +
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
constexpr size_t auction_codec_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + auction_codec_allocator_frames;
constexpr size_t auction_codec_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<uint8_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + auction_codec_vector_frames;

// Genuine additional selected typed library scopes beside the vector's
// reserve/forward-insert profile. The owning inline DTOs remain in the actual
// enclosing object sizes, rather than a fabricated encoded-envelope baseline.
static_assert(std::is_trivially_copyable_v<economic_source_event>);
static_assert(std::is_trivially_destructible_v<economic_source_event>);
static_assert(std::is_trivially_copyable_v<auction_command_payload>);
static_assert(std::is_trivially_copyable_v<economic_account_key>);
constexpr size_t auction_codec_optional_frames =
	// Actual metadata/frozen/admission default/generated move/copy member
	// functions: this/source refs; optional/_Optional_base/_payload/_Storage
	// default constructors and trivial storage destructor this carriers.
	6 * sizeof(void *) + 5 * sizeof(void *) + sizeof(void *) +
	// source_event assignment operator=(T&&): this/u/ref-result; real
	// is_engaged/get/construct wrappers, payload _M_construct and forward.
	3 * sizeof(void *) + (sizeof(void *) + sizeof(bool)) + 2 * (2 * sizeof(void *)) +
	2 * sizeof(void *) + 2 * sizeof(void *) +
	// __addressof -> _Construct -> forward -> placement new -> trivial
	// economic_source_event generated move this/source. No extra DTO copy.
	2 * sizeof(void *) + 2 * sizeof(void *) + 2 * sizeof(void *) + sizeof(void *) +
	sizeof(size_t) + sizeof(void *) + 2 * sizeof(void *) +
	// Actual optional operator bool/operator->/base get/payload get and
	// addressof parameter and reference/pointer/bool result carriers.
	2 * (sizeof(void *) + sizeof(bool)) + 3 * (2 * sizeof(void *));
constexpr size_t auction_codec_equal_frames =
	// array/vector operator== actual lhs/rhs and returned bool; genuine
	// container size/begin/end and array_traits::_S_ptr pointer returns.
	2 * sizeof(void *) + sizeof(bool) + 2 * (sizeof(void *) + sizeof(size_t)) +
	6 * (2 * sizeof(void *)) +
	// equal/__equal_aux/__equal_aux1/__equal<true>::equal each three
	// iterator arguments and returned bool; __simple and __len locals;
	// niter_base calls and __memcmp's genuine pointers/length/int result.
	4 * (3 * sizeof(void *) + sizeof(bool)) + sizeof(bool) + sizeof(size_t) +
	3 * (2 * sizeof(void *)) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int) +
	// Actual normal_iterator copied argument/ctor/base source carriers.
	4 * (2 * sizeof(void *));
constexpr size_t auction_codec_copy_n_frames =
	// Original copy_n(count literal16) owns first/count/result/__n2/result,
	// __size_to_integer(int), iterator_category and __copy_n<RA> tag.
	3 * sizeof(void *) + 2 * sizeof(int) + 2 * sizeof(int) + sizeof(void *) +
	sizeof(std::random_access_iterator_tag) + 3 * sizeof(void *) + sizeof(int) +
	sizeof(std::random_access_iterator_tag);
constexpr size_t auction_codec_scalar_source_frames =
	// Original read lambda (facts-reference capture/this, offset,width,
	// byte index,value/result); typed read_number alternatives; span data,
	// index,size/constructor parameters and returned pointer/reference.
	2 * sizeof(void *) + 3 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	sizeof(std::span<const uint8_t>) + 2 * sizeof(size_t) + 2 * sizeof(uint64_t) +
	4 * (sizeof(void *) + sizeof(size_t) + sizeof(void *)) +
	// Actual account/empty/key_valid/kind_valid helper params/results,
	// operation_id_equal two refs/result and zero's byte range loop.
	3 * sizeof(void *) + sizeof(economic_account_kind) + sizeof(uint64_t) + sizeof(bool) +
	2 * (sizeof(void *) + sizeof(bool)) + sizeof(economic_account_kind) + sizeof(bool) +
	2 * sizeof(void *) + sizeof(bool) + 4 * sizeof(void *) + sizeof(uint8_t) + sizeof(bool) +
	// Original append_u64/u32/u16 and native_fact_append<T> pointer/value/
	// index/byte result lifetimes, plus initializer-list begin/end/size.
	4 * (sizeof(void *) + sizeof(uint64_t) + sizeof(size_t) + sizeof(uint8_t)) +
	3 * (sizeof(void *) + sizeof(void *)) + sizeof(std::initializer_list<uint64_t>) +
	// Original metadata assignment generated function this/source and the
	// returned source_for event temporary, optional typed path above.
	2 * sizeof(void *) + sizeof(economic_source_event) + auction_codec_optional_frames +
	auction_codec_equal_frames + auction_codec_copy_n_frames;
bool auction_codec_add(size_t &total, size_t value) noexcept
{
	if (value > SIZE_MAX - total)
		return false;
	total += value;
	return true;
}

struct auction_command_refusal
{
};
struct auction_command_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, frames;
	const critical_command *command = nullptr;
	const std::vector<critical_entity_key> *keys = nullptr;
	const std::vector<critical_expected_revision> *revisions = nullptr;
	const std::vector<uint8_t> *output = nullptr;
	size_t excluded_caller_heap = 0;
	bool denied = false;
	static bool forward(size_t amount, void *opaque) noexcept
	{
		auto &b = *static_cast<auction_command_budget *>(opaque);
		if (b.denied || !b.reserve || !b.reserve(amount, b.context))
		{
			b.denied = true;
			return false;
		}
		return true;
	}
	bool prefix(size_t &bytes, size_t extra = 0) noexcept
	{
		if (outer < excluded_caller_heap)
		{
			denied = true;
			return false;
		}
		bytes = outer - excluded_caller_heap;
		size_t heap = 0;
		if (!auction_codec_add(bytes, sizeof(*this)) || !auction_codec_add(bytes, frames) ||
		    !auction_codec_add(bytes, extra) ||
		    (command && (!critical_command_current_heap_bytes(*command, &heap) ||
				 !auction_codec_add(bytes, heap))) ||
		    (keys &&
		     (keys->capacity() > SIZE_MAX / sizeof(critical_entity_key) ||
		      !auction_codec_add(bytes, keys->capacity() * sizeof(critical_entity_key)))) ||
		    (revisions &&
		     (revisions->capacity() > SIZE_MAX / sizeof(critical_expected_revision) ||
		      !auction_codec_add(bytes, revisions->capacity() *
							sizeof(critical_expected_revision)))) ||
		    (output && !auction_codec_add(bytes, output->capacity())))
		{
			denied = true;
			return false;
		}
		return true;
	}
	bool peak(size_t extra = 0) noexcept
	{
		size_t bytes = 0;
		if (!prefix(bytes, extra) || !forward(bytes, this))
		{
			denied = true;
			return false;
		}
		return true;
	}
	template <class T> void growth(std::vector<T> &v, size_t count)
	{
		if (count > v.max_size() - v.size())
			throw std::length_error("auction command");
		size_t request = 0;
		if (v.size() + count > v.capacity())
		{
			const size_t growth = std::max(v.size(), count);
			const size_t capacity =
				growth > v.max_size() - v.size() ? v.max_size() : v.size() + growth;
			if (capacity > SIZE_MAX / sizeof(T))
			{
				denied = true;
				throw auction_command_refusal{};
			}
			request = capacity * sizeof(T);
		}
		if (!peak(request))
			throw auction_command_refusal{};
	}
	template <class T> void push(std::vector<T> &v, const T &value)
	{
		growth(v, 1);
		v.push_back(value);
	}
	template <class T, class Iterator>
	void append(std::vector<T> &v, Iterator first, Iterator last)
	{
		const auto count = last - first;
		if (count < 0)
			throw std::length_error("auction command");
		growth(v, static_cast<size_t>(count));
		v.insert(v.end(), first, last);
	}
};
struct auction_command_denial_latch
{
	const auction_command_budget &budget;
	bool *output;
	~auction_command_denial_latch() noexcept
	{
		if (output && budget.denied)
			*output = true;
	}
};
template <class T> void auction_append_le_bounded(auction_command_budget &budget,
						  std::vector<uint8_t> *output, T value)
{
	using unsigned_type = std::make_unsigned_t<T>;
	const unsigned_type encoded = static_cast<unsigned_type>(value);
	for (size_t byte = 0; byte < sizeof(T); ++byte)
		budget.push(*output, static_cast<uint8_t>(encoded >> (byte * 8)));
}
template <size_t Size> bool auction_append_string_bounded(auction_command_budget &budget,
							  std::vector<uint8_t> *output,
							  const std::array<char, Size> &value)
{
	const size_t length = strnlen(value.data(), value.size());
	if (length >= value.size() || length > UINT16_MAX)
		return false;
	auction_append_le_bounded<uint16_t>(budget, output, static_cast<uint16_t>(length));
	budget.append(*output, value.begin(), value.begin() + length);
	return true;
}
bool auction_command_bounded_policy() noexcept
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	return sizeof(void *) == 8 && sizeof(size_t) == 8;
#else
	return false;
#endif
}
}

namespace
{
template <class T, class Compare> size_t auction_command_sort_source_frames(size_t count) noexcept
{
	using iterator = typename std::vector<T>::iterator;
	using difference = typename std::vector<T>::difference_type;
	using compare = __gnu_cxx::__ops::_Iter_comp_iter<Compare>;
	using value_compare = __gnu_cxx::__ops::_Val_comp_iter<Compare>;
	using iter_value_compare = __gnu_cxx::__ops::_Iter_comp_val<Compare>;
	// Actual typed sort/__sort and comparator construction/forwarding; their
	// normal_iterator ctor/base/difference and __lg caller/return scopes.
	constexpr size_t setup = 4 * sizeof(iterator) + 2 * sizeof(compare) + 2 * sizeof(void *) +
				 3 * sizeof(difference) + sizeof(int) +
				 8 * (sizeof(void *) + sizeof(iterator)) + 4 * sizeof(bool);
	constexpr size_t recursive = 3 * sizeof(iterator) + sizeof(difference) + sizeof(compare);
	// median-to-first, pivot partition, comparator forwarding and genuine swap
	// temporary are typed T, not a uint64 stand-in.
	constexpr size_t partition = 12 * sizeof(iterator) + 3 * sizeof(compare) +
				     2 * sizeof(bool) + 7 * sizeof(void *) + sizeof(T);
	constexpr size_t insertion = 10 * sizeof(iterator) + 2 * sizeof(compare) +
				     2 * sizeof(value_compare) + sizeof(iter_value_compare) +
				     3 * sizeof(T) + 2 * sizeof(difference) + 12 * sizeof(void *) +
				     3 * sizeof(bool);
	constexpr size_t heap = 12 * sizeof(iterator) + 14 * sizeof(difference) +
				5 * sizeof(compare) + 2 * sizeof(value_compare) +
				2 * sizeof(iter_value_compare) + 4 * sizeof(T) +
				18 * sizeof(void *) + 3 * sizeof(bool);
	// Function-pointer and captureless predicate bodies own their actual
	// two row references, this where present and returned bool separately.
	constexpr size_t comparator = 2 * sizeof(iterator) + 3 * sizeof(void *) + 3 * sizeof(bool);
	size_t logarithm = 0;
	for (size_t n = count; n > 1; n >>= 1)
		++logarithm;
	const size_t depth = count <= 16 ? 1 : std::min(logarithm * 2 + 1, count - 16 + 1);
	return setup + depth * recursive + std::max({ partition, insertion, heap }) + comparator;
}
}
size_t auction_command_build_sort_source_frame_bytes(const critical_command &command) noexcept
{
	using key_compare = decltype(&critical_entity_key_less);
	using revision_compare =
		decltype([](const critical_expected_revision &left,
			    const critical_expected_revision &right)
			 { return critical_entity_key_less(left.key, right.key); });
	// The original two sorts execute sequentially. Their actual source
	// scopes form a maximum; the declared caller/getter frame coexists.
	const size_t keys = auction_command_sort_source_frames<critical_entity_key, key_compare>(
		command.keys.size());
	const size_t revisions =
		auction_command_sort_source_frames<critical_expected_revision, revision_compare>(
			command.expected_revisions.size());
	// Original adjacent_find -> __adjacent_find with function-pointer
	// iterator comparator adapter, first/last/next and key equality body.
	const size_t adjacent =
		12 * sizeof(std::vector<critical_entity_key>::iterator) +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_iter<decltype(&critical_entity_key_equal)>) +
		10 * sizeof(void *) + 4 * sizeof(bool);
	return 3 * sizeof(void *) + 6 * sizeof(size_t) + sizeof(std::initializer_list<size_t>) +
	       std::max({ keys, revisions, adjacent });
}
size_t auction_command_matching_fences_source_frame_bytes(const critical_command &) noexcept
{
	using key_iterator = std::vector<critical_entity_key>::const_iterator;
	using revision_iterator = std::vector<critical_expected_revision>::const_iterator;
	// Original matching_native_item_fences parameters, outer key/known/
	// original/count/found scopes; original matching_fences indices and
	// genuine key/revision vector access and returned comparisons.
	constexpr size_t bodies = 18 * sizeof(void *) + 9 * sizeof(size_t) + 8 * sizeof(bool) +
				  4 * sizeof(key_iterator) + 4 * sizeof(revision_iterator);
	// is_sorted -> is_sorted_until -> __is_sorted_until, and adjacent_find
	// -> __adjacent_find: typed const iterators and real fn-pointer wrappers.
	constexpr size_t ordered =
		12 * sizeof(key_iterator) +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_iter<decltype(&critical_entity_key_less)>) +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_iter<decltype(&critical_entity_key_equal)>) +
		12 * sizeof(void *) + 6 * sizeof(bool);
	// binary_search -> lower_bound -> __lower_bound uses count/step
	// difference_type, midpoint, advance -> __advance/random-access tag,
	// iter_comp_val and final comparator. All loops are iterative.
	constexpr size_t search =
		12 * sizeof(key_iterator) + 5 * sizeof(std::ptrdiff_t) +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_val<decltype(&critical_entity_key_less)>) +
		15 * sizeof(void *) + 8 * sizeof(bool) +
		2 * sizeof(std::random_access_iterator_tag);
	// find_if/__find_if and count_if/__count_if actual captured key-reference
	// closures, _Iter_pred wrappers, iterators, trip_count/count and row
	// comparator bodies. No dictionary, allocation or recursive closure.
	constexpr size_t scans = 14 * sizeof(revision_iterator) + 8 * sizeof(void *) +
				 5 * sizeof(std::ptrdiff_t) + 14 * sizeof(void *) +
				 8 * sizeof(bool);
	return bodies + ordered + search + scans + auction_codec_vector_frames;
}

bool auction_command_encode_payload_bounded(const auction_command_payload &payload,
					    std::vector<uint8_t> *encoded,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer, bool *budget_denied) noexcept
{
	const size_t frames =
		// Actual encode and append helper args, local scalar values and loops,
		// vector reserve/growth/move/copy/destruction source controls.
		16 * sizeof(void *) + 12 * sizeof(size_t) + 8 * sizeof(bool) +
		4 * sizeof(uint64_t) + sizeof(std::array<char, AUCTION_INFO_MAX_BYTES + 1> *) +
		auction_codec_vector_frames + auction_codec_move_frames;
	auction_command_budget budget{ reserve, context, outer, frames };
	if (budget_denied)
		*budget_denied = false;
	auction_command_denial_latch denial{ budget, budget_denied };
	if (encoded)
	{
		budget.output = encoded;
		budget.excluded_caller_heap = encoded->capacity();
	}
	if (!auction_command_bounded_policy() || !budget.peak())
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	if (!encoded || !valid_payload(payload))
		return false;
	try
	{
		encoded->clear();
		const size_t reservation =
			512 + payload.object_blob_size +
			strnlen(payload.object_info.data(), payload.object_info.size());
		if (reservation > encoded->capacity() && !budget.peak(reservation))
			throw auction_command_refusal{};
		encoded->reserve(512 + payload.object_blob_size +
				 strnlen(payload.object_info.data(), payload.object_info.size()));
		auction_append_le_bounded<uint8_t>(budget, encoded,
						   static_cast<uint8_t>(payload.action));
		auction_append_le_bounded<uint32_t>(budget, encoded, payload.actor_pid);
		auction_append_le_bounded<uint32_t>(budget, encoded, payload.auction_id);
		auction_append_le_bounded<uint8_t>(budget, encoded, payload.racewar);
		auction_append_le_bounded<uint64_t>(budget, encoded,
						    payload.expected_wallet_revision);
		auction_append_le_bounded<uint64_t>(budget, encoded,
						    payload.expected_bank_revision);
		auction_append_le_bounded<int64_t>(budget, encoded, payload.value);
		auction_append_le_bounded<int64_t>(budget, encoded, payload.start_price);
		auction_append_le_bounded<int64_t>(budget, encoded, payload.buy_price);
		auction_append_le_bounded<int64_t>(budget, encoded, payload.listing_fee);
		auction_append_le_bounded<uint32_t>(budget, encoded,
						    payload.closing_fee_basis_points);
		auction_append_le_bounded<uint32_t>(budget, encoded, payload.bid_extension_seconds);
		auction_append_le_bounded<uint64_t>(budget, encoded, payload.end_time);
		auction_append_le_bounded<uint16_t>(budget, encoded, payload.item_count);
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			auction_append_le_bounded<uint64_t>(budget, encoded,
							    payload.items[index].item_uid);
			auction_append_le_bounded<uint64_t>(
				budget, encoded, payload.items[index].expected_item_revision);
			auction_append_le_bounded<int32_t>(budget, encoded,
							   payload.items[index].vnum);
		}
		if (!auction_append_string_bounded(budget, encoded, payload.account_name) ||
		    !auction_append_string_bounded(budget, encoded, payload.actor_name) ||
		    !auction_append_string_bounded(budget, encoded, payload.object_short) ||
		    !auction_append_string_bounded(budget, encoded, payload.id_keywords) ||
		    !auction_append_string_bounded(budget, encoded, payload.object_info))
			return false;
		auction_append_le_bounded<uint32_t>(budget, encoded, payload.object_blob_size);
		budget.append(*encoded, payload.object_blob.begin(),
			      payload.object_blob.begin() + payload.object_blob_size);
	}
	catch (const auction_command_refusal &)
	{
		if (budget_denied)
			*budget_denied = true;
		encoded->clear();
		return false;
	}
	catch (const std::bad_alloc &)
	{
		encoded->clear();
		return false;
	}
	return encoded->size() <= CRITICAL_COMMAND_MAX_PAYLOAD_BYTES;
}

bool auction_command_build_bounded(critical_command *command, critical_operation_id operation_id,
				   const auction_command_payload &payload,
				   critical_source_site source_site,
				   critical_deadline_class deadline_class,
				   bool (*reserve)(size_t, void *) noexcept, void *context,
				   size_t outer, bool *budget_denied) noexcept
{
	const size_t frames =
		// Complete actual build values: account/player/item keys, operation
		// parameter, two local vectors, scalar loops and result latches.
		sizeof(critical_operation_id) + 3 * sizeof(critical_entity_key) +
		sizeof(std::vector<critical_entity_key>) +
		sizeof(std::vector<critical_expected_revision>) + sizeof(critical_command) +
		16 * sizeof(void *) + 11 * sizeof(size_t) + 8 * sizeof(bool) +
		3 * sizeof(uint64_t) + auction_codec_vector_frames + auction_codec_move_frames;
	auction_command_budget budget{ reserve, context, outer, frames };
	if (budget_denied)
		*budget_denied = false;
	auction_command_denial_latch denial{ budget, budget_denied };
	if (command)
	{
		// Admit the pure copy-profile getter's return carrier, then its real
		// complete command CURRENT observer closure before touching output.
		// command remains null in the budget until this physical scan finishes.
		if (!auction_command_bounded_policy() || !budget.peak(sizeof(size_t)) ||
		    !budget.peak(critical_command_copy_frame_bytes()))
		{
			if (budget_denied)
				*budget_denied = true;
			return false;
		}
		if (!critical_command_current_heap_bytes(*command, &budget.excluded_caller_heap))
		{
			budget.denied = true;
			return false;
		}
		budget.command = command;
	}
	if (!auction_command_bounded_policy() || !budget.peak())
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	if (!command || !valid_payload(payload))
		return false;
	critical_entity_key account_key = {};
	std::vector<critical_entity_key> keys;
	std::vector<critical_expected_revision> revisions;
	try
	{
		budget.keys = &keys;
		budget.revisions = &revisions;
		size_t nested = 0;
		if (payload.actor_pid)
		{
			const critical_entity_key player = { critical_entity_type::player,
							     payload.actor_pid };
			if (!budget.prefix(nested) ||
			    !currency_account_key_bounded(
				    payload.account_name.data(), payload.racewar, &account_key,
				    auction_command_budget::forward, &budget, nested))
				return false;
			budget.push(keys, player);
			budget.push(keys, account_key);
			budget.push(revisions, critical_expected_revision{
						       player, payload.expected_wallet_revision });
			budget.push(revisions,
				    critical_expected_revision{ account_key,
								payload.expected_bank_revision });
		}
		budget.push(keys,
			    critical_entity_key{ critical_entity_type::auction,
						 payload.auction_id ? payload.auction_id :
								      listing_key(operation_id) });
		for (size_t index = 0; index < payload.item_count; ++index)
		{
			const critical_entity_key item = { critical_entity_type::item,
							   payload.items[index].item_uid };
			budget.push(keys, item);
			budget.push(revisions,
				    critical_expected_revision{
					    item, payload.items[index].expected_item_revision });
		}
	}
	catch (const auction_command_refusal &)
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	*command = { .schema_version = CRITICAL_COMMAND_SCHEMA_VERSION,
		     .operation_id = operation_id,
		     .type = critical_command_type::auction,
		     .payload_version = AUCTION_COMMAND_PAYLOAD_VERSION,
		     .source_site = source_site,
		     .deadline_class = deadline_class,
		     .accepted_at_usec = 0,
		     .keys = std::move(keys),
		     .expected_revisions = std::move(revisions),
		     .payload = {} };
	size_t nested = 0;
	if (!budget.prefix(nested) ||
	    !auction_command_encode_payload_bounded(payload, &command->payload,
						    auction_command_budget::forward, &budget,
						    nested, budget_denied))
		return false;
	if (!budget.peak(20 * sizeof(void *) + 19 * sizeof(size_t) + 3 * sizeof(bool)))
		return false;
	if (!budget.peak(auction_command_build_sort_source_frame_bytes(*command)))
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	std::sort(command->keys.begin(), command->keys.end(), critical_entity_key_less);
	if (std::adjacent_find(command->keys.begin(), command->keys.end(),
			       critical_entity_key_equal) != command->keys.end())
		return false;
	std::sort(command->expected_revisions.begin(), command->expected_revisions.end(),
		  [](const critical_expected_revision &left,
		     const critical_expected_revision &right)
		  { return critical_entity_key_less(left.key, right.key); });
	return true;
}

static bool decode_original_payload_bounded(const critical_command &command,
					    auction_command_payload *payload,
					    bool native_item_fences,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer, bool *budget_denied)
{
	const size_t frames = sizeof(critical_command) + 16 * sizeof(void *) + 8 * sizeof(size_t) +
			      8 * sizeof(bool) + 4 * sizeof(uint64_t) + 4 * sizeof(uint32_t) +
			      3 * sizeof(uint16_t) + auction_codec_vector_constructor_frames +
			      auction_codec_move_frames;
	auction_command_budget budget{ reserve, context, outer, frames };
	if (budget_denied)
		*budget_denied = false;
	auction_command_denial_latch denial{ budget, budget_denied };
	// The accessor owns one command reference, four constexpr size_t locals
	// and its returned size_t; no CURRENT/profile query precedes this admission.
	if (!auction_command_bounded_policy() ||
	    !budget.peak(sizeof(void *) + 5 * sizeof(size_t)) ||
	    !budget.peak(auction_command_matching_fences_source_frame_bytes(command)))
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	if (!payload || command.type != critical_command_type::auction ||
	    command.payload_version != AUCTION_COMMAND_PAYLOAD_VERSION)
		return false;
	*payload = {};
	const uint8_t *cursor = command.payload.data();
	const uint8_t *end = cursor + command.payload.size();
	uint8_t action = 0;
	if (!read_le(&cursor, end, &action) || !read_le(&cursor, end, &payload->actor_pid) ||
	    !read_le(&cursor, end, &payload->auction_id) ||
	    !read_le(&cursor, end, &payload->racewar) ||
	    !read_le(&cursor, end, &payload->expected_wallet_revision) ||
	    !read_le(&cursor, end, &payload->expected_bank_revision) ||
	    !read_le(&cursor, end, &payload->value) ||
	    !read_le(&cursor, end, &payload->start_price) ||
	    !read_le(&cursor, end, &payload->buy_price) ||
	    !read_le(&cursor, end, &payload->listing_fee) ||
	    !read_le(&cursor, end, &payload->closing_fee_basis_points) ||
	    !read_le(&cursor, end, &payload->bid_extension_seconds) ||
	    !read_le(&cursor, end, &payload->end_time) ||
	    !read_le(&cursor, end, &payload->item_count) ||
	    payload->item_count > payload->items.size())
		return false;
	payload->action = static_cast<auction_action>(action);
	for (size_t index = 0; index < payload->item_count; ++index)
		if (!read_le(&cursor, end, &payload->items[index].item_uid) ||
		    !read_le(&cursor, end, &payload->items[index].expected_item_revision) ||
		    !read_le(&cursor, end, &payload->items[index].vnum))
			return false;
	if (!read_string(&cursor, end, &payload->account_name) ||
	    !read_string(&cursor, end, &payload->actor_name) ||
	    !read_string(&cursor, end, &payload->object_short) ||
	    !read_string(&cursor, end, &payload->id_keywords) ||
	    !read_string(&cursor, end, &payload->object_info) ||
	    !read_le(&cursor, end, &payload->object_blob_size) ||
	    payload->object_blob_size > payload->object_blob.size() ||
	    static_cast<size_t>(end - cursor) != payload->object_blob_size)
		return false;
	memcpy(payload->object_blob.data(), cursor, payload->object_blob_size);
	if (!valid_payload(*payload))
		return false;
	critical_command expected = {};
	budget.command = &expected;
	size_t nested = 0;
	if (!budget.prefix(nested) ||
	    !auction_command_build_bounded(&expected, command.operation_id, *payload,
					   command.source_site, command.deadline_class,
					   auction_command_budget::forward, &budget, nested,
					   budget_denied) ||
	    !(native_item_fences ? matching_native_item_fences(expected, command) :
				   matching_fences(expected, command)))
		return false;
	return true;
}

bool auction_command_decode_native_base_bounded(const critical_command &command,
						std::span<const uint8_t> base,
						auction_command_payload *payload,
						bool (*reserve)(size_t, void *) noexcept,
						void *context, size_t outer,
						bool *budget_denied) noexcept
{
	const size_t frames = sizeof(critical_command) + sizeof(auction_command_payload) +
			      14 * sizeof(void *) + 8 * sizeof(size_t) + 6 * sizeof(bool) +
			      sizeof(std::span<const uint8_t>) + auction_codec_vector_frames +
			      auction_codec_move_frames;
	auction_command_budget budget{ reserve, context, outer, frames };
	if (budget_denied)
		*budget_denied = false;
	auction_command_denial_latch denial{ budget, budget_denied };
	if (!auction_command_bounded_policy() || !budget.peak())
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	try
	{
		if (!payload || command.type != critical_command_type::auction ||
		    command.payload_version != AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION ||
		    (command.schema_version != CRITICAL_COMMAND_SCHEMA_VERSION &&
		     command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION))
			return false;
		size_t request = 0, nested = 0;
		if (!critical_command_fresh_copy_request_bytes(command, &request) ||
		    !budget.peak(request + critical_command_copy_frame_bytes()))
		{
			if (budget_denied)
				*budget_denied = true;
			return false;
		}
		critical_command original = command;
		budget.command = &original;
		original.payload_version = AUCTION_COMMAND_PAYLOAD_VERSION;
		if (base.size() > original.payload.capacity() && !budget.peak(base.size()))
		{
			if (budget_denied)
				*budget_denied = true;
			return false;
		}
		original.payload.assign(base.begin(), base.end());
		auction_command_payload parsed{};
		if (!budget.prefix(nested) ||
		    !decode_original_payload_bounded(original, &parsed, true,
						     auction_command_budget::forward, &budget,
						     nested, budget_denied))
			return false;
		if (parsed.action == auction_action::claim_money &&
		    !matching_fences(
			    original,
			    [&]()
			    {
				    critical_command expected{};
				    if (!budget.prefix(nested) ||
					!auction_command_build_bounded(
						&expected, command.operation_id, parsed,
						command.source_site, command.deadline_class,
						auction_command_budget::forward, &budget, nested,
						budget_denied))
				    {
					    if (budget.denied || (budget_denied && *budget_denied))
						    throw auction_command_refusal{};
				    }
				    return expected;
			    }()))
			return false;
		*payload = parsed;
		return true;
	}
	catch (const auction_command_refusal &)
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool auction_command_decode_payload_bounded(const critical_command &command,
					    auction_command_payload *payload,
					    bool (*reserve)(size_t, void *) noexcept, void *context,
					    size_t outer, bool *budget_denied) noexcept
{
	if (budget_denied)
		*budget_denied = false;
	if (payload && command.type == critical_command_type::auction &&
	    command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION)
	{
		const size_t frames = sizeof(auction_native_command_context) + 10 * sizeof(void *) +
				      3 * sizeof(size_t) + 3 * sizeof(bool);
		auction_command_budget budget{ reserve, context, outer, frames };
		size_t nested = 0;
		if (!auction_command_bounded_policy() || !budget.peak() || !budget.prefix(nested))
		{
			if (budget_denied)
				*budget_denied = true;
			return false;
		}
		auction_native_command_context candidate;
		const auto result = auction_native_command_decode_bounded(
			command, &candidate, auction_command_budget::forward, &budget, nested);
		if (result != economic_accounting_error::ok)
		{
			if (budget_denied)
				*budget_denied = budget.denied;
			return false;
		}
		*payload = candidate.payload;
		return true;
	}
	try
	{
		return decode_original_payload_bounded(command, payload, false, reserve, context,
						       outer, budget_denied);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool auction_command_encode_result_bounded(
	const auction_command_result &result,
	std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> *encoded,
	bool (*reserve)(size_t, void *) noexcept, void *context, size_t outer,
	bool *budget_denied) noexcept
{
	const size_t frames = sizeof(std::vector<uint8_t>) + 14 * sizeof(void *) +
			      10 * sizeof(size_t) + 6 * sizeof(bool) + 4 * sizeof(uint64_t) +
			      4 * sizeof(int64_t) + auction_codec_vector_frames +
			      auction_codec_move_frames;
	auction_command_budget budget{ reserve, context, outer, frames };
	if (budget_denied)
		*budget_denied = false;
	auction_command_denial_latch denial{ budget, budget_denied };
	if (!auction_command_bounded_policy() || !budget.peak())
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	if (!encoded || result.item_count > result.item_uids.size())
		return false;
	encoded->fill(0);
	std::vector<uint8_t> bytes;
	budget.output = &bytes;
	try
	{
		auction_append_le_bounded<uint8_t>(budget, &bytes,
						   static_cast<uint8_t>(result.action));
		auction_append_le_bounded<uint8_t>(budget, &bytes,
						   static_cast<uint8_t>(result.event_type));
		auction_append_le_bounded<uint32_t>(budget, &bytes, result.auction_id);
		auction_append_le_bounded<uint32_t>(budget, &bytes, result.status);
		auction_append_le_bounded<uint32_t>(budget, &bytes, result.seller_pid);
		auction_append_le_bounded<uint32_t>(budget, &bytes, result.winner_pid);
		auction_append_le_bounded<uint32_t>(budget, &bytes, result.previous_bidder_pid);
		auction_append_le_bounded<int64_t>(budget, &bytes, result.final_price);
		auction_append_le_bounded<int64_t>(budget, &bytes, result.wallet_value_delta);
		for (int64_t value : result.wallet.amount)
			auction_append_le_bounded<int64_t>(budget, &bytes, value);
		for (int64_t value : result.bank.amount)
			auction_append_le_bounded<int64_t>(budget, &bytes, value);
		auction_append_le_bounded<uint64_t>(budget, &bytes, result.wallet_revision);
		auction_append_le_bounded<uint64_t>(budget, &bytes, result.bank_revision);
		auction_append_le_bounded<uint64_t>(budget, &bytes, result.auction_revision);
		auction_append_le_bounded<uint64_t>(budget, &bytes, result.player_owner_revision);
		auction_append_le_bounded<uint64_t>(budget, &bytes, result.auction_owner_revision);
		auction_append_le_bounded<uint16_t>(budget, &bytes, result.item_count);
		for (size_t index = 0; index < result.item_count; ++index)
		{
			auction_append_le_bounded<uint64_t>(budget, &bytes,
							    result.item_uids[index]);
			auction_append_le_bounded<uint64_t>(budget, &bytes,
							    result.item_revisions[index]);
		}
		auction_append_le_bounded<int64_t>(budget, &bytes, result.claim_credit_used);
	}
	catch (const auction_command_refusal &)
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (bytes.size() > encoded->size())
		return false;
	std::copy(bytes.begin(), bytes.end(), encoded->begin());
	return true;
}
#else

size_t auction_command_build_sort_source_frame_bytes(const critical_command &) noexcept
{
	return 0;
}
size_t auction_command_matching_fences_source_frame_bytes(const critical_command &) noexcept
{
	return 0;
}
bool auction_command_encode_payload_bounded(const auction_command_payload &, std::vector<uint8_t> *,
					    bool (*)(size_t, void *) noexcept, void *, size_t,
					    bool *denied) noexcept
{
	if (denied)
		*denied = true;
	return false;
}
bool auction_command_build_bounded(critical_command *, critical_operation_id,
				   const auction_command_payload &, critical_source_site,
				   critical_deadline_class, bool (*)(size_t, void *) noexcept,
				   void *, size_t, bool *denied) noexcept
{
	if (denied)
		*denied = true;
	return false;
}
bool auction_command_decode_native_base_bounded(const critical_command &, std::span<const uint8_t>,
						auction_command_payload *,
						bool (*)(size_t, void *) noexcept, void *, size_t,
						bool *denied) noexcept
{
	if (denied)
		*denied = true;
	return false;
}
bool auction_command_decode_payload_bounded(const critical_command &, auction_command_payload *,
					    bool (*)(size_t, void *) noexcept, void *, size_t,
					    bool *denied) noexcept
{
	if (denied)
		*denied = true;
	return false;
}
bool auction_command_encode_result_bounded(const auction_command_result &,
					   std::array<uint8_t, AUCTION_RESULT_PAYLOAD_BYTES> *,
					   bool (*)(size_t, void *) noexcept, void *, size_t,
					   bool *denied) noexcept
{
	if (denied)
		*denied = true;
	return false;
}

#endif

// Complete lower auction codec ownership. These additive companions retain the
// uncovered SOURCE of the unchanged original graph, not a second full baseline.
#include <openssl/sha.h>

namespace
{
[[maybe_unused]] constexpr size_t auction_lower_P = sizeof(void *);
[[maybe_unused]] constexpr size_t auction_lower_N = sizeof(size_t);
[[maybe_unused]] constexpr size_t auction_lower_B = sizeof(bool);
[[maybe_unused]] constexpr size_t auction_lower_D = sizeof(std::ptrdiff_t);

[[maybe_unused]] bool auction_lower_source_policy() noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__GLIBCXX__) &&                          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&      \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                           \
	!defined(__SANITIZE_ADDRESS__) && !defined(__SANITIZE_THREAD__) &&                        \
	!defined(_GLIBCXX_SANITIZE_VECTOR) && defined(OPENSSL_VERSION_MAJOR) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	return sizeof(void *) == 8 && sizeof(size_t) == 8 && sizeof(std::ptrdiff_t) == 8 &&
	       sizeof(bool) == 1 && sizeof(unsigned long) == 8 && sizeof(SHA_LONG) == 4 &&
	       sizeof(std::allocator<uint8_t>) == 1 &&
	       sizeof(std::vector<critical_entity_key>::iterator) == sizeof(void *) &&
	       sizeof(std::vector<critical_expected_revision>::iterator) == sizeof(void *);
#else
	return false;
#endif
}

#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)

// These are exactly the original persistent SOURCE terms. The command,
// payload, two local vectors, budget and denial relay are INLINE; each remains
// owned by the original actual prefix, apart from the omitted temporary below.
constexpr size_t auction_lower_encode_owned_source =
	16 * auction_lower_P + 12 * auction_lower_N + 8 * auction_lower_B + 4 * sizeof(uint64_t) +
	auction_lower_P + auction_codec_vector_frames + auction_codec_move_frames;
// The original builder's 3*sizeof(uint64_t) is the existing INLINE credit
// for one critical_expected_revision{key,revision} bound to push(const T&).
// Those three argument temporaries are sequential full expressions; each
// survives its growth/callback/push, with no second retained allowance.
static_assert(sizeof(critical_expected_revision) == 3 * sizeof(uint64_t),
	      "Pinned LP64 original revision temporary credit");
constexpr size_t auction_lower_build_owned_source =
	sizeof(critical_operation_id) + 3 * sizeof(critical_entity_key) + 16 * auction_lower_P +
	11 * auction_lower_N + 8 * auction_lower_B + auction_codec_vector_frames +
	auction_codec_move_frames;
constexpr size_t auction_lower_v1_owned_source =
	16 * auction_lower_P + 8 * auction_lower_N + 8 * auction_lower_B + 4 * sizeof(uint64_t) +
	4 * sizeof(uint32_t) + 3 * sizeof(uint16_t) + auction_codec_vector_constructor_frames +
	auction_codec_move_frames;
constexpr size_t auction_lower_base_owned_source =
	14 * auction_lower_P + 8 * auction_lower_N + 6 * auction_lower_B +
	sizeof(std::span<const uint8_t>) + auction_codec_vector_frames + auction_codec_move_frames;

// Actual budget::forward(amount,opaque), b reference and bool result;
// prefix(this,bytes,extra), heap, bool; peak(this,extra), bytes, bool;
// checked add(total,value), bool; growth(this,v,count), request/growth/capacity;
// push(this,v,value); append(this,v,first,last), difference count;
// denial-latch destructor(this). No callback implementation is inventoried.
constexpr size_t auction_lower_budget_source =
	3 * auction_lower_P + auction_lower_N + auction_lower_B + 2 * auction_lower_P +
	2 * auction_lower_N + auction_lower_B + auction_lower_P + 2 * auction_lower_N +
	auction_lower_B + auction_lower_P + auction_lower_N + auction_lower_B +
	2 * auction_lower_P + 4 * auction_lower_N + 3 * auction_lower_P + 4 * auction_lower_P +
	auction_lower_D + auction_lower_P;

// Original read_le<T> and append_le_bounded<T> select six genuine scalar
// instantiations (u8/u16/u32/u64/i32/i64). Scalar widths are not DTO baselines.
constexpr size_t auction_lower_scalar_widths = sizeof(uint8_t) + sizeof(uint16_t) +
					       sizeof(uint32_t) + sizeof(uint64_t) +
					       sizeof(int32_t) + sizeof(int64_t);
constexpr size_t auction_lower_number_source =
	6 * (3 * auction_lower_P + auction_lower_N + auction_lower_B) +
	auction_lower_scalar_widths +
	6 * (2 * auction_lower_P + auction_lower_N + sizeof(uint8_t)) +
	2 * auction_lower_scalar_widths;

// Five real read_string/append_string instantiations. The byte fill closure is
// array.fill -> fill_n -> __fill_n_a(RA) -> __fill_a -> byte __fill_a1.
// Its genuine zero/value temporary, count/tag, constant-evaluation result and
// returned pointer are distinct from the original numeric scalar ledger.
constexpr size_t auction_lower_string_source =
	5 * (3 * auction_lower_P + sizeof(uint16_t) + auction_lower_B + 3 * auction_lower_P +
	     auction_lower_N + auction_lower_B) +
	2 * auction_lower_P + sizeof(char) + 2 * (3 * auction_lower_P + auction_lower_N) +
	sizeof(std::random_access_iterator_tag) + 2 * auction_lower_N + 2 * (3 * auction_lower_P) +
	sizeof(char) + auction_lower_N + 2 * auction_lower_B + auction_lower_P +
	sizeof(std::random_access_iterator_tag);

// Genuine fixed-array access and declarations of libc calls reached here.
// Runtime libc implementation remains a separate native qualification gate.
constexpr size_t auction_lower_array_source =
	2 * auction_lower_P + // data(this,result)
	2 * (4 * auction_lower_P) + // begin/end through data
	auction_lower_P + auction_lower_N + // size(this,result)
	2 * auction_lower_P + auction_lower_N + // [](this,n,returned-reference)
	2 * auction_lower_P + auction_lower_N + // strnlen(input,n,return)
	3 * auction_lower_P + auction_lower_N; // memcpy(dst,src,n,return)

// GNU13 ordinary vector descendants absent from the original primitive ledger:
// allocator access/copy/destruction; C++20 alloc_on_move, allocator assignments;
// data default/destructor and third copy; move constructors/reset; const-
// allocator temporary construction; base/impl/data/allocator destructor chain;
// actual constant-evaluation result; clear's erase_at_end(this,pos,n).
constexpr size_t auction_lower_vector_missing_source =
	3 * (2 * auction_lower_P) + 2 * auction_lower_P + 2 * auction_lower_P +
	2 * (3 * auction_lower_P) + 2 * (2 * auction_lower_P) + 2 * auction_lower_P +
	2 * auction_lower_P + 16 * auction_lower_P + sizeof(uint8_t *) + 11 * auction_lower_P +
	auction_lower_P + 4 * auction_lower_P + 2 * auction_lower_P + auction_lower_B +
	2 * auction_lower_P + auction_lower_N + 2 * auction_lower_P +
	auction_lower_N; // actual vector [](this,n,ref)

// Actual payload/member aggregate assignment and default vector construction.
// The auction payload contains seven fixed arrays, including item entries.
// Key/revision comparator values have genuine move/assignment/dtor receivers.
constexpr size_t auction_lower_generated_source =
	3 * auction_lower_P + 7 * (3 * auction_lower_P) + 3 * auction_lower_P + auction_lower_P +
	6 * (6 * auction_lower_P) +
	2 * (2 * auction_lower_P + 3 * auction_lower_P + auction_lower_P);

// matching_native_item_fences uses three genuine captured-key predicates.
// Each selected find/count graph carries the reference capture through the
// public parameter, __pred_iter, _Iter_pred, RA helper and operator(). Include
// generated predicate and iterator member lifetime scopes as actual carriers.
constexpr size_t auction_lower_captured_predicate_source =
	3 * (8 * auction_lower_P + auction_lower_P + 3 * (2 * auction_lower_P) +
	     2 * auction_lower_P + 2 * auction_lower_P + 2 * auction_lower_P +
	     2 * (2 * auction_lower_P) + 2 * (2 * auction_lower_P)) +
	2 * auction_lower_P + auction_lower_P;

// claim_money's genuine expected-command lambda captures command/parsed/budget/
// nested/budget_denied by reference; its receiver and returned command move
// coexist. listing_key owns operation reference, key/index and returned key.
constexpr size_t auction_lower_lambda_source = 5 * auction_lower_P + auction_lower_P +
					       2 * auction_lower_P + auction_lower_P +
					       2 * sizeof(uint64_t) + auction_lower_N;

constexpr size_t auction_lower_uncovered_source =
	auction_lower_budget_source + auction_lower_number_source + auction_lower_string_source +
	auction_lower_array_source + auction_lower_vector_missing_source +
	auction_lower_generated_source + auction_lower_captured_predicate_source +
	auction_lower_lambda_source;

struct auction_lower_fixed_work
{
	size_t source = 0, supplement = 0, initial = 0, request = 0;
};
constexpr size_t auction_lower_fixed_call_source =
	// Fixed public formals: command,payload,reserve,context,denial; outer;
	// returned bool, actual query local and original call returned bool.
	5 * auction_lower_P + 2 * auction_lower_N + 2 * auction_lower_B;
// These original automatic temporaries are genuinely omitted by the old
// prefix: v1 payload assignment's complete {} value, and claim_money's second
// expected command inside the canonical lambda. Keep them INLINE, never SOURCE.
constexpr size_t auction_lower_missing_inline =
	sizeof(auction_command_payload) + sizeof(critical_command);

template <class T, class Compare>
constexpr size_t auction_lower_fixed_sort_source(size_t count) noexcept
{
	using iterator = typename std::vector<T>::iterator;
	using difference = typename std::vector<T>::difference_type;
	using compare = __gnu_cxx::__ops::_Iter_comp_iter<Compare>;
	using value_compare = __gnu_cxx::__ops::_Val_comp_iter<Compare>;
	using iter_value_compare = __gnu_cxx::__ops::_Iter_comp_val<Compare>;
	constexpr size_t setup = 4 * sizeof(iterator) + 2 * sizeof(compare) + 2 * sizeof(void *) +
				 3 * sizeof(difference) + sizeof(int) +
				 8 * (sizeof(void *) + sizeof(iterator)) + 4 * sizeof(bool);
	constexpr size_t recursive = 3 * sizeof(iterator) + sizeof(difference) + sizeof(compare);
	constexpr size_t partition = 12 * sizeof(iterator) + 3 * sizeof(compare) +
				     2 * sizeof(bool) + 7 * sizeof(void *) + sizeof(T);
	constexpr size_t insertion = 10 * sizeof(iterator) + 2 * sizeof(compare) +
				     2 * sizeof(value_compare) + sizeof(iter_value_compare) +
				     3 * sizeof(T) + 2 * sizeof(difference) + 12 * sizeof(void *) +
				     3 * sizeof(bool);
	constexpr size_t heap = 12 * sizeof(iterator) + 14 * sizeof(difference) +
				5 * sizeof(compare) + 2 * sizeof(value_compare) +
				2 * sizeof(iter_value_compare) + 4 * sizeof(T) +
				18 * sizeof(void *) + 3 * sizeof(bool);
	constexpr size_t comparator = 2 * sizeof(iterator) + 3 * sizeof(void *) + 3 * sizeof(bool);
	size_t logarithm = 0;
	for (size_t n = count; n > 1; n >>= 1)
		++logarithm;
	const size_t depth = count <= 16 ? 1 : std::min(logarithm * 2 + 1, count - 16 + 1);
	return setup + depth * recursive + std::max({ partition, insertion, heap }) + comparator;
}
constexpr size_t auction_lower_key_sort_source =
	auction_lower_fixed_sort_source<critical_entity_key, decltype(&critical_entity_key_less)>(
		AUCTION_COMMAND_MAX_ITEMS + 3);
constexpr size_t auction_lower_revision_sort_source = auction_lower_fixed_sort_source<
	critical_expected_revision,
	decltype([](const critical_expected_revision &left, const critical_expected_revision &right)
		 { return critical_entity_key_less(left.key, right.key); })>(
	AUCTION_COMMAND_MAX_ITEMS + 2);

bool auction_lower_base_parts(size_t *source, size_t *supplement, size_t *initial) noexcept
{
	if (!source || !supplement || !initial || !auction_lower_source_policy())
		return false;
	size_t account = 0, account_extra = 0, observation = 0;
	if (!currency_account_key_source_frame_bytes(&account) ||
	    !currency_account_key_source_supplement_frame_bytes(&account_extra) ||
	    !critical_command_current_heap_observer_frame_bytes(&observation))
		return false;
	size_t complete = auction_lower_encode_owned_source + auction_lower_build_owned_source +
			  auction_lower_v1_owned_source + auction_lower_base_owned_source;
	size_t extra = auction_lower_uncovered_source;
	// Both canonical sorts have genuine finite typed source bounds: valid
	// payload permits at most nine items plus player/account/auction keys.
	// Original matching-fences getter's pure complete typed body is selected
	// without fabricated command storage: its parameter is unused. The same
	// scalar/control subtotal is reproduced from the authenticated definition.
	using K = std::vector<critical_entity_key>::const_iterator;
	using R = std::vector<critical_expected_revision>::const_iterator;
	constexpr size_t matching =
		18 * auction_lower_P + 9 * auction_lower_N + 8 * auction_lower_B + 4 * sizeof(K) +
		4 * sizeof(R) + 12 * sizeof(K) +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_iter<decltype(&critical_entity_key_less)>) +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_iter<decltype(&critical_entity_key_equal)>) +
		12 * auction_lower_P + 6 * auction_lower_B + 12 * sizeof(K) + 5 * auction_lower_D +
		4 * sizeof(__gnu_cxx::__ops::_Iter_comp_val<decltype(&critical_entity_key_less)>) +
		15 * auction_lower_P + 8 * auction_lower_B +
		2 * sizeof(std::random_access_iterator_tag) + 14 * sizeof(R) + 8 * auction_lower_P +
		5 * auction_lower_D + 14 * auction_lower_P + 8 * auction_lower_B +
		auction_codec_vector_frames;
	if (!auction_codec_add(extra, account_extra) || !auction_codec_add(extra, observation) ||
	    !auction_codec_add(complete, auction_lower_uncovered_source) ||
	    !auction_codec_add(complete, account) || !auction_codec_add(complete, observation) ||
	    !auction_codec_add(complete, critical_command_copy_frame_bytes()) ||
	    !auction_codec_add(complete, auction_lower_key_sort_source) ||
	    !auction_codec_add(complete, auction_lower_revision_sort_source) ||
	    !auction_codec_add(complete, matching))
		return false;
	*source = complete;
	*supplement = extra;
	*initial =
		sizeof(auction_command_budget) + 2 * auction_lower_P + auction_lower_missing_inline;
	return true;
}
#endif
}

bool auction_command_base_codec_source_parts(size_t *source, size_t *supplement,
					     size_t *initial) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	return auction_lower_base_parts(source, supplement, initial);
#else
	(void)source;
	(void)supplement;
	(void)initial;
	return false;
#endif
}

namespace
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
bool auction_lower_fixed_parts(size_t *source, size_t *supplement, size_t *initial) noexcept
{
	if (!source || !supplement || !initial || !auction_lower_source_policy())
		return false;
	size_t full = 0, extra = 0, entry = 0;
	// The native graph includes the complete v1/base/encoder graph. Its pure
	// parts accessor has no dependency on the public payload dispatcher, so
	// the full v1/v2 source union has no recursive profile call.
	if (!auction_native_command_codec_source_parts(&full, &extra, &entry) ||
	    !auction_codec_add(full, auction_lower_fixed_call_source) ||
	    !auction_codec_add(full,
			       10 * auction_lower_P + 3 * auction_lower_N + 3 * auction_lower_B))
		return false;
	*source = full;
	*supplement = extra;
	*initial = sizeof(auction_lower_fixed_work) + sizeof(auction_command_budget) +
		   2 * auction_lower_P + auction_lower_missing_inline;
	return true;
}
#endif
}

bool auction_command_decode_payload_fixed_bounded(const critical_command &command,
						  auction_command_payload *payload,
						  bool (*reserve)(size_t, void *) noexcept,
						  void *context, size_t outer,
						  bool *budget_denied) noexcept
{
	if (budget_denied)
		*budget_denied = false;
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	constexpr size_t query = auction_command_decode_payload_source_query_frame_bytes();
	auction_lower_fixed_work work;
	work.request = outer;
	// Admit real profile-query carriers before entering any getter; then the
	// complete original SOURCE and initial automatic storage before old entry.
	if (!reserve ||
	    !auction_codec_add(work.request,
			       sizeof(work) + auction_lower_fixed_call_source + query) ||
	    !reserve(work.request, context) ||
	    !auction_lower_fixed_parts(&work.source, &work.supplement, &work.initial))
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	work.request = outer;
	if (!auction_codec_add(work.request, work.source) ||
	    !auction_codec_add(work.request, work.initial) || !reserve(work.request, context))
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	work.request = outer;
	if (!auction_codec_add(work.request, sizeof(work) + auction_lower_fixed_call_source) ||
	    !auction_codec_add(work.request, work.supplement) ||
	    !auction_codec_add(work.request, auction_lower_missing_inline))
	{
		if (budget_denied)
			*budget_denied = true;
		return false;
	}
	// Unchanged original dispatcher remains authoritative for both versions,
	// including v1 partial output and native v2 strong output/error behavior.
	return auction_command_decode_payload_bounded(command, payload, reserve, context,
						      work.request, budget_denied);
#else
	(void)command;
	(void)payload;
	(void)reserve;
	(void)context;
	(void)outer;
	if (budget_denied)
		*budget_denied = true;
	return false;
#endif
}

bool auction_command_decode_payload_source_frame_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	size_t full = 0, extra = 0, entry = 0;
	if (!output || !auction_lower_fixed_parts(&full, &extra, &entry))
		return false;
	*output = full;
	return true;
#else
	(void)output;
	return false;
#endif
}
bool auction_command_decode_payload_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	size_t full = 0, extra = 0, entry = 0;
	if (!output || !auction_lower_fixed_parts(&full, &extra, &entry))
		return false;
	*output = 0; // Fixed companion owns its genuine uncovered Source internally.
	return true;
#else
	(void)output;
	return false;
#endif
}
bool auction_command_decode_payload_initial_inline_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && __cplusplus == 202002L &&                        \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG) && !defined(_GLIBCXX_ASSERTIONS) &&    \
	!defined(_GLIBCXX_PARALLEL) && !defined(_GLIBCXX_SANITIZE_VECTOR)
	size_t full = 0, extra = 0, entry = 0;
	if (!output || !auction_lower_fixed_parts(&full, &extra, &entry))
		return false;
	*output = entry;
	return true;
#else
	(void)output;
	return false;
#endif
}
