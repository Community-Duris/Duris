#include "item/held_retirement_recovery.h"
#include "economy/item_transfer_accounting.h"
#include "core/defines.h"
#include <algorithm>
#include <openssl/sha.h>

namespace
{
constexpr size_t HEADER = 148;
void put(std::vector<uint8_t> &out, size_t at, uint64_t value, size_t count)
{
	for (size_t i = 0; i < count; ++i)
		out[at + i] = static_cast<uint8_t>(value >> (8 * i));
}
uint64_t get(std::span<const uint8_t> in, size_t at, size_t count)
{
	uint64_t value = 0;
	for (size_t i = 0; i < count; ++i)
		value |= static_cast<uint64_t>(in[at + i]) << (8 * i);
	return value;
}
bool canonical_body(std::span<const uint8_t> bytes, std::vector<player_item_snapshot> *out)
{
	std::vector<player_item_snapshot> items;
	std::vector<uint8_t> encoded;
	if (player_item_snapshot_list_decode(bytes.data(), bytes.size(), &items) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(items, &encoded) != player_snapshot_codec_result::ok ||
	    encoded.size() != bytes.size() ||
	    !std::equal(encoded.begin(), encoded.end(), bytes.begin()))
		return false;
	if (out)
		*out = std::move(items);
	return true;
}
bool receipt_valid(const held_retirement_receipt &receipt)
{
	if (receipt.result_size > receipt.result.size())
		return false;
	if (!receipt.present)
		return receipt.outcome == critical_apply_outcome::applied &&
		       !receipt.durable_revision && !receipt.error_code &&
		       receipt.failure_stage == critical_failure_stage::none &&
		       !receipt.result_size &&
		       std::all_of(receipt.result.begin(), receipt.result.end(),
				   [](uint8_t v) { return !v; });
	return (receipt.outcome == critical_apply_outcome::applied ||
		receipt.outcome == critical_apply_outcome::already_applied ||
		receipt.outcome == critical_apply_outcome::terminal_failure) &&
	       receipt.failure_stage == critical_failure_stage::none &&
	       std::all_of(receipt.result.begin() + receipt.result_size, receipt.result.end(),
			   [](uint8_t v) { return !v; });
}
}

bool held_retirement_command_identity(const critical_command &command, item_transfer_payload *out,
				      lockpick_retirement_terms *terms_out) noexcept
try
{
	item_transfer_payload payload{};
	lockpick_retirement_terms terms;
	economic_frozen_intent intent;
	if (!command.publication_required || !command.accepted_at_usec ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command) ||
	    !item_transfer_accounting_command_supported(command) ||
	    !item_transfer_command_decode_payload(command, &payload) ||
	    payload.continuation.kind != static_cast<item_transfer_continuation_kind>(8) ||
	    !lockpick_retirement_payload_valid(payload) ||
	    !lockpick_retirement_decode(payload.continuation.data, &terms) ||
	    economic_intent_decode(command.accounting_intent, &intent) !=
		    economic_accounting_error::ok ||
	    economic_intent_verify_binding(command, intent) != economic_accounting_error::ok ||
	    !intent.admission.metadata.source_event ||
	    intent.admission.metadata.source_event->kind !=
		    economic_source_kind::intentional_destruction ||
	    intent.admission.metadata.source_event->sequence != terms.item_uid ||
	    intent.admission.metadata.source_event->slot !=
		    static_cast<uint32_t>(terms.item_vnum) ||
	    intent.admission.metadata.actor_id != terms.actor_pid)
		return false;
	if (out)
		*out = std::move(payload);
	if (terms_out)
		*terms_out = terms;
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_body_pair(const item_transfer_payload &payload,
			       std::span<const uint8_t> before,
			       std::vector<uint8_t> *after) noexcept
try
{
	if (!after || !lockpick_retirement_payload_valid(payload))
		return false;
	std::vector<player_item_snapshot> full, selected, remaining;
	std::vector<uint8_t> selected_bytes, after_bytes;
	if (!canonical_body(before, &full) ||
	    player_item_snapshot_extract_subtree(full, payload.selected_item_uid, &selected,
						 &remaining) != player_snapshot_codec_result::ok ||
	    selected.size() != 1 || selected[0].equipment_slot != HOLD + 1 ||
	    player_item_snapshot_list_encode(selected, &selected_bytes) !=
		    player_snapshot_codec_result::ok ||
	    selected_bytes.size() != payload.item_blob_size ||
	    !std::equal(selected_bytes.begin(), selected_bytes.end(), payload.item_blob.begin()) ||
	    player_item_snapshot_list_encode(remaining, &after_bytes) !=
		    player_snapshot_codec_result::ok)
		return false;
	*after = std::move(after_bytes);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_encode(const critical_command &command,
				     const held_retirement_recovery &context,
				     std::vector<uint8_t> *output) noexcept
try
{
	if (!output || !context.save_revision || context.save_revision == UINT64_MAX ||
	    context.physical_stage > 2 || (!context.receipt.present && context.physical_stage) ||
	    !receipt_valid(context.receipt))
		return false;
	item_transfer_payload payload{};
	std::vector<uint8_t> frozen, before, after, expected_after;
	if (!held_retirement_command_identity(command, &payload) ||
	    critical_command_encode(command, &frozen) != critical_command_codec_result::ok ||
	    player_item_snapshot_list_encode(context.before, &before) !=
		    player_snapshot_codec_result::ok ||
	    player_item_snapshot_list_encode(context.after, &after) !=
		    player_snapshot_codec_result::ok ||
	    !held_retirement_body_pair(payload, before, &expected_after) || after != expected_after)
		return false;
	size_t bytes = HEADER;
	for (size_t size :
	     { before.size(), after.size(), static_cast<size_t>(context.receipt.result_size) })
	{
		if (size > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES - bytes)
			return false;
		bytes += size;
	}
	std::vector<uint8_t> encoded(bytes, 0);
	std::copy_n(reinterpret_cast<const uint8_t *>("HRT1"), 4, encoded.begin());
	encoded[4] = 1;
	encoded[5] = context.physical_stage;
	encoded[6] = context.receipt.present;
	encoded[7] = static_cast<uint8_t>(context.receipt.outcome);
	put(encoded, 8, context.save_revision, 8);
	put(encoded, 16, before.size(), 4);
	put(encoded, 20, after.size(), 4);
	put(encoded, 24, context.receipt.durable_revision, 8);
	put(encoded, 32, context.receipt.error_code, 4);
	put(encoded, 36, context.receipt.result_size, 2);
	encoded[38] = static_cast<uint8_t>(context.receipt.failure_stage);
	SHA256(frozen.data(), frozen.size(), encoded.data() + 40);
	SHA256(before.data(), before.size(), encoded.data() + 72);
	SHA256(after.data(), after.size(), encoded.data() + 104);
	// Reserved tail is zero. Full original bodies, then exact delivered result.
	auto at = std::copy(before.begin(), before.end(), encoded.begin() + HEADER);
	at = std::copy(after.begin(), after.end(), at);
	std::copy_n(context.receipt.result.begin(), context.receipt.result_size, at);
	*output = std::move(encoded);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_decode(const critical_command &command,
				     std::span<const uint8_t> bytes,
				     held_retirement_recovery *output) noexcept
try
{
	if (!output || bytes.size() < HEADER ||
	    bytes.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
	    !std::equal(bytes.begin(), bytes.begin() + 4,
			reinterpret_cast<const uint8_t *>("HRT1")) ||
	    bytes[4] != 1 || bytes[5] > 2 || bytes[6] > 1 || bytes[39] ||
	    std::any_of(bytes.begin() + 136, bytes.begin() + HEADER,
			[](uint8_t v) { return v != 0; }))
		return false;
	const size_t before_size = get(bytes, 16, 4), after_size = get(bytes, 20, 4),
		     result_size = get(bytes, 36, 2);
	if (before_size > bytes.size() - HEADER ||
	    after_size > bytes.size() - HEADER - before_size ||
	    result_size != bytes.size() - HEADER - before_size - after_size ||
	    result_size > CRITICAL_COMPLETION_RESULT_MAX_BYTES)
		return false;
	held_retirement_recovery decoded;
	decoded.save_revision = get(bytes, 8, 8);
	decoded.physical_stage = bytes[5];
	decoded.receipt.present = bytes[6];
	decoded.receipt.outcome = static_cast<critical_apply_outcome>(bytes[7]);
	decoded.receipt.durable_revision = get(bytes, 24, 8);
	decoded.receipt.error_code = static_cast<uint32_t>(get(bytes, 32, 4));
	decoded.receipt.result_size = static_cast<uint16_t>(result_size);
	decoded.receipt.failure_stage = static_cast<critical_failure_stage>(bytes[38]);
	if (!canonical_body(bytes.subspan(HEADER, before_size), &decoded.before) ||
	    !canonical_body(bytes.subspan(HEADER + before_size, after_size), &decoded.after))
		return false;
	std::copy_n(bytes.begin() + HEADER + before_size + after_size, result_size,
		    decoded.receipt.result.begin());
	std::vector<uint8_t> canonical;
	if (!held_retirement_recovery_encode(command, decoded, &canonical) ||
	    canonical.size() != bytes.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = std::move(decoded);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_receipt_matches(const held_retirement_receipt &receipt,
					      const critical_completion &completion) noexcept
{
	return receipt.present && receipt_valid(receipt) &&
	       critical_completion_disposition_valid(completion) &&
	       completion.disposition == critical_completion_disposition::execution &&
	       receipt.outcome == completion.outcome &&
	       receipt.durable_revision == completion.durable_revision &&
	       receipt.error_code == completion.error_code &&
	       receipt.failure_stage == completion.failure_stage &&
	       receipt.result_size == completion.result_size &&
	       std::equal(receipt.result.begin(), receipt.result.end(),
			  completion.result_payload.begin());
}

bool held_retirement_recovery_transition_valid(
	const critical_native_recovery_envelope &original,
	const critical_native_recovery_envelope &successor) noexcept
try
{
	std::vector<uint8_t> a, b;
	held_retirement_recovery before, after;
	if (!original.revision || original.revision == UINT64_MAX ||
	    successor.revision != original.revision + 1 ||
	    original.phase != critical_native_recovery_phase::execution_pending ||
	    successor.phase != original.phase ||
	    critical_command_encode(original.command, &a) != critical_command_codec_result::ok ||
	    critical_command_encode(successor.command, &b) != critical_command_codec_result::ok ||
	    a != b ||
	    !held_retirement_recovery_decode(original.command, original.attachment, &before) ||
	    !held_retirement_recovery_decode(successor.command, successor.attachment, &after) ||
	    before.save_revision != after.save_revision ||
	    after.physical_stage < before.physical_stage ||
	    after.physical_stage > before.physical_stage + 1 ||
	    (before.receipt.present && before.receipt != after.receipt))
		return false;
	std::vector<uint8_t> old_body, new_body;
	for (const auto *pair : { &before.before, &before.after })
	{
		const auto &target = pair == &before.before ? after.before : after.after;
		if (player_item_snapshot_list_encode(*pair, &old_body) !=
			    player_snapshot_codec_result::ok ||
		    player_item_snapshot_list_encode(target, &new_body) !=
			    player_snapshot_codec_result::ok ||
		    old_body != new_body)
			return false;
	}
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_publication_context_valid(
	const critical_native_recovery_envelope &envelope,
	const critical_completion &completion) noexcept
{
	held_retirement_recovery context;
	return envelope.revision &&
	       envelope.phase == critical_native_recovery_phase::execution_pending &&
	       envelope.command.operation_id.bytes == completion.operation_id.bytes &&
	       held_retirement_recovery_decode(envelope.command, envelope.attachment, &context) &&
	       context.physical_stage == 2 &&
	       held_retirement_recovery_receipt_matches(context.receipt, completion);
}

bool held_retirement_publication_snapshot_valid(
	const critical_command &command, const critical_completion &completion,
	const held_retirement_publication_snapshot &snapshot) noexcept
try
{
	std::vector<uint8_t> supplied, retained;
	held_retirement_recovery context;
	if (!critical_completion_disposition_valid(completion) ||
	    completion.operation_id.bytes != command.operation_id.bytes ||
	    !snapshot.envelope.revision ||
	    snapshot.envelope.phase != critical_native_recovery_phase::execution_pending ||
	    critical_command_encode(command, &supplied) != critical_command_codec_result::ok ||
	    critical_command_encode(snapshot.envelope.command, &retained) !=
		    critical_command_codec_result::ok ||
	    supplied != retained ||
	    !held_retirement_recovery_decode(command, snapshot.envelope.attachment, &context))
		return false;
	switch (snapshot.mode)
	{
	case held_retirement_publication_mode::original_refusal:
		return completion.disposition == critical_completion_disposition::never_admitted &&
		       !context.receipt.present && context.physical_stage == 0;
	case held_retirement_publication_mode::terminal_ack_retry:
		return held_retirement_recovery_publication_context_valid(snapshot.envelope,
									  completion);
	case held_retirement_publication_mode::execution:
		return completion.disposition == critical_completion_disposition::execution &&
		       (!context.receipt.present ||
			held_retirement_recovery_receipt_matches(context.receipt, completion));
	}
	return false;
}
catch (...)
{
	return false;
}

#include <cerrno>
#include <limits>
#include <functional>
#include <iterator>
namespace
{
constexpr size_t recovery_library_allocator_frames =
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
constexpr size_t recovery_library_copy_frames =
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
constexpr size_t recovery_library_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t recovery_library_default_frames =
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
	2 * (3 * sizeof(void *)) + sizeof(uint8_t);
constexpr size_t recovery_library_vector_frames =
	recovery_library_allocator_frames + recovery_library_copy_frames +
	recovery_library_relocate_frames + recovery_library_default_frames +
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
constexpr size_t recovery_library_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + recovery_library_allocator_frames;

// Nontrivial row/description construction and destruction are source scopes,
// not heap metadata. The nested member objects already live in sizeof(row).
constexpr size_t recovery_library_nontrivial_frames =
	// default_n_1<false>: first/n/cur/return; _Construct/addressof/placement.
	3 * sizeof(void *) + sizeof(size_t) + 5 * sizeof(void *) + sizeof(size_t) +
	// Actual aggregate row and description this, four row strings and two
	// description strings: string()/allocator hider/use-local-data/set-length.
	2 * sizeof(void *) +
	6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) + sizeof(std::allocator<char>)) +
	// row/description nested vector()/Vector_base()/Vector_impl()/data() and
	// allocator return carriers. Three source member vector types.
	3 * (5 * sizeof(void *) + sizeof(std::allocator<int32_t>)) +
	// Nontrivial _Destroy range/aux::__destroy/destroy_at/__addressof; actual
	// row/description destructor this then six string destructors/dispose/
	// _M_is_local/_M_destroy and three nested vector destroy/deallocate scopes.
	8 * sizeof(void *) + 2 * sizeof(size_t) + 2 * sizeof(void *) +
	6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
	3 * recovery_library_allocator_frames;
// Fitting _M_replace calls _M_disjunct(this,s). Both actual less pointer
// temporaries can coexist through the full || expression; their operator()
// has this/x/y/result and is_constant_evaluated result. Data/size queries.
constexpr size_t recovery_library_disjunct_frames =
	2 * sizeof(void *) + sizeof(bool) + 2 * sizeof(std::less<const char *>) +
	2 * (3 * sizeof(void *) + 2 * sizeof(bool)) + 2 * (2 * sizeof(void *)) + sizeof(void *) +
	sizeof(size_t);
constexpr size_t recovery_library_string_frames =
	recovery_library_disjunct_frames +
	// assign(s,n): this/s/n/ref-return; _M_replace(this,pos,len1,s,len2),
	// old_size/new_size/p/how_much/ref-return, actual length checks/queries.
	3 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + 5 * sizeof(size_t) +
	6 * (sizeof(void *) + sizeof(size_t)) + sizeof(bool) +
	// _M_mutate(this,pos,len1,s,len2), how_much/new_capacity/r;
	// _M_create(this,capacityref,oldcapacity), max_size, allocation return.
	3 * sizeof(void *) + 5 * sizeof(size_t) + 3 * sizeof(void *) + sizeof(size_t) +
	recovery_library_allocator_frames +
	// _S_copy(d,s,n), traits::copy(s1,s2,n) returned pointer and memcopy
	// argument/result carriers; one-character assign reference/char scopes.
	2 * (3 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	2 * sizeof(void *) + sizeof(char) +
	// old block dispose/destroy plus data/capacity/set-length and final NUL.
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) + sizeof(char);
// Genuine vector(n,value,allocator) constructor scopes, before fill:
// vector this/n/value-reference/allocator-reference and default allocator;
// _S_check_init_len n/a/result and its _Tp allocator copy; _Vector_base
// this/n/a, _Vector_impl this/a and allocator copy, _Vector_impl_data this;
// _M_create_storage this/n. Existing allocator profile owns _S_max_size.
constexpr size_t recovery_library_size_constructor_frames =
	3 * sizeof(void *) + sizeof(size_t) + sizeof(std::allocator<size_t>) + sizeof(void *) +
	2 * sizeof(size_t) + sizeof(std::allocator<size_t>) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) +
	sizeof(size_t);

// Actual codec linked owner/row-vector observation and relay source scopes.
// These declared carriers complement the real T-specific CURRENT profiles.
constexpr size_t recovery_owner_source_frames =
	// add/rows/capacity + byte/row/payload/intent/context overload arguments,
	// genuine const row range iterators, row reference and current-heap scalar.
	sizeof(void *) + sizeof(size_t) + sizeof(bool) + 4 * sizeof(void *) + sizeof(size_t) +
	sizeof(bool) + 10 * sizeof(void *) +
	2 * sizeof(std::vector<player_item_snapshot>::const_iterator) + 2 * sizeof(size_t) +
	4 * sizeof(bool) +
	// current(this,out), bytes and actual linked cursor; prefix/peak this,
	// outputs, requests/frames/bytes plus callback arguments/results.
	3 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + 5 * sizeof(void *) +
	7 * sizeof(size_t) + 4 * sizeof(bool) +
	// invoke this/F-reference/failure/frames/local value, actual lambda prefix;
	// watcher ctor this/owner/value and destructor this; observe formal pair.
	2 * sizeof(void *) + sizeof(bool) + 3 * sizeof(size_t) + 6 * sizeof(void *) + sizeof(bool);
}

namespace
{
// Actual installed span/array/initializer_list source constructor/accessors.
// The view object itself is a real caller local,priced separately.
constexpr size_t recovery_library_view_frames =
	// span(pointer,count) this/first/count; to_address/__to_address params+results;
	// dynamic extent constructor and accessor each actual receiver/size value.
	2 * sizeof(void *) + sizeof(size_t) + 4 * sizeof(void *) +
	2 * (sizeof(void *) + sizeof(size_t)) +
	// span default/array/converting constructors,actual reference and extent;
	// begin/end actual normal_iterator return and ctor source references.
	4 * sizeof(void *) + sizeof(size_t) + 2 * (4 * sizeof(void *)) +
	// subspan this/offset/count/returned span; size/empty/data/index receiver/
	// result,where size reaches the real extent accessor.
	sizeof(void *) + 2 * sizeof(size_t) + sizeof(std::span<const uint8_t>) +
	6 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool) +
	// array begin/end/data/_S_ptr and actual operator[]/_S_ref argument/results.
	4 * (4 * sizeof(void *)) + 3 * sizeof(void *) + sizeof(size_t) +
	// initializer_list private compiler ctor and member begin/end/size.
	2 * sizeof(void *) + sizeof(size_t) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	sizeof(size_t);
}
// Private source candidate: source-carrier closure still requires independent
// review. The original selected codecs are not changed by this companion.
namespace
{
using recovery_reserve = bool (*)(size_t, void *) noexcept;
bool recovery_add(size_t &bytes, size_t value) noexcept
{
	if (value > SIZE_MAX - bytes)
	{
		errno = EOVERFLOW;
		return false;
	}
	bytes += value;
	return true;
}
template <class T> bool recovery_rows(size_t &bytes, const std::vector<T> &value) noexcept
{
	if (value.capacity() > SIZE_MAX / sizeof(T))
	{
		errno = EOVERFLOW;
		return false;
	}
	return recovery_add(bytes, value.capacity() * sizeof(T));
}
bool recovery_heap(const std::vector<uint8_t> &value, size_t &bytes) noexcept
{
	return recovery_rows(bytes, value);
}
bool recovery_heap(const std::vector<player_item_snapshot> &value, size_t &bytes) noexcept
{
	if (!recovery_rows(bytes, value))
		return false;
	for (const auto &row : value)
	{
		size_t heap = 0;
		if (!player_item_snapshot_current_heap_bytes(row, &heap) ||
		    !recovery_add(bytes, heap))
			return false;
	}
	return true;
}
bool recovery_heap(const item_transfer_payload &value, size_t &bytes) noexcept
{
	size_t heap = 0;
	return item_transfer_payload_current_heap_bytes(value, &heap) && recovery_add(bytes, heap);
}
bool recovery_heap(const economic_frozen_intent &value, size_t &bytes) noexcept
{
	return recovery_rows(bytes, value.admission.facts);
}
bool recovery_heap(const held_retirement_recovery &value, size_t &bytes) noexcept
{
	return recovery_heap(value.before, bytes) && recovery_heap(value.after, bytes);
}
struct recovery_codec_scope;
struct recovery_codec_live
{
	recovery_codec_scope &scope;
	recovery_codec_live *prior;
	const void *object;
	size_t inline_bytes;
	bool (*observe)(const void *, size_t &) noexcept;
	template <class T> recovery_codec_live(recovery_codec_scope &, const T &) noexcept;
	~recovery_codec_live();
};
struct recovery_codec_scope
{
	template <class F, class R> R invoke_codec(F &&body, R failure, size_t frames) noexcept
	{
		size_t value = 0;
		return peak(held_retirement_codec_entry_inline_bytes(), frames) &&
				       prefix(&value, frames) ?
			       body(value) :
			       failure;
	}
	template <class F, class R> R invoke(F &&body, R failure, size_t frames) noexcept
	{
		size_t value = 0;
		return prefix(&value, frames) ? body(value) : failure;
	}
	recovery_reserve reserve;
	void *context;
	size_t outer;
	recovery_codec_live *live = nullptr;
	bool current(size_t *output) const noexcept
	{
		size_t bytes = outer;
		if (!output || !recovery_add(bytes, sizeof(*this)))
			return false;
		for (const auto *entry = live; entry; entry = entry->prior)
			if (!recovery_add(bytes, sizeof(*entry)) ||
			    !recovery_add(bytes, entry->inline_bytes) ||
			    !entry->observe(entry->object, bytes))
				return false;
		*output = bytes;
		return true;
	}
	bool prefix(size_t *output, size_t frames) noexcept
	{
		size_t bytes = 0;
		if (!current(&bytes) || !recovery_add(bytes, frames) || !reserve ||
		    !reserve(bytes, context))
		{
			errno = ENOBUFS;
			return false;
		}
		*output = bytes;
		return true;
	}
	bool peak(size_t requests, size_t frames) noexcept
	{
		size_t bytes = 0;
		if (!current(&bytes) || !recovery_add(bytes, requests) ||
		    !recovery_add(bytes, frames) || !reserve || !reserve(bytes, context))
		{
			errno = ENOBUFS;
			return false;
		}
		return true;
	}
};
template <class T>
recovery_codec_live::recovery_codec_live(recovery_codec_scope &owner, const T &value) noexcept
	: scope(owner)
	, prior(owner.live)
	, object(&value)
	, inline_bytes(sizeof(T))
	, observe([](const void *pointer, size_t &bytes) noexcept
		  { return recovery_heap(*static_cast<const T *>(pointer), bytes); })
{
	owner.live = this;
}
recovery_codec_live::~recovery_codec_live()
{
	scope.live = prior;
}
}

// Genuine codec lexical and library source storage, separate from buffers.
// Each subtotal names reached original/helper scopes. Conservative sums across
// sibling scopes are source storage, never heap margins or lower-provider fees.
size_t held_retirement_codec_source_frame_bytes() noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return SIZE_MAX;
#else

	using byte_iterator = std::vector<uint8_t>::iterator;
	const size_t identity = 5 * sizeof(void *) + sizeof(size_t) + sizeof(bool) +
				sizeof(lockpick_retirement_terms) + sizeof(void *);
	const size_t canonical = sizeof(std::span<const uint8_t>) + 3 * sizeof(void *) +
				 sizeof(size_t) + sizeof(bool);
	const size_t pair = sizeof(std::span<const uint8_t>) + 4 * sizeof(void *) + sizeof(size_t) +
			    sizeof(bool) + 2 * sizeof(void *);
	const size_t encode = 5 * sizeof(void *) + sizeof(size_t) + sizeof(bool) + sizeof(size_t) +
			      3 * sizeof(size_t) + sizeof(std::initializer_list<size_t>) +
			      2 * sizeof(const size_t *) + sizeof(size_t) + sizeof(byte_iterator);
	const size_t decode = 4 * sizeof(void *) + sizeof(size_t) +
			      sizeof(std::span<const uint8_t>) + sizeof(bool) + 3 * sizeof(size_t) +
			      sizeof(void *);
	// put out/at/value/count/i and vector[] receiver/index/ref-return;
	// get span/at/count/value/i and span[] receiver/index/ref-return.
	const size_t integer = sizeof(void *) + 3 * sizeof(size_t) + sizeof(uint64_t) +
			       2 * sizeof(void *) + sizeof(size_t) +
			       sizeof(std::span<const uint8_t>) + 3 * sizeof(size_t) +
			       sizeof(uint64_t) + 2 * sizeof(void *) + sizeof(size_t);
	// Actual nonallocating lockpick decoder: span/output,decoded terms/vnum,
	// read_le span/offset/count/value/index,valid terms/result. No encode call.
	const size_t lockpick = sizeof(std::span<const uint8_t>) + sizeof(void *) + sizeof(bool) +
				sizeof(lockpick_retirement_terms) + sizeof(uint64_t) +
				sizeof(std::span<const uint8_t>) + 3 * sizeof(size_t) +
				sizeof(uint64_t) + sizeof(void *) + sizeof(bool);
	// receipt_valid/predicates; any_of/all_of->find_if(_not),copy/copy_n and
	// vector/array equality instantiate authentic contiguous-byte algorithms.
	const size_t algorithms =
		sizeof(void *) + sizeof(bool) +
		2 * (sizeof(char) + sizeof(uint8_t) + sizeof(void *) + sizeof(bool)) +
		recovery_library_copy_frames + 4 * (3 * sizeof(void *) + sizeof(bool)) +
		2 * sizeof(std::ptrdiff_t) + 3 * sizeof(void *) +
		sizeof(std::random_access_iterator_tag) + 3 * sizeof(char) + 8 * sizeof(void *) +
		4 * sizeof(bool) + sizeof(uint8_t) +
		2 * (2 * sizeof(void *) + sizeof(size_t) + sizeof(int)) + sizeof(bool) +
		sizeof(std::ptrdiff_t) + 2 * sizeof(bool);
	// Actual span constructor/size/data/begin/end/subspan and array accessors.
	const size_t views =
		2 * sizeof(void *) + sizeof(size_t) + 5 * (sizeof(void *) + sizeof(size_t)) +
		2 * (sizeof(void *) + sizeof(void *)) + sizeof(void *) + 2 * sizeof(size_t) +
		sizeof(std::span<const uint8_t>) + 4 * (2 * sizeof(void *));
	// Actual payload six strings/five vectors; intent facts vector; held two
	// row vectors. Empty constructors allocate none. Element objects remain
	// owned by the authentic watcher/request, not by these source scopes.
	const size_t defaults = 7 * sizeof(void *) +
				6 * (6 * sizeof(void *) + sizeof(size_t) + sizeof(char) +
				     sizeof(std::allocator<char>)) +
				8 * (5 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
				6 * (5 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool)) +
				8 * recovery_library_allocator_frames;
	return identity + canonical + pair + encode + decode + integer + lockpick + algorithms +
	       views + defaults + critical_command_valid_frame_bytes() +
	       player_item_snapshot_current_heap_observer_frame_bytes() +
	       item_transfer_payload_current_heap_observer_frame_bytes() +
	       held_retirement_codec_source_profile_query_frames() + recovery_owner_source_frames +
	       // Actual transient invoke_codec formals/local, two admissions and entry getter.
	       4 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
	       recovery_library_view_frames + recovery_library_vector_frames +
	       recovery_library_move_frames + recovery_library_size_constructor_frames;
#endif
}

namespace
{
// Genuine initial watched objects, before the first callback. Terms and all
// unwatched lexical objects already belong to the paired source inventory.
constexpr size_t held_entry_identity =
	sizeof(recovery_codec_scope) + sizeof(item_transfer_payload) +
	sizeof(economic_frozen_intent) + 2 * sizeof(recovery_codec_live);
constexpr size_t held_entry_pair =
	sizeof(recovery_codec_scope) + 3 * sizeof(std::vector<player_item_snapshot>) +
	2 * sizeof(std::vector<uint8_t>) + 5 * sizeof(recovery_codec_live);
constexpr size_t held_entry_encode = sizeof(recovery_codec_scope) + sizeof(item_transfer_payload) +
				     4 * sizeof(std::vector<uint8_t>) +
				     5 * sizeof(recovery_codec_live);
constexpr size_t held_entry_decode = sizeof(recovery_codec_scope) +
				     sizeof(held_retirement_recovery) + sizeof(recovery_codec_live);
constexpr size_t held_entry_a = held_entry_identity > held_entry_pair ? held_entry_identity :
									held_entry_pair;
constexpr size_t held_entry_b = held_entry_encode > held_entry_decode ? held_entry_encode :
									held_entry_decode;
constexpr size_t held_entry_max = held_entry_a > held_entry_b ? held_entry_a : held_entry_b;
// Canonical's scope/two vectors/two watchers are contained in pair's exact
// larger real shape. This maximum is prospective only; never a cached census.
}
size_t held_retirement_codec_entry_inline_bytes() noexcept
{
	return held_entry_max;
}

namespace
{
// Exact fixed SHA source carriers inherited from economic fixed-digest source
// controls; this is additive, not a change to the selected one-shot encoder.
constexpr size_t held_sha_frames =
	std::max(size_t(2 * 4 * 64 + 4 * sizeof(void *) + 6 * sizeof(uint64_t) + (256 * 4 - 1) +
			2 * sizeof(void *)),
		 std::max(size_t(16 * sizeof(unsigned int) + 13 * sizeof(unsigned int) +
				 sizeof(int) + sizeof(const uint8_t *)),
			  size_t(27 * sizeof(unsigned int) + 2 * sizeof(int) +
				 2 * sizeof(void *)))) +
	std::max(size_t(sizeof(void *) + sizeof(int)),
		 std::max(size_t(4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(unsigned int) +
				 sizeof(int)),
			  size_t(3 * sizeof(void *) + sizeof(size_t) + sizeof(unsigned long) +
				 sizeof(unsigned int) + sizeof(int))));
bool held_fixed_digest(recovery_codec_scope &scope, const std::vector<uint8_t> &bytes,
		       uint8_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && !defined(_WIN32) && defined(_GLIBCXX_RELEASE) && \
	_GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && _GLIBCXX_USE_CXX11_ABI &&    \
	!defined(_GLIBCXX_DEBUG) && defined(OPENSSL_VERSION_MAJOR) &&                             \
	OPENSSL_VERSION_MAJOR == 3 && defined(OPENSSL_VERSION_MINOR) &&                           \
	OPENSSL_VERSION_MINOR == 0 && defined(OPENSSL_VERSION_PATCH) &&                           \
	OPENSSL_VERSION_PATCH == 13 && !defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!scope.peak(sizeof(SHA256_CTX) + 2 * sizeof(void *) + sizeof(size_t) + sizeof(int),
			held_sha_frames))
		return false;
	SHA256_CTX digest;
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
	const bool result = SHA256_Init(&digest) == 1 &&
			    SHA256_Update(&digest, bytes.data(), bytes.size()) == 1 &&
			    SHA256_Final(output, &digest) == 1;
#pragma GCC diagnostic pop
	return result;
#else
	(void)scope;
	(void)bytes;
	(void)output;
	errno = ENOTSUP;
	return false;
#endif
}
}

namespace
{
bool held_canonical_body_bounded(std::span<const uint8_t> bytes,
				 std::vector<player_item_snapshot> *out, recovery_reserve reserve,
				 void *context, size_t outer)
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return false;
#endif

	std::vector<player_item_snapshot> items;
	std::vector<uint8_t> encoded;
	recovery_codec_scope scope{ reserve, context, outer };
	recovery_codec_live items_live(scope, items), encoded_live(scope, encoded);
	if (scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return player_item_snapshot_list_decode_bounded(
				    bytes.data(), bytes.size(), &items, reserve, context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    scope.invoke(
		    [&](size_t prefix) noexcept {
			    return player_item_snapshot_list_encode_bounded(
				    items, &encoded, reserve, context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    encoded.size() != bytes.size() ||
	    !std::equal(encoded.begin(), encoded.end(), bytes.begin()))
		return false;
	if (out)
		*out = std::move(items);
	return true;
}
}
bool held_retirement_command_identity_bounded(const critical_command &command,
					      item_transfer_payload *out,
					      lockpick_retirement_terms *terms_out,
					      recovery_reserve reserve, void *context,
					      size_t outer) noexcept
try
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return false;
#endif

	item_transfer_payload payload{};
	lockpick_retirement_terms terms;
	economic_frozen_intent intent;
	recovery_codec_scope scope{ reserve, context, outer };
	recovery_codec_live payload_live(scope, payload), intent_live(scope, intent);
	if (!scope.peak(0, held_retirement_codec_source_frame_bytes()))
		return false;
	if (!command.publication_required || !command.accepted_at_usec ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command) ||
	    !scope.invoke(
		    [&](size_t prefix) noexcept {
			    return item_transfer_accounting_command_supported_bounded(
				    command, reserve, context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    !scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return item_transfer_command_decode_payload_bounded(
				    command, &payload, reserve, context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    payload.continuation.kind != static_cast<item_transfer_continuation_kind>(8) ||
	    !scope.invoke(
		    [&](size_t prefix) noexcept {
			    return lockpick_retirement_payload_valid_bounded(payload, reserve,
									     context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    !lockpick_retirement_decode(payload.continuation.data, &terms) ||
	    scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return economic_intent_decode_bounded(
				    command.accounting_intent, &intent, reserve, context, prefix);
		    },
		    economic_accounting_error::capacity,
		    held_retirement_codec_source_frame_bytes()) != economic_accounting_error::ok ||
	    scope.invoke(
		    [&](size_t prefix) noexcept {
			    return economic_intent_verify_binding_bounded(command, intent, reserve,
									  context, prefix);
		    },
		    economic_accounting_error::capacity,
		    held_retirement_codec_source_frame_bytes()) != economic_accounting_error::ok ||
	    !intent.admission.metadata.source_event ||
	    intent.admission.metadata.source_event->kind !=
		    economic_source_kind::intentional_destruction ||
	    intent.admission.metadata.source_event->sequence != terms.item_uid ||
	    intent.admission.metadata.source_event->slot !=
		    static_cast<uint32_t>(terms.item_vnum) ||
	    intent.admission.metadata.actor_id != terms.actor_pid)
		return false;
	if (out)
		*out = std::move(payload);
	if (terms_out)
		*terms_out = terms;
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_body_pair_bounded(const item_transfer_payload &payload,
				       std::span<const uint8_t> before, std::vector<uint8_t> *after,
				       recovery_reserve reserve, void *context,
				       size_t outer) noexcept
try
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return false;
#endif

	recovery_codec_scope scope{ reserve, context, outer };
	if (!after || !scope.invoke(
			      [&](size_t prefix) noexcept {
				      return lockpick_retirement_payload_valid_bounded(
					      payload, reserve, context, prefix);
			      },
			      false, held_retirement_codec_source_frame_bytes()))
		return false;
	std::vector<player_item_snapshot> full, selected, remaining;
	std::vector<uint8_t> selected_bytes, after_bytes;
	recovery_codec_live full_live(scope, full), selected_live(scope, selected),
		remaining_live(scope, remaining), selected_bytes_live(scope, selected_bytes),
		after_bytes_live(scope, after_bytes);
	if (!scope.invoke_codec(
		    [&](size_t prefix) noexcept {
			    return held_canonical_body_bounded(before, &full, reserve, context,
							       prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return player_item_snapshot_extract_subtree_bounded(
				    full, payload.selected_item_uid, &selected, &remaining, reserve,
				    context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    selected.size() != 1 || selected[0].equipment_slot != HOLD + 1 ||
	    scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return player_item_snapshot_list_encode_bounded(
				    selected, &selected_bytes, reserve, context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    selected_bytes.size() != payload.item_blob_size ||
	    !std::equal(selected_bytes.begin(), selected_bytes.end(), payload.item_blob.begin()) ||
	    scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return player_item_snapshot_list_encode_bounded(
				    remaining, &after_bytes, reserve, context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) != player_snapshot_codec_result::ok)
		return false;
	*after = std::move(after_bytes);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_encode_bounded(const critical_command &command,
					     const held_retirement_recovery &value,
					     std::vector<uint8_t> *output, recovery_reserve reserve,
					     void *context, size_t outer) noexcept
try
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return false;
#endif

	if (!output || !value.save_revision || value.save_revision == UINT64_MAX ||
	    value.physical_stage > 2 || (!value.receipt.present && value.physical_stage) ||
	    !receipt_valid(value.receipt))
		return false;
	item_transfer_payload payload{};
	std::vector<uint8_t> frozen, before, after, expected_after;
	recovery_codec_scope scope{ reserve, context, outer };
	recovery_codec_live payload_live(scope, payload), frozen_live(scope, frozen),
		before_live(scope, before), after_live(scope, after),
		expected_live(scope, expected_after);
	if (!scope.invoke_codec(
		    [&](size_t prefix) noexcept
		    {
			    return held_retirement_command_identity_bounded(
				    command, &payload, nullptr, reserve, context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    scope.invoke(
		    [&](size_t prefix) noexcept {
			    return critical_command_encode_bounded(command, &frozen, reserve,
								   context, prefix);
		    },
		    critical_command_codec_result::overflow,
		    held_retirement_codec_source_frame_bytes()) !=
		    critical_command_codec_result::ok ||
	    scope.invoke(
		    [&](size_t prefix) noexcept
		    {
			    return player_item_snapshot_list_encode_bounded(
				    value.before, &before, reserve, context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    scope.invoke(
		    [&](size_t prefix) noexcept {
			    return player_item_snapshot_list_encode_bounded(
				    value.after, &after, reserve, context, prefix);
		    },
		    player_snapshot_codec_result::allocation_failure,
		    held_retirement_codec_source_frame_bytes()) !=
		    player_snapshot_codec_result::ok ||
	    !scope.invoke_codec(
		    [&](size_t prefix) noexcept
		    {
			    return held_retirement_body_pair_bounded(
				    payload, before, &expected_after, reserve, context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    after != expected_after)
		return false;
	size_t bytes = HEADER;
	for (size_t size :
	     { before.size(), after.size(), static_cast<size_t>(value.receipt.result_size) })
	{
		if (size > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES - bytes)
			return false;
		bytes += size;
	}
	if (!scope.peak(bytes + sizeof(std::vector<uint8_t>),
			held_retirement_codec_source_frame_bytes()))
		return false;
	std::vector<uint8_t> encoded(bytes, 0);
	recovery_codec_live encoded_live(scope, encoded);
	std::copy_n(reinterpret_cast<const uint8_t *>("HRT1"), 4, encoded.begin());
	encoded[4] = 1;
	encoded[5] = value.physical_stage;
	encoded[6] = value.receipt.present;
	encoded[7] = static_cast<uint8_t>(value.receipt.outcome);
	put(encoded, 8, value.save_revision, 8);
	put(encoded, 16, before.size(), 4);
	put(encoded, 20, after.size(), 4);
	put(encoded, 24, value.receipt.durable_revision, 8);
	put(encoded, 32, value.receipt.error_code, 4);
	put(encoded, 36, value.receipt.result_size, 2);
	encoded[38] = static_cast<uint8_t>(value.receipt.failure_stage);
	if (!held_fixed_digest(scope, frozen, encoded.data() + 40))
		return false;
	if (!held_fixed_digest(scope, before, encoded.data() + 72))
		return false;
	if (!held_fixed_digest(scope, after, encoded.data() + 104))
		return false;
	// Reserved tail is zero. Full original bodies, then exact delivered result.
	auto at = std::copy(before.begin(), before.end(), encoded.begin() + HEADER);
	at = std::copy(after.begin(), after.end(), at);
	std::copy_n(value.receipt.result.begin(), value.receipt.result_size, at);
	*output = std::move(encoded);
	return true;
}
catch (...)
{
	return false;
}

bool held_retirement_recovery_decode_bounded(const critical_command &command,
					     std::span<const uint8_t> bytes,
					     held_retirement_recovery *output,
					     recovery_reserve reserve, void *context,
					     size_t outer) noexcept
try
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG) || __cplusplus != 202002L ||         \
	defined(_GLIBCXX_ASSERTIONS) || defined(_GLIBCXX_PARALLEL)
	errno = ENOTSUP;
	return false;
#endif

	if (!output || bytes.size() < HEADER ||
	    bytes.size() > CRITICAL_NATIVE_RECOVERY_MAX_ATTACHMENT_BYTES ||
	    !std::equal(bytes.begin(), bytes.begin() + 4,
			reinterpret_cast<const uint8_t *>("HRT1")) ||
	    bytes[4] != 1 || bytes[5] > 2 || bytes[6] > 1 || bytes[39] ||
	    std::any_of(bytes.begin() + 136, bytes.begin() + HEADER,
			[](uint8_t v) { return v != 0; }))
		return false;
	const size_t before_size = get(bytes, 16, 4), after_size = get(bytes, 20, 4),
		     result_size = get(bytes, 36, 2);
	if (before_size > bytes.size() - HEADER ||
	    after_size > bytes.size() - HEADER - before_size ||
	    result_size != bytes.size() - HEADER - before_size - after_size ||
	    result_size > CRITICAL_COMPLETION_RESULT_MAX_BYTES)
		return false;
	held_retirement_recovery decoded;
	recovery_codec_scope scope{ reserve, context, outer };
	recovery_codec_live decoded_live(scope, decoded);
	if (!scope.peak(0, held_retirement_codec_source_frame_bytes()))
		return false;
	decoded.save_revision = get(bytes, 8, 8);
	decoded.physical_stage = bytes[5];
	decoded.receipt.present = bytes[6];
	decoded.receipt.outcome = static_cast<critical_apply_outcome>(bytes[7]);
	decoded.receipt.durable_revision = get(bytes, 24, 8);
	decoded.receipt.error_code = static_cast<uint32_t>(get(bytes, 32, 4));
	decoded.receipt.result_size = static_cast<uint16_t>(result_size);
	decoded.receipt.failure_stage = static_cast<critical_failure_stage>(bytes[38]);
	if (!scope.invoke_codec(
		    [&](size_t prefix) noexcept
		    {
			    return held_canonical_body_bounded(bytes.subspan(HEADER, before_size),
							       &decoded.before, reserve, context,
							       prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    !scope.invoke_codec(
		    [&](size_t prefix) noexcept
		    {
			    return held_canonical_body_bounded(
				    bytes.subspan(HEADER + before_size, after_size), &decoded.after,
				    reserve, context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()))
		return false;
	std::copy_n(bytes.begin() + HEADER + before_size + after_size, result_size,
		    decoded.receipt.result.begin());
	std::vector<uint8_t> canonical;
	recovery_codec_live canonical_live(scope, canonical);
	if (!scope.invoke_codec(
		    [&](size_t prefix) noexcept
		    {
			    return held_retirement_recovery_encode_bounded(
				    command, decoded, &canonical, reserve, context, prefix);
		    },
		    false, held_retirement_codec_source_frame_bytes()) ||
	    canonical.size() != bytes.size() ||
	    !std::equal(canonical.begin(), canonical.end(), bytes.begin()))
		return false;
	*output = std::move(decoded);
	return true;
}
catch (...)
{
	return false;
}
