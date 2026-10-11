#include "economy/economic_command_admission.h"
#include "economy/auction_repository.h"
#include "economy/auction_native_command_context.h"
#include "economy/native_mobile_birth_command.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "economy/zone_reset_item_command.h"
#include "economy/economic_currency_adapter.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/item_transfer_accounting.h"
#include "item/native_quest_transport.h"
#include "economy/collector_accounting.h"
#include "economy/shop_trade_accounting.h"

#include <new>

namespace
{
uint64_t little_u64(std::span<const uint8_t> input, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(input[offset + byte]) << (8 * byte);
	return value;
}
}

bool economic_shop_trade_admission_available() noexcept
{
	// SHOP has no branch in the maintained schema-2 admission allowlist yet.
	// Change alongside that complete route, never as a producer-side override.
	return false;
}

bool economic_command_admission_supported(const critical_command &command) noexcept
{
	using error = economic_accounting_error;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
	try
	{
		if (command.type == critical_command_type::auction)
		{
#ifdef __NO_MYSQL__
			return false;
#else
			// Original immutable typed support; current SQL/world authority
			// and guarded publication remain with the real native owner.
			return command.publication_required &&
			       command.payload_version == AUCTION_NATIVE_COMMAND_PAYLOAD_VERSION &&
			       auction_repository_frozen_accounting_valid(command);
#endif
		}
		// Immutable type support only. Original source admission and guarded
		// native publication/ACK remain the private birth owner's obligations.
		if (command.type == critical_command_type::native_mobile_birth)
		{
			quest_mobile_native_image original;
			if (command.payload_version ==
			    NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION)
			{
				std::vector<native_mobile_birth_item_recipe> recipes;
				native_mobile_birth_cash_role_recipe role;
				// Complete original NMB4 binding and ordinary role only. Shared
				// birth has no complete atomic owner and stays unregistered.
				return native_mobile_birth_cash_role_command_decode(
					       command, &original, &recipes, &role) == error::ok &&
				       role.role == native_mobile_birth_cash_role::ordinary_wallet;
			}
			return native_mobile_birth_command_decode(command, &original) == error::ok;
		}
		if (command.type == critical_command_type::zone_reset_item_birth)
		{
#ifdef __NO_MYSQL__
			return false;
#else
			// Immutable SQL type support only. Actual reset source admission,
			// current room/season and native publication stay with their owners.
			zone_reset_item_image original;
			return zone_reset_item_command_decode(command, &original) == error::ok;
#endif
		}
		economic_frozen_intent intent;
		if (economic_intent_decode(command.accounting_intent, &intent) != error::ok)
			return false;
		const auto &meta = intent.admission.metadata;
		const auto facts = std::span<const uint8_t>(intent.admission.facts);
		// This projection is used only to regenerate the original immutable
		// intent. Execution always receives the complete schema-2 command.
		auto admission = command;
		admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
		admission.accounting_intent.clear();
		admission.publication_required = false;
		std::vector<uint8_t> expected;
		if (command.type == critical_command_type::account_bank)
		{
			if (facts.size() != ECONOMIC_BANK_FACT_BYTES)
				return false;
			const economic_account_key wallet = { meta.lineage,
							      economic_account_kind::wallet,
							      little_u64(facts, 0), 0 };
			const economic_account_key bank = { meta.lineage,
							    economic_account_kind::bank,
							    little_u64(facts, 8),
							    little_u64(facts, 16) };
			currency_command_payload payload = {};
			if (!currency_command_decode_payload(admission, &payload))
				return false;
			const auto result =
				payload.reason == currency_reason_type::chaos_starter_reward ?
					economic_chaos_starter_bank_intent(
						admission, meta.epoch, wallet, bank, &expected) :
				payload.reason == currency_reason_type::wallet_reward ?
					economic_quest_wallet_reward_intent(
						admission, meta.epoch, wallet, bank, &expected) :
					economic_bank_transfer_intent(admission, meta.epoch, wallet,
								      bank, &expected);
			if (result != error::ok)
				return false;
		}
		else if (command.type == critical_command_type::coin_transfer)
		{
			if (facts.size() != ECONOMIC_COIN_TRANSFER_FACT_BYTES)
				return false;
			coin_transfer_payload payload = {};
			if (!coin_transfer_command_decode_payload(admission, &payload))
				return false;
			auto endpoint_kind = [](const coin_transfer_endpoint &endpoint)
			{
				if (endpoint.change.type == critical_command_type::account_bank)
					return economic_account_kind::wallet;
				if (endpoint.change.type == critical_command_type::item_transfer)
					return economic_account_kind::pile;
				return economic_account_kind{};
			};
			const economic_account_key source = { meta.lineage,
							      endpoint_kind(payload.source),
							      little_u64(facts, 0), 0 };
			const economic_account_key destination = {
				meta.lineage, endpoint_kind(payload.destination),
				little_u64(facts, 8), 0
			};
			if (coin_transfer_accounting_intent(admission, meta.epoch, source,
							    destination, &expected) != error::ok)
				return false;
		}
		else if (command.type == critical_command_type::item_transfer)
		{
			if (native_quest_transport_command(command))
			{
				// Immutable type support only. Rebuild the complete original intent;
				// live held ownership, birth/source and SQL authority remain mandatory.
				if (!command.publication_required ||
				    meta.writer_id != ECONOMIC_WRITER_ITEM_TRANSFER ||
				    meta.actor_kind != economic_actor_kind::domain ||
				    meta.actor_id > INT32_MAX)
					return false;
				admission.accepted_at_usec = 0;
				const auto *source = meta.source_event ? &*meta.source_event :
									 nullptr;
				return item_native_mobile_accounting_intent(
					       admission, meta.lineage, meta.epoch,
					       static_cast<uint32_t>(meta.actor_id), source,
					       &expected) == error::ok &&
				       expected == command.accounting_intent;
			}
			return item_transfer_accounting_command_supported(command);
		}
		else if (command.type == critical_command_type::collector)
		{
			collector_command_payload payload;
			collector::record listing;
			economic_account_key wallet, bank;
			return collector_purchase_accounting_decode(command, &intent, &payload,
								    &listing, &wallet,
								    &bank) == error::ok;
		}
		else
			return false;
		return expected == command.accounting_intent;
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
}

bool economic_flatfile_command_admission_supported(const critical_command &command) noexcept
{
#ifdef __NO_MYSQL__
	if (command.type == critical_command_type::shop_trade)
	{
		try
		{
			economic_frozen_intent intent;
			shop_trade_payload payload{};
			economic_account_key wallet, bank, counterparty;
			// Genuine client-free publication retains full v8 forests. Older
			// flat SHOP commands have no admitted native recovery owner.
			return command.publication_required &&
			       command.payload_version == SHOP_TRADE_RECOVERY_PAYLOAD_VERSION &&
			       shop_trade_accounting_decode(command, &intent, &payload, &wallet,
							    &bank, &counterparty) ==
				       economic_accounting_error::ok;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (command.type == critical_command_type::collector)
	{
		try
		{
			economic_frozen_intent intent;
			collector_command_payload payload{};
			collector::record listing;
			economic_account_key wallet, bank;
			return command.publication_required &&
			       command.schema_version ==
				       CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			       critical_command_envelope_valid(command) &&
			       collector_purchase_accounting_decode(command, &intent, &payload,
								    &listing, &wallet, &bank) ==
				       economic_accounting_error::ok &&
			       payload.action == collector_action::purchase &&
			       payload.item_count == 1 && payload.actor_pid &&
			       payload.actor_pid <= INT32_MAX && payload.selected_item_uid;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
#endif
	// Native acknowledged mutation has no qualified flat-file publication owner.
	if (native_quest_transport_command(command))
		return false;
	if (command.type == critical_command_type::account_bank)
	{
		currency_command_payload payload = {};
		if (!currency_command_decode_payload(command, &payload) ||
		    (payload.reason != currency_reason_type::atm_deposit &&
		     payload.reason != currency_reason_type::atm_withdraw &&
		     payload.reason != currency_reason_type::chaos_starter_reward &&
		     !(payload.reason == currency_reason_type::wallet_reward &&
		       payload.reason_id > 0 &&
		       command.source_site == critical_source_site::recovery &&
		       command.deadline_class == critical_deadline_class::recovery)))
			return false;
	}
	return (command.type == critical_command_type::account_bank ||
		command.type == critical_command_type::item_transfer) &&
	       economic_command_admission_supported(command);
}

bool economic_room_command_admission_supported_bounded(const critical_command &command,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer) noexcept
{
	// Complete projection of the original SQL ROOM branch only. Other command
	// families are deliberately outside this bounded provider's scope.
	if (!reserve || command.type != critical_command_type::zone_reset_item_birth ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
#ifdef __NO_MYSQL__
	(void)context;
	(void)outer;
	// Exact original SQL ROOM support refusal in the flat build.
	return false;
#else
	if (outer > SIZE_MAX - sizeof(zone_reset_item_image) ||
	    !reserve(outer + sizeof(zone_reset_item_image), context))
		return false;
	zone_reset_item_image original;
	// Owning full decoder retains the original publication/payload/literal,
	// recipe/source/intent binding and complete canonical command comparison.
	return zone_reset_item_command_decode_bounded(command, &original, reserve, context,
						      outer + sizeof(zone_reset_item_image)) ==
	       economic_accounting_error::ok;
#endif
}

bool economic_flatfile_room_command_admission_supported_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer) noexcept
{
	if (!reserve || command.type != critical_command_type::zone_reset_item_birth)
		return false;
	// The original flat callback's SHOP/collector/account-bank/native-quest
	// branches do not match ROOM. Its decisive final bank/item allowlist remains
	// unchanged; type22 never reaches a decoder or becomes admitted here.
	return (command.type == critical_command_type::account_bank ||
		command.type == critical_command_type::item_transfer) &&
	       economic_room_command_admission_supported_bounded(command, reserve, context, outer);
}

bool economic_ordinary_native_command_admission_supported_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	// Complete original NMB4 ordinary branch, without changing registration.
	// The original selected flat allowlist still decides whether this route is
	// available. A codec proof alone cannot grant submission or activation.
	if (!reserve || command.type != critical_command_type::native_mobile_birth ||
	    command.payload_version != NATIVE_MOBILE_BIRTH_CASH_ROLE_PAYLOAD_VERSION ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
	struct admission_workspace
	{
		quest_mobile_native_image original;
		std::vector<native_mobile_birth_item_recipe> recipes;
		native_mobile_birth_cash_role_recipe role;
	};
	// Charge the genuine simultaneous outputs before constructing them. Caller
	// owns command/context/prior outputs. The full owning decoder admits its
	// real transient allocations and retains all these outputs through return.
	constexpr size_t frame = sizeof(admission_workspace) + sizeof(const critical_command *) +
				 sizeof(reserve) + sizeof(context) + sizeof(outer_live) +
				 sizeof(size_t) + sizeof(bool) + sizeof(economic_accounting_error);
	if (frame > SIZE_MAX - outer_live || !reserve(outer_live + frame, context))
		return false;
	admission_workspace work;
	return native_mobile_birth_cash_role_command_decode_bounded(
		       command, &work.original, &work.recipes, &work.role, reserve, context,
		       outer_live + frame) == economic_accounting_error::ok &&
	       work.role.role == native_mobile_birth_cash_role::ordinary_wallet;
}

#include <algorithm>
#include <iterator>
#include <openssl/sha.h>

namespace
{
using bank_admission_getter = bool (*)(size_t *) noexcept;
bool bank_admission_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
struct bank_admission_workspace
{
	economic_frozen_intent intent{};
	currency_command_payload payload{};
	critical_command admission{};
	std::vector<uint8_t> expected;
	economic_account_key wallet{}, bank{};
	bool (*reserve)(size_t, void *) noexcept;
	void *context;
	size_t outer, owned_source, current = 0;
	size_t child_source = 0, child_supplement = 0, child_initial = 0, child_outer = 0;
	bool denied = false;
	bool observe() noexcept
	{
		current = outer;
		if (admission.keys.capacity() > SIZE_MAX / sizeof(critical_entity_key) ||
		    admission.expected_revisions.capacity() >
			    SIZE_MAX / sizeof(critical_expected_revision))
			return false;
		return bank_admission_add(current, sizeof(*this)) &&
		       bank_admission_add(current, owned_source) &&
		       bank_admission_add(current, intent.admission.facts.capacity()) &&
		       bank_admission_add(current, admission.keys.capacity() *
							   sizeof(critical_entity_key)) &&
		       bank_admission_add(current, admission.expected_revisions.capacity() *
							   sizeof(critical_expected_revision)) &&
		       bank_admission_add(current, admission.payload.capacity()) &&
		       bank_admission_add(current, admission.accounting_intent.capacity()) &&
		       bank_admission_add(current, expected.capacity());
	}
	bool admit(size_t extra) noexcept
	{
		if (!observe() || !bank_admission_add(current, extra) || !reserve(current, context))
		{
			denied = true;
			return false;
		}
		return true;
	}
	static bool forward(size_t bytes, void *opaque) noexcept
	{
		auto &work = *static_cast<bank_admission_workspace *>(opaque);
		if (!work.reserve(bytes, work.context))
		{
			work.denied = true;
			return false;
		}
		return true;
	}
	bool child(bank_admission_getter source, bank_admission_getter supplement,
		   bank_admission_getter initial, size_t query) noexcept
	{
		child_source = child_supplement = child_initial = 0;
		if (!admit(query) || !source(&child_source) ||
		    (supplement && !supplement(&child_supplement)) || !initial(&child_initial) ||
		    !bank_admission_add(child_source, child_initial) || !admit(child_source) ||
		    !observe())
		{
			denied = true;
			return false;
		}
		child_outer = current;
		if (!bank_admission_add(child_outer, child_supplement))
		{
			denied = true;
			return false;
		}
		return true;
	}
};

// Exact named parent selections supplied by finite readback of body bc98da27.
// Selected child Source contracts remain separate, conservative occurrences.
// Runtime/allocator/libc/ABI/emitted-stack qualification is outside this law.
namespace bank_admission_source
{
constexpr size_t P = sizeof(void *), N = sizeof(size_t), B = sizeof(bool);
using F = bool (*)(size_t, void *) noexcept;
using G = bank_admission_getter;
using S = std::span<const uint8_t>;
using CIK = std::vector<critical_entity_key>::const_iterator;
using CIU = std::vector<uint8_t>::const_iterator;
using Cmp = decltype(&critical_entity_key_less);
using Adapter = __gnu_cxx::__ops::_Iter_comp_val<Cmp>;
using RA = std::random_access_iterator_tag;

// Real selected public wrapper and common body lexical scopes. The two span
// objects and two brace-assignment RHS accounts are physical expressions.
// The named result is assigned from the child; both actual carriers remain.
constexpr size_t parent = 5 * P + 2 * sizeof(F) + 9 * N + 3 * B + 2 * sizeof(S) +
			  sizeof(economic_accounting_error) + 2 * sizeof(economic_account_key);
// observe, admit, forward and child. The real checked-add P+N+B is already
// retained by the own complete Source query below and is not added twice.
constexpr size_t helpers = 5 * P + 3 * N + 4 * B + 3 * sizeof(G);
constexpr size_t capacities = 3 * (P + N);
// Aggregate initialization invokes explicit byte/key/revision vector defaults
// and the real empty optional chain, not aggregate-wide constructor bodies.
constexpr size_t defaults = 23 * P;
// Three typed cleanup lanes22P5N2B, one sized-delete and one internal predicate;
// workspace/intent/facts/command and optional-base actual receivers remain.
constexpr size_t cleanup = 69 * P + 13 * N + B;
// Actual const/mutable vector Range constructors, span copy/destruction and
// little_u64's by-value formal/offset/index/value/return/subscript. Its internal
// assertion predicate is the same std::__is_constant_evaluated as cleanup.
constexpr size_t spans = 40 * P + 9 * N + sizeof(S) + 2 * sizeof(uint64_t);
constexpr size_t opaque_byte = 4 * P + 2 * N;
// Published genuine byte equality declaration row; public constant-evaluation
// function differs from the internal cleanup predicate above.
constexpr size_t equality = 9 * sizeof(CIU) + 30 * P + 4 * N + 7 * B + sizeof(int);
// Account-bank envelope-selected leaves only; no legacy none_of, native-auction
// payload, or SHOP predicate descendants are imported from older whole bounds.
constexpr size_t envelope = 22 * P + 7 * N + 7 * B + sizeof(uint8_t) + sizeof(critical_entity_type);
constexpr size_t envelope_methods = 33 * P + 5 * N + 4 * B + 2 * sizeof(CIK) + 2 * sizeof(CIU);
// Genuine normal-iterator binary_search/lower_bound, comparator adapter, raw
// distance/advance and positive increment branch. No implicit trivial iterator
// bodies, negative decrement branch, or builtin callee frames are invented.
constexpr size_t binary = 11 * sizeof(CIK) + 30 * P + 3 * sizeof(Cmp) + sizeof(Adapter) +
			  9 * sizeof(std::ptrdiff_t) + 2 * sizeof(RA) + 3 * B;
// Actual full profile query executes once before workspace construction; the
// separate INITIAL getter owns output/result. The accessor executes only in
// mandatory constant evaluation; no runtime accessor-return carrier is added.
constexpr size_t queries = economic_bank_command_admission_source_query_frame_bytes() + P + B;
constexpr size_t owned = parent + helpers + capacities + defaults + cleanup + spans + opaque_byte +
			 equality + envelope + envelope_methods + binary + queries;
} // namespace bank_admission_source
constexpr size_t bank_admission_owned_frames = bank_admission_source::owned;

bool bank_admission_supported(const critical_command &command,
			      bool (*reserve)(size_t, void *) noexcept, void *context,
			      size_t outer_live, bool flat) noexcept
{
	using error = economic_accounting_error;
	if (!reserve)
		return false;
	size_t initial = outer_live, source = 0, inline_bytes = 0;
	if (!bank_admission_add(initial, bank_admission_owned_frames) ||
	    !reserve(initial, context) ||
	    !economic_bank_command_admission_source_frame_bytes(&source) ||
	    !economic_bank_command_admission_initial_inline_bytes(&inline_bytes))
		return false;
	initial = outer_live;
	if (!bank_admission_add(initial, source) || !bank_admission_add(initial, inline_bytes) ||
	    !reserve(initial, context))
		return false;
	if (command.type != critical_command_type::account_bank ||
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !critical_command_envelope_valid(command))
		return false;
	try
	{
		bank_admission_workspace work{
			{}, {},	     {},      {},	  {},
			{}, reserve, context, outer_live, bank_admission_owned_frames
		};
		constexpr size_t payload_query =
			3 * currency_command_decode_payload_source_query_frame_bytes();
		if (!work.child(currency_command_decode_payload_source_frame_bytes,
				currency_command_decode_payload_source_supplement_frame_bytes,
				currency_command_decode_payload_initial_inline_bytes,
				payload_query) ||
		    currency_command_decode_payload_bounded_status(
			    command, &work.payload, bank_admission_workspace::forward, &work,
			    work.child_outer) != currency_command_bounded_result::ok)
			return false;
		// The original flat callback adds this exact allowlist before the common
		// immutable bank branch. SQL preserves the original intent route choices.
		if (flat && work.payload.reason != currency_reason_type::atm_deposit &&
		    work.payload.reason != currency_reason_type::atm_withdraw &&
		    work.payload.reason != currency_reason_type::chaos_starter_reward &&
		    !(work.payload.reason == currency_reason_type::wallet_reward &&
		      work.payload.reason_id > 0 &&
		      command.source_site == critical_source_site::recovery &&
		      command.deadline_class == critical_deadline_class::recovery))
			return false;
		constexpr size_t decode_query =
			3 * economic_intent_decode_source_query_frame_bytes();
		if (!work.child(economic_intent_decode_source_frame_bytes,
				economic_intent_decode_source_supplement_frame_bytes,
				economic_intent_decode_initial_inline_bytes, decode_query))
			return false;
		const std::span<const uint8_t> encoded(command.accounting_intent);
		if (economic_intent_decode_bounded(encoded, &work.intent,
						   bank_admission_workspace::forward, &work,
						   work.child_outer) != error::ok ||
		    work.intent.admission.facts.size() != ECONOMIC_BANK_FACT_BYTES)
			return false;
		const auto &meta = work.intent.admission.metadata;
		const std::span<const uint8_t> facts(work.intent.admission.facts);
		work.wallet = { meta.lineage, economic_account_kind::wallet, little_u64(facts, 0),
				0 };
		work.bank = { meta.lineage, economic_account_kind::bank, little_u64(facts, 8),
			      little_u64(facts, 16) };
		// The full payload decoder authenticated the exact ordered two keys and
		// revision keys and the 136-byte field layout, including both zero tails.
		// Reconstruction therefore equals the original copy/clear schema-1
		// projection in every command value used by the original intent compiler.
		constexpr size_t build_query =
			3 * currency_command_build_source_query_frame_bytes();
		if (!work.child(currency_command_build_source_frame_bytes,
				currency_command_build_source_supplement_frame_bytes,
				currency_command_build_initial_inline_bytes, build_query) ||
		    currency_command_build_bounded_status(
			    &work.admission, command.operation_id, work.payload,
			    command.expected_revisions[0].revision,
			    command.expected_revisions[1].revision, command.source_site,
			    command.deadline_class, bank_admission_workspace::forward, &work,
			    work.child_outer) != currency_command_bounded_result::ok)
			return false;
		// Currency v1 byte5 is accepted opaque padding by the original decoder.
		// The fresh encoder zeroes it; retain its original value for the exact
		// original command binding rather than tightening the accepted wire.
		work.admission.payload[5] = command.payload[5];
		work.admission.accepted_at_usec = command.accepted_at_usec;
		constexpr size_t intent_query =
			2 * economic_currency_intent_source_query_frame_bytes();
		if (!work.child(economic_currency_intent_source_frame_bytes, nullptr,
				economic_currency_intent_initial_inline_bytes, intent_query))
			return false;
		error result;
		if (work.payload.reason == currency_reason_type::chaos_starter_reward)
			result = economic_chaos_starter_bank_intent_bounded(
				work.admission, meta.epoch, work.wallet, work.bank, &work.expected,
				bank_admission_workspace::forward, &work, work.child_outer);
		else if (work.payload.reason == currency_reason_type::wallet_reward)
			result = economic_quest_wallet_reward_intent_bounded(
				work.admission, meta.epoch, work.wallet, work.bank, &work.expected,
				bank_admission_workspace::forward, &work, work.child_outer);
		else
			result = economic_bank_transfer_intent_bounded(
				work.admission, meta.epoch, work.wallet, work.bank, &work.expected,
				bank_admission_workspace::forward, &work, work.child_outer);
		return result == error::ok && !work.denied &&
		       work.expected == command.accounting_intent;
	}
	catch (...)
	{
		return false;
	}
}
} // namespace

bool economic_bank_command_admission_supported_bounded(const critical_command &command,
						       bool (*reserve)(size_t, void *) noexcept,
						       void *context, size_t outer_live) noexcept
{
	return bank_admission_supported(command, reserve, context, outer_live, false);
}

bool economic_flatfile_bank_command_admission_supported_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
	return bank_admission_supported(command, reserve, context, outer_live, true);
}

bool economic_bank_command_admission_source_frame_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                            \
	!defined(_GLIBCXX_CONCEPT_CHECKS) && !defined(__SANITIZE_ADDRESS__) &&                     \
	!defined(__SANITIZE_THREAD__) &&                                                           \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
	    sizeof(SHA_LONG) != 4 || sizeof(std::allocator<uint8_t>) != 1 ||
	    sizeof(std::vector<uint8_t>::iterator) != sizeof(void *))
		return false;
	size_t decode = 0, payload = 0, build = 0, intent = 0, total = bank_admission_owned_frames;
	if (!economic_intent_decode_source_frame_bytes(&decode) ||
	    !currency_command_decode_payload_source_frame_bytes(&payload) ||
	    !currency_command_build_source_frame_bytes(&build) ||
	    !economic_currency_intent_source_frame_bytes(&intent) ||
	    !bank_admission_add(total, decode) || !bank_admission_add(total, payload) ||
	    !bank_admission_add(total, build) || !bank_admission_add(total, intent))
		return false;
	*output = total;
	return true;
#else
	(void)output;
	return false;
#endif
}

bool economic_bank_command_admission_source_supplement_frame_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                            \
	!defined(_GLIBCXX_CONCEPT_CHECKS) && !defined(__SANITIZE_ADDRESS__) &&                     \
	!defined(__SANITIZE_THREAD__) &&                                                           \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
	    sizeof(SHA_LONG) != 4 || sizeof(std::allocator<uint8_t>) != 1 ||
	    sizeof(std::vector<uint8_t>::iterator) != sizeof(void *))
		return false;
	*output = 0;
	return true;
#else
	(void)output;
	return false;
#endif
}

bool economic_bank_command_admission_initial_inline_bytes(size_t *output) noexcept
{
#if defined(__linux__) && defined(__x86_64__) && defined(__LP64__) && !defined(_WIN32) &&          \
	defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) &&  \
	_GLIBCXX_USE_CXX11_ABI == 1 && __cplusplus == 202002L && !defined(_GLIBCXX_DEBUG) &&       \
	!defined(_GLIBCXX_ASSERTIONS) && !defined(_GLIBCXX_PARALLEL) &&                            \
	!defined(_GLIBCXX_CONCEPT_CHECKS) && !defined(__SANITIZE_ADDRESS__) &&                     \
	!defined(__SANITIZE_THREAD__) &&                                                           \
	(!defined(_GLIBCXX_SANITIZE_VECTOR) || _GLIBCXX_SANITIZE_VECTOR == 0) &&                   \
	OPENSSL_VERSION_MAJOR == 3 && OPENSSL_VERSION_MINOR == 0 && OPENSSL_VERSION_PATCH == 13 && \
	!defined(OPENSSL_NO_DEPRECATED_3_0)
	if (!output || sizeof(void *) != 8 || sizeof(size_t) != 8 || sizeof(std::ptrdiff_t) != 8 ||
	    sizeof(SHA_LONG) != 4 || sizeof(std::allocator<uint8_t>) != 1 ||
	    sizeof(std::vector<uint8_t>::iterator) != sizeof(void *))
		return false;
	*output = sizeof(bank_admission_workspace);
	return true;
#else
	(void)output;
	return false;
#endif
}
