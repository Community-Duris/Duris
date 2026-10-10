#include "economy/currency_transaction.h"
#include "economy/currency_publication.h"
#include "economy/economic_gameplay_authority.h"
#include "economy/account_bank_balances.h"
#include "player/player_save_pipeline.h"

#include "net/gmcp.h"
#include "core/prototypes.h"
#include "core/utils.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <chrono>
#include <climits>
#include <cstring>
#include <new>
#include <optional>
#include <string>
#include <unordered_map>
#include <utility>
#include <type_traits>

extern P_desc descriptor_list;
extern P_char character_list;

namespace
{
P_char live_wallet_by_pid(uint32_t pid)
{
	if (P_char direct = find_player_by_pid(static_cast<int>(pid)))
		return direct;
	for (P_desc desc = descriptor_list; desc; desc = desc->next)
	{
		if (STATE(desc) != CON_PLAYING || !desc->character)
			continue;
		P_char wallet = GET_PLYR(desc->character);
		if (wallet && IS_PC(wallet) && GET_PID(wallet) == static_cast<int>(pid))
			return wallet;
	}
	return nullptr;
}

P_char coin_wallet_by_pid(uint32_t pid)
{
	if (P_char character = live_wallet_by_pid(pid))
		return character;
	// Linkdead characters are retained and reused on reconnect. Their wallet
	// must receive the receipt even after their descriptor has disappeared.
	for (P_char character = character_list; character; character = character->next)
	{
		P_char wallet = GET_PLYR(character);
		if (wallet && IS_PC(wallet) && GET_PID(wallet) == static_cast<int>(pid))
			return wallet;
	}
	return nullptr;
}

struct pending_currency
{
	uint32_t pid;
	std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name;
	uint8_t racewar;
	currency_completion_fn completion;
	std::array<uint8_t, CURRENCY_PENDING_CONTEXT_MAX_BYTES> context;
	size_t context_size;
	currency_publication_state publication_state =
		currency_publication_state::awaiting_completion;
	critical_completion completed;
	std::optional<coin_transfer_payload> coin = std::nullopt;
	coin_completion_fn coin_completion = nullptr;
	coin_publication_callbacks coin_publication = {};
	bool coin_wallets_published = false;
	std::array<uint64_t, 2> coin_wallet_body_ids = {};
	bool coin_publication_started = false;
	bool coin_physical_published = false;
	bool coin_publication_in_progress = false;
	unsigned int publication_attempts = 0;
	bool publication_required = false;
	bool receipt_received = false;
	bool disposition_blocked = false;
	bool disposition_blocked_this_batch = false;
	// Replay owns the entire immutable parent, including typed intent, admission
	// identity, endpoint commands and publication bit; decoded endpoints alone
	// cannot supply the retained/native proof required for cold publication.
	std::optional<critical_command> restored_coin_command = std::nullopt;
	bool restored_room_coin = false;
	bool restored_coin_receipt_sealed = false;
};

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
// New data-free owner of the original unordered_map's actual table. No existing
// map is cast or layout-read. All selected ordinary algorithms remain inherited.
class currency_native_pending_table final
	: public std::__umap_hashtable<std::string, pending_currency>
{
	using table_type = std::__umap_hashtable<std::string, pending_currency>;
	using original_type = std::unordered_map<std::string, pending_currency>;
	using actual_node = std::__detail::_Hash_node<
		typename table_type::value_type,
		std::__cache_default<std::string, std::hash<std::string>>::value>;

    public:
	using table_type::table_type;
	using table_type::operator=;
	using table_type::insert;
	// Exact original unordered_map node-handle forwarding used by selected
	// coin-publication refusal recovery; retain inherited value insert overloads.
	insert_return_type insert(node_type &&node)
	{
		return this->_M_reinsert_node(std::move(node));
	}
	bool current_table_heap_bytes(size_t *output) const noexcept
	{
		if (!output)
			return false;
		size_t bytes = 0;
		const size_t buckets = this->bucket_count();
		if (!buckets ||
		    (buckets > 1 && buckets > SIZE_MAX / sizeof(std::__detail::_Hash_node_base *)))
			return false;
		if (buckets > 1)
			bytes = buckets * sizeof(std::__detail::_Hash_node_base *);
		if (this->size() > (SIZE_MAX - bytes) / sizeof(actual_node))
			return false;
		bytes += this->size() * sizeof(actual_node);
		for (const auto &entry : *this)
			if (entry.first.capacity() > 15)
			{
				if (entry.first.capacity() == SIZE_MAX ||
				    entry.first.capacity() + 1 > SIZE_MAX - bytes)
					return false;
				bytes += entry.first.capacity() + 1;
			}
		*output = bytes;
		return true;
	}
	bool next_bank_insert_extra_peak(const std::string &key, size_t *output) const noexcept
	{
		if (!output || this->size() == this->max_size() || key.size() == SIZE_MAX)
			return false;
		const size_t text = key.size() > 15 ? key.size() + 1 : 0;
		if (text > SIZE_MAX - sizeof(actual_node))
			return false;
		size_t extra = sizeof(actual_node) + text;
		// These objects are admitted by the owning caller before this pure profile
		// call. Copy the actual CURRENT original policy, never guess a threshold.
		auto policy = this->__rehash_policy();
		const auto next = policy._M_need_rehash(this->bucket_count(), this->size(), 1);
		if (next.first && next.second > 1)
		{
			if (next.second >
			    (SIZE_MAX - extra) / sizeof(std::__detail::_Hash_node_base *))
				return false;
			extra += next.second * sizeof(std::__detail::_Hash_node_base *);
		}
		*output = extra;
		return true;
	}
};
static_assert(sizeof(currency_native_pending_table) ==
	      sizeof(std::unordered_map<std::string, pending_currency>));
static_assert(alignof(currency_native_pending_table) ==
	      alignof(std::unordered_map<std::string, pending_currency>));
static_assert(std::is_same_v<currency_native_pending_table::iterator,
			     std::unordered_map<std::string, pending_currency>::iterator>);
static_assert(std::is_same_v<currency_native_pending_table::allocator_type,
			     std::unordered_map<std::string, pending_currency>::allocator_type>);
using currency_pending_table = currency_native_pending_table;
#else
using currency_pending_table = std::unordered_map<std::string, pending_currency>;
#endif
currency_pending_table pending;
currency_transaction_health health = {};

std::string operation_key(const critical_operation_id &operation_id)
{
	return std::string(reinterpret_cast<const char *>(operation_id.bytes.data()),
			   operation_id.bytes.size());
}

bool same_publication_receipt(const critical_completion &left, const critical_completion &right)
{
	// Queue timestamps do not affect publication. Every field that can change the
	// authoritative outcome or decoded result must match before a replay sleeps.
	return left.disposition == right.disposition && left.outcome == right.outcome &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.attempt == right.attempt && left.result_size == right.result_size &&
	       left.result_payload == right.result_payload;
}

bool retain_unresolved_publication(pending_currency &entry, const char *reason, bool malformed)
{
	if (!currency_publication_state_is_blocked(entry.publication_state))
	{
		entry.publication_state = currency_publication_state::blocked_receipt;
		++health.publication_blocked;
		if (malformed)
			++health.malformed_completions;
		char operation[33];
		critical_operation_id_to_hex(entry.completed.operation_id, operation,
					     sizeof(operation));
		const unsigned int reason_code = !strcmp(reason, "unresolved_outcome")	       ? 1 :
						 !strcmp(reason, "invalid_result")	       ? 2 :
						 !strcmp(reason, "invalid_live_balances")      ? 3 :
						 !strcmp(reason, "invalid_coin_result")	       ? 4 :
						 !strcmp(reason, "invalid_coin_endpoint")      ? 5 :
						 !strcmp(reason, "invalid_coin_live_balances") ? 6 :
												 0;
		persistence_alert(AVATAR, "currency", "publication", operation, "none",
				  "publication_blocked", "outcome=%u error=%u reason_code=%u",
				  static_cast<unsigned int>(entry.completed.outcome),
				  entry.completed.error_code, reason_code);
	}
	return false;
}

bool publish_bank(P_char character, const char *account_name, uint8_t racewar,
		  const currency_vector &bank, uint64_t bank_revision)
{
	for (int64_t amount : bank.amount)
		if (amount < 0 || amount > INT_MAX)
			return false;
	const AccountBankBalances balances = { static_cast<int>(bank.amount[0]),
					       static_cast<int>(bank.amount[1]),
					       static_cast<int>(bank.amount[2]),
					       static_cast<int>(bank.amount[3]) };
	// The endpoint is identified by the immutable command, so it can be updated
	// without a playing descriptor or an account name on the retained body.
	if (character && bank_revision >= character->only.pc->bank_revision)
	{
		GET_BALANCE_COPPER(character) = balances.copper;
		GET_BALANCE_SILVER(character) = balances.silver;
		GET_BALANCE_GOLD(character) = balances.gold;
		GET_BALANCE_PLATINUM(character) = balances.platinum;
		character->only.pc->bank_revision = bank_revision;
		gmcp_char_vitals(character);
	}
	publish_account_bank_balances_revision(account_name, racewar, &balances, bank_revision);
	return true;
}

bool coin_has_physical_endpoint(const coin_transfer_payload &payload)
{
	return payload.source.change.type == critical_command_type::item_transfer ||
	       payload.destination.change.type == critical_command_type::item_transfer;
}

bool restored_room_coin_shape(const coin_transfer_payload &payload,
			      const item_transfer_payload &pile)
{
	const bool drop = payload.source.change.type == critical_command_type::account_bank &&
			  payload.destination.change.type == critical_command_type::item_transfer;
	const bool pickup = payload.source.change.type == critical_command_type::item_transfer &&
			    payload.destination.change.type == critical_command_type::account_bank;
	if (!drop && !pickup)
		return false;
	if (pile.multi_root || pile.item_count != 1 || !pile.selected_item_uid ||
	    pile.items[0].item_uid != pile.selected_item_uid ||
	    pile.items[0].root_item_uid != pile.selected_item_uid ||
	    pile.items[0].parent_item_uid || pile.target_parent_item_uid ||
	    pile.target_root_item_uid != pile.selected_item_uid ||
	    pile.expected_target_parent_revision || pile.corpse.present || pile.collector.present ||
	    pile.continuation.kind != item_transfer_continuation_kind::none ||
	    !pile.continuation.data.empty())
		return false;
	const auto room = [](const item_owner_identity &owner)
	{
		return owner.type == item_owner_type::room && owner.id && owner.id <= INT32_MAX &&
		       !owner.context_id;
	};
	if (drop)
		return pile.reason == item_transfer_reason::creation &&
		       pile.from_owner.type == item_owner_type::system && !pile.from_owner.id &&
		       !pile.from_owner.context_id && room(pile.to_owner);
	return room(pile.from_owner) &&
	       ((pile.reason == item_transfer_reason::player_put &&
		 item_owner_identity_equal(pile.from_owner, pile.to_owner)) ||
		(pile.reason == item_transfer_reason::destruction &&
		 pile.to_owner.type == item_owner_type::destruction && !pile.to_owner.id &&
		 !pile.to_owner.context_id));
}

bool same_restored_coin_command(const critical_command &left, const critical_command &right)
{
	// critical_command_equal serializes and allocates. The synchronous receipt
	// fence must compare every original field without allocating or normalizing.
	return left.schema_version == right.schema_version &&
	       critical_operation_id_equal(left.operation_id, right.operation_id) &&
	       left.type == right.type && left.payload_version == right.payload_version &&
	       left.source_site == right.source_site &&
	       left.deadline_class == right.deadline_class &&
	       left.accepted_at_usec == right.accepted_at_usec &&
	       left.publication_required == right.publication_required &&
	       left.payload == right.payload && left.accounting_intent == right.accounting_intent &&
	       left.keys.size() == right.keys.size() &&
	       std::equal(left.keys.begin(), left.keys.end(), right.keys.begin(),
			  critical_entity_key_equal) &&
	       left.expected_revisions.size() == right.expected_revisions.size() &&
	       std::equal(left.expected_revisions.begin(), left.expected_revisions.end(),
			  right.expected_revisions.begin(),
			  [](const critical_expected_revision &a,
			     const critical_expected_revision &b) {
				  return a.revision == b.revision &&
					 critical_entity_key_equal(a.key, b.key);
			  });
}

bool same_restored_coin_receipt(const critical_completion &left, const critical_completion &right)
{
	const auto committed = [](critical_apply_outcome outcome)
	{
		return outcome == critical_apply_outcome::applied ||
		       outcome == critical_apply_outcome::already_applied;
	};
	return critical_operation_id_equal(left.operation_id, right.operation_id) &&
	       left.disposition == right.disposition &&
	       (committed(left.outcome) ? committed(right.outcome) :
					  left.outcome == right.outcome) &&
	       left.durable_revision == right.durable_revision &&
	       left.error_code == right.error_code && left.failure_stage == right.failure_stage &&
	       left.result_size == right.result_size && left.result_payload == right.result_payload;
}

bool coin_physical_result_matches(const coin_transfer_payload &payload,
				  const coin_transfer_result &result);

bool restored_coin_definitive_receipt(const pending_currency &entry,
				      const critical_completion &completion)
{
	if (!entry.restored_coin_command || !entry.coin ||
	    !critical_operation_id_equal(entry.restored_coin_command->operation_id,
					 completion.operation_id) ||
	    completion.disposition != critical_completion_disposition::execution ||
	    !critical_completion_disposition_valid(completion) ||
	    !critical_failure_stage_valid(completion.failure_stage) ||
	    completion.result_size > completion.result_payload.size() ||
	    std::any_of(completion.result_payload.begin() + completion.result_size,
			completion.result_payload.end(), [](uint8_t byte) { return byte != 0; }))
		return false;
	if (completion.outcome == critical_apply_outcome::applied ||
	    completion.outcome == critical_apply_outcome::already_applied)
	{
		coin_transfer_result result = {};
		std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> canonical = {};
		if (completion.error_code ||
		    completion.failure_stage != critical_failure_stage::none ||
		    !coin_transfer_command_decode_result(*entry.coin,
							 completion.result_payload.data(),
							 completion.result_size, &result) ||
		    !coin_physical_result_matches(*entry.coin, result) ||
		    !coin_transfer_command_encode_result(*entry.coin, result, &canonical) ||
		    !std::equal(canonical.begin(), canonical.end(),
				completion.result_payload.begin()))
			return false;
		uint64_t revision = 0;
		const coin_transfer_endpoint *endpoints[] = { &entry.coin->source,
							      &entry.coin->destination };
		for (size_t index = 0; index < 2; ++index)
			if (endpoints[index]->change.type == critical_command_type::account_bank)
				revision =
					std::max({ revision, result.wallets[index].wallet_revision,
						   result.wallets[index].bank_revision });
			else
				revision =
					std::max({ revision, result.piles[index].max_item_revision
#ifndef __NO_MYSQL__
						   ,
						   result.piles[index].from_owner_revision,
						   result.piles[index].to_owner_revision
#endif
					});
		// Native retained/current proof remains mandatory in the primary owner.
		return completion.durable_revision == revision;
	}
	if (completion.outcome != critical_apply_outcome::terminal_failure ||
	    !completion.error_code)
		return false;
	const bool stale_expected =
		completion.error_code == ESTALE &&
		coin_transfer_command_stale_result_expected(*entry.coin, completion.failure_stage);
	if (!stale_expected)
#ifdef __NO_MYSQL__
		// Flat retained rejection revisions cover full opening native vectors.
		// Only exact verify_retained_locked comparison grants publication authority.
		return !completion.result_size;
#else
		return !completion.result_size && !completion.durable_revision;
#endif
	coin_transfer_stale_result stale = {};
	coin_transfer_result result = {};
	std::array<uint8_t, COIN_TRANSFER_STALE_RESULT_BYTES> canonical = {};
	if (!coin_transfer_command_decode_stale_result(*entry.coin, completion.failure_stage,
						       completion.result_payload.data(),
						       completion.result_size, &stale))
		return false;
	result.wallets[stale.endpoint_index] = stale.current;
	const uint64_t revision = std::max(stale.wallet_stale ? stale.current.wallet_revision : 0,
					   stale.bank_stale ? stale.current.bank_revision : 0);
	return
#ifndef __NO_MYSQL__
		completion.durable_revision == revision &&
#else
		// Compact bytes omit the unflagged opening vector; the original flat
		// receipt retains its full opening revision. Prove equality under lock.
		completion.durable_revision >= revision &&
#endif
		coin_transfer_command_encode_stale_result(*entry.coin, result,
							  completion.failure_stage, &canonical) &&
		std::equal(canonical.begin(), canonical.end(), completion.result_payload.begin());
}

// Decode checks the result's wire shape. Bind every physical transfer result
// to its original command before any live wallet or bank projection can run.
// The unchanged bank vector is supplied by the authoritative immutable receipt:
// this command stores its revision and zero delta, not its opening balances.
bool coin_physical_result_matches(const coin_transfer_payload &payload,
				  const coin_transfer_result &result)
try
{
	const coin_transfer_endpoint *endpoints[] = { &payload.source, &payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		if (endpoint.change.type == critical_command_type::account_bank)
		{
			currency_command_payload wallet = {};
			const auto &balances = result.wallets[index];
			if (!currency_command_decode_payload(endpoint.change, &wallet) ||
			    wallet.reason != currency_reason_type::coin_transfer ||
			    endpoint.change.expected_revisions[0].revision == UINT64_MAX ||
			    endpoint.change.expected_revisions[1].revision == UINT64_MAX ||
			    balances.wallet_revision !=
				    endpoint.change.expected_revisions[0].revision + 1 ||
			    balances.bank_revision !=
				    endpoint.change.expected_revisions[1].revision + 1)
				return false;
			for (size_t denomination = 0; denomination < endpoint.after.size();
			     ++denomination)
				if (endpoint.before[denomination] < 0 ||
				    endpoint.after[denomination] < 0 ||
				    wallet.bank_delta.amount[denomination] ||
				    wallet.wallet_delta.amount[denomination] !=
					    static_cast<int64_t>(endpoint.after[denomination]) -
						    endpoint.before[denomination] ||
				    balances.wallet.amount[denomination] !=
					    endpoint.after[denomination] ||
				    balances.bank.amount[denomination] < 0 ||
				    balances.bank.amount[denomination] > INT_MAX)
					return false;
			continue;
		}
		if (endpoint.change.type != critical_command_type::item_transfer)
			return false;
		// A second pile sharing owner rows must use the source's committed
		// owner revision, exactly as the atomic executor does.
		critical_command change;
		const critical_command *original = &endpoint.change;
		if (index && payload.source.change.type == critical_command_type::item_transfer)
		{
			if (!coin_transfer_command_destination_after_source(payload, result,
									    &change))
				return false;
			original = &change;
		}
		item_transfer_payload pile = {};
		if (!item_transfer_command_decode_payload(*original, &pile) ||
		    pile.item_count != 1 || pile.multi_root ||
		    pile.items[0].item_uid != pile.selected_item_uid ||
		    pile.expected_from_revision == UINT64_MAX ||
		    pile.expected_to_revision == UINT64_MAX)
			return false;
		const bool creation = pile.from_owner.type == item_owner_type::system;
		const uint64_t item_revision = creation ? 1 :
							  pile.items[0].expected_item_revision + 1;
		const auto &committed = result.piles[index];
		if (!item_revision || committed.root_item_uid != item_transfer_result_root(pile) ||
		    !committed.root_item_uid || committed.item_count != 1 ||
		    committed.max_item_revision != item_revision || committed.corpse_revision ||
		    committed.collector_catalog_changed ||
		    committed.from_owner_revision != pile.expected_from_revision + 1 ||
		    committed.to_owner_revision !=
			    (item_owner_identity_equal(pile.from_owner, pile.to_owner) ?
				     committed.from_owner_revision :
				     pile.expected_to_revision + 1))
			return false;
	}
	return true;
}
catch (const std::bad_alloc &)
{
	return false;
}

bool coin_body_identity_matches(P_char character, const currency_command_payload &wallet)
{
	return character && IS_PC(character) && character->only.pc &&
	       static_cast<uint32_t>(GET_PID(character)) == wallet.pid &&
	       !std::strcmp(get_account_name_safe(character), wallet.account_name.data()) &&
	       character->player.racewar == wallet.racewar;
}

bool coin_body_matches(P_char character, const currency_command_result &result)
{
	return character && character->only.pc &&
	       character->only.pc->wallet_revision == result.wallet_revision &&
	       character->only.pc->bank_revision == result.bank_revision &&
	       std::array<int64_t, 4>{ GET_COPPER(character), GET_SILVER(character),
				       GET_GOLD(character),
				       GET_PLATINUM(character) } == result.wallet.amount &&
	       std::array<int64_t, 4>{ GET_BALANCE_COPPER(character), GET_BALANCE_SILVER(character),
				       GET_BALANCE_GOLD(character),
				       GET_BALANCE_PLATINUM(character) } == result.bank.amount;
}

struct coin_publication_guard
{
	bool *flag;
	~coin_publication_guard() { release(); }
	void release()
	{
		if (flag)
			*flag = false;
		flag = nullptr;
	}
};

bool publish_accounted_coin(std::unordered_map<std::string, pending_currency>::iterator found,
			    P_char actor, bool committed, const coin_transfer_result &result)
{
	auto &entry = found->second;
	const std::string &original_key = found->first;
	if (entry.disposition_blocked ||
	    currency_publication_state_is_blocked(entry.publication_state))
		return false;
	if (entry.coin_publication_in_progress)
		return false;
	entry.coin_publication_in_progress = true;
	coin_publication_guard guard{ &entry.coin_publication_in_progress };
	const bool physical = coin_has_physical_endpoint(*entry.coin);
	if (committed && physical)
	{
		actor = live_wallet_by_pid(entry.pid);
		const size_t wallet_index =
			entry.coin->source.change.type == critical_command_type::account_bank ? 0 :
												1;
		currency_command_payload wallet = {};
		if (!currency_command_decode_payload(wallet_index ? entry.coin->destination.change :
								    entry.coin->source.change,
						     &wallet))
			return retain_unresolved_publication(entry, "invalid_coin_endpoint", true);
		if (actor && !coin_body_identity_matches(actor, wallet))
			return retain_unresolved_publication(entry, "coin_body_authority_conflict",
							     false);
		// Journal recovery has no live callback/context. Its immutable command
		// does not prove a native pile exists; retain it until typed publication
		// recovery is implemented. A legacy composite callback is not proof.
		if (!entry.coin_publication.publish)
			return retain_unresolved_publication(entry, "missing_coin_publication",
							     false);
		bool published = false;
		try
		{
			published = entry.coin_publication.publish(
				actor, entry.completed.operation_id, *entry.coin, result,
				entry.context.data(), entry.context_size);
		}
		catch (...)
		{
			// No exception can discharge a committed economic obligation.
		}
		if (entry.disposition_blocked ||
		    currency_publication_state_is_blocked(entry.publication_state))
			return false;
		P_char current = live_wallet_by_pid(entry.pid);
		if (current && !coin_body_identity_matches(current, wallet))
			return retain_unresolved_publication(entry, "coin_body_authority_conflict",
							     false);
		if (!published)
		{
			// Each dispatch bounds work to one attempt. A lifetime retry cap would
			// permanently strand a committed pile after its dependency recovers.
			entry.publication_state = currency_publication_state::retrying_callback;
			return false;
		}
		entry.coin_physical_published = true;
		if (!current || current->runtime_id != entry.coin_wallet_body_ids[wallet_index] ||
		    !coin_body_matches(current, result.wallets[wallet_index]))
		{
			entry.publication_state = currency_publication_state::retrying_callback;
			return false;
		}
		actor = current;
	}
	if (entry.completed.disposition != critical_completion_disposition::never_admitted &&
	    !critical_command_coordinator_acknowledge_publication(entry.completed.operation_id))
	{
		entry.publication_state = currency_publication_state::retrying_callback;
		return false;
	}
	// Only notifications may advance a bulk command. The durable ACK and owner
	// release precede them; failures cannot refund, reinsert or redo physical work.
	guard.release();
	// References survive rehash when a publisher admits another PID's command.
	// The original iterator does not; extract using the pinned original key.
	auto node = pending.extract(original_key);
	const auto &finished = node.mapped();
	if (finished.coin_publication.release)
		finished.coin_publication.release(finished.completed.operation_id);
	if (committed)
		++health.committed;
	else
		++health.rejected;
	const coin_completion_fn notify = physical ? finished.coin_publication.notify :
						     finished.coin_completion;
	if (notify)
	{
		bool notified = false;
		try
		{
			notified = notify(actor, committed, *finished.coin, result,
					  finished.completed.error_code, finished.context.data(),
					  finished.context_size);
		}
		catch (...)
		{
		}
		if (!notified)
		{
			char operation[33];
			critical_operation_id_to_hex(finished.completed.operation_id, operation,
						     sizeof(operation));
			persistence_alert(AVATAR, "currency", "coin_notification", operation,
					  "none", "notification_failed", "pid=%u committed=%d",
					  finished.pid, committed);
		}
	}
	return true;
}

bool publish_coin(std::unordered_map<std::string, pending_currency>::iterator found, P_char actor)
{
	auto &entry = found->second;
	const std::string &original_key = found->first;
	if (entry.publication_required && entry.coin_publication_in_progress)
		return false;
	coin_publication_guard guard{ entry.publication_required ?
					      &entry.coin_publication_in_progress :
					      nullptr };
	if (guard.flag)
		*guard.flag = true;
	const auto &completed = entry.completed;
	const bool committed = completed.outcome == critical_apply_outcome::applied ||
			       completed.outcome == critical_apply_outcome::already_applied;
	coin_transfer_result result = {};
	if (committed &&
	    !coin_transfer_command_decode_result(*entry.coin, completed.result_payload.data(),
						 completed.result_size, &result))
	{
		return retain_unresolved_publication(entry, "invalid_coin_result", true);
	}
	if (committed && entry.publication_required && coin_has_physical_endpoint(*entry.coin) &&
	    !coin_physical_result_matches(*entry.coin, result))
		return retain_unresolved_publication(entry, "invalid_coin_result", true);
	coin_transfer_stale_result stale = {};
	const bool stale_receipt_expected =
		!committed && completed.error_code == ESTALE &&
		coin_transfer_command_stale_result_expected(*entry.coin, completed.failure_stage);
	const bool stale_authority =
		stale_receipt_expected && completed.result_size &&
		coin_transfer_command_decode_stale_result(*entry.coin, completed.failure_stage,
							  completed.result_payload.data(),
							  completed.result_size, &stale);
	if (stale_receipt_expected && !stale_authority)
		return retain_unresolved_publication(entry, "invalid_coin_stale_result", true);
	if (!committed && completed.result_size && !stale_authority)
		return retain_unresolved_publication(entry, "unexpected_coin_terminal_result",
						     true);
	if (entry.restored_coin_command && coin_has_physical_endpoint(*entry.coin))
	{
		// Cold replay must not project stale command balances before native
		// current authority and the original save reservation are established.
		if (!entry.restored_room_coin)
			return retain_unresolved_publication(entry, "unsupported_restored_coin",
							     false);
		if (!entry.restored_coin_receipt_sealed || entry.disposition_blocked)
			return retain_unresolved_publication(
				entry, "unsealed_restored_coin_receipt", false);
		entry.coin_publication_started = true;
		const critical_completion sealed = entry.completed;
		bool acknowledged = false;
		try
		{
			acknowledged = coin_physical_publication_restore_and_acknowledge(
				*entry.restored_coin_command, sealed);
		}
		catch (...)
		{
			// Native/session/hold cleanup belongs to the primary owner. No
			// throwing attempt can authorize retirement or an ID-only ACK here.
		}
		if (!currency_transaction_restored_coin_receipt_current(
			    *entry.restored_coin_command, sealed))
			return false;
		if (!acknowledged)
		{
			entry.publication_state = currency_publication_state::retrying_callback;
			return false;
		}
		// The primary owner already completed guarded ACK and consumed its
		// exact save hold. Never repeat a separate ID-only coordinator ACK.
		guard.release();
		auto node = pending.extract(original_key);
		if (committed)
			++health.committed;
		else
			++health.rejected;
		return true;
	}
	try
	{
		if (stale_authority && !entry.coin_wallets_published)
		{
			const coin_transfer_endpoint &endpoint =
				stale.endpoint_index ? entry.coin->destination : entry.coin->source;
			currency_command_payload wallet;
			if (!currency_command_decode_payload(endpoint.change, &wallet))
				return retain_unresolved_publication(
					entry, "invalid_stale_coin_endpoint", true);
			P_char character = coin_wallet_by_pid(wallet.pid);
			entry.coin_publication_started = entry.publication_required;
			if ((stale.wallet_stale && character &&
			     !currency_transaction_publish_wallet(character, stale.current.wallet,
								  stale.current.wallet_revision)) ||
			    (stale.bank_stale &&
			     !publish_bank(character, wallet.account_name.data(), wallet.racewar,
					   stale.current.bank, stale.current.bank_revision)))
				return retain_unresolved_publication(
					entry, "invalid_stale_coin_authority", true);
			// The original transfer remains rejected. Only the live projection read
			// under the failed endpoint's row locks is repaired, including a retained
			// body that reconnect will reuse. An unloaded player loads it on re-entry.
			entry.coin_wallets_published = true;
		}
		if (committed && entry.publication_required && entry.coin_wallets_published)
		{
			const coin_transfer_endpoint *endpoints[] = { &entry.coin->source,
								      &entry.coin->destination };
			for (size_t index = 0; index < 2; ++index)
			{
				if (endpoints[index]->change.type !=
				    critical_command_type::account_bank)
					continue;
				currency_command_payload wallet = {};
				if (!currency_command_decode_payload(endpoints[index]->change,
								     &wallet))
					return retain_unresolved_publication(
						entry, "invalid_coin_endpoint", true);
				P_char current = coin_wallet_by_pid(wallet.pid);
				if (coin_has_physical_endpoint(*entry.coin) && current &&
				    !coin_body_identity_matches(current, wallet))
					return retain_unresolved_publication(
						entry, "coin_body_authority_conflict", false);
				if (current &&
				    (current->runtime_id != entry.coin_wallet_body_ids[index] ||
				     !coin_body_matches(current, result.wallets[index])))
					entry.coin_wallets_published = false;
			}
		}
		if (committed && !entry.coin_wallets_published)
		{
			const coin_transfer_endpoint *endpoints[] = { &entry.coin->source,
								      &entry.coin->destination };
			if (entry.publication_required)
			{
				// Validate every projection before the first native assignment. The
				// started latch remains set even if a later shared-bank publisher throws.
				for (size_t index = 0; index < 2; ++index)
				{
					if (endpoints[index]->change.type !=
					    critical_command_type::account_bank)
						continue;
					currency_command_payload wallet;
					if (!currency_command_decode_payload(
						    endpoints[index]->change, &wallet))
						return retain_unresolved_publication(
							entry, "invalid_coin_endpoint", true);
					const auto valid = [](const currency_vector &vector)
					{
						return std::all_of(vector.amount.begin(),
								   vector.amount.end(),
								   [](int64_t amount) {
									   return amount >= 0 &&
										  amount <= INT_MAX;
								   });
					};
					if (!valid(result.wallets[index].wallet) ||
					    !valid(result.wallets[index].bank))
						return retain_unresolved_publication(
							entry, "invalid_coin_live_balances", true);
				}
				entry.coin_publication_started = true;
			}
			for (size_t index = 0; index < 2; ++index)
			{
				if (endpoints[index]->change.type !=
				    critical_command_type::account_bank)
					continue;
				currency_command_payload wallet;
				if (!currency_command_decode_payload(endpoints[index]->change,
								     &wallet))
					return retain_unresolved_publication(
						entry, "invalid_coin_endpoint", true);
				P_char character = coin_wallet_by_pid(wallet.pid);
				const auto &balances = result.wallets[index];
				const uint64_t body_id = character ? character->runtime_id : 0;
				if (entry.publication_required &&
				    coin_has_physical_endpoint(*entry.coin) && character &&
				    (character->only.pc->wallet_revision >
					     balances.wallet_revision ||
				     character->only.pc->bank_revision > balances.bank_revision ||
				     !coin_body_identity_matches(character, wallet)))
					return retain_unresolved_publication(
						entry, "coin_body_authority_conflict", false);
				// Only unloaded endpoints need a fresh load. Publish retained bodies
				// now, and publish shared bank authority even if the body is unloaded.
				if (character &&
				    !currency_transaction_publish_balances(
					    character, wallet.account_name.data(), wallet.racewar,
					    balances.wallet, balances.bank,
					    balances.wallet_revision, balances.bank_revision))
					return retain_unresolved_publication(
						entry, "invalid_coin_live_balances", true);
				if (!character && !publish_bank(nullptr, wallet.account_name.data(),
								wallet.racewar, balances.bank,
								balances.bank_revision))
					return retain_unresolved_publication(
						entry, "invalid_coin_live_bank", true);
				if (entry.publication_required &&
				    coin_has_physical_endpoint(*entry.coin) &&
				    (entry.disposition_blocked ||
				     currency_publication_state_is_blocked(entry.publication_state)))
					return false;
				entry.coin_wallet_body_ids[index] = body_id;
				if (entry.publication_required &&
				    coin_has_physical_endpoint(*entry.coin))
				{
					P_char current = coin_wallet_by_pid(wallet.pid);
					if (current && !coin_body_identity_matches(current, wallet))
						return retain_unresolved_publication(
							entry, "coin_body_authority_conflict",
							false);
					if (current && (current->runtime_id != body_id ||
							!coin_body_matches(current, balances)))
					{
						entry.publication_state =
							currency_publication_state::retrying_callback;
						return false;
					}
				}
			}
			entry.coin_wallets_published = true;
		}
	}
	catch (...)
	{
		if (!entry.publication_required)
			throw;
		if (entry.disposition_blocked ||
		    currency_publication_state_is_blocked(entry.publication_state))
			return false;
		// Keep the original schema2 projection eligible after transient failures.
		// The dispatch loop bounds retry work; schema1 retains its legacy cap.
		entry.publication_state = currency_publication_state::retrying_callback;
		return false;
	}
	if (entry.publication_required)
	{
		guard.release();
		return publish_accounted_coin(pending.find(original_key), actor, committed, result);
	}
	// Extract the node so a successful callback can advance a bulk command. A
	// temporarily unavailable live destination keeps this same operation for retry.
	auto node = pending.extract(found);
	auto &finished = node.mapped();
	bool published = true;
	try
	{
		if (finished.coin_completion)
			published = finished.coin_completion(actor, committed, *finished.coin,
							     result, finished.completed.error_code,
							     finished.context.data(),
							     finished.context_size);
	}
	catch (const std::bad_alloc &)
	{
		published = false;
	}
	if (!published)
	{
		if (++finished.publication_attempts < CURRENCY_COIN_PUBLICATION_MAX_ATTEMPTS)
		{
			finished.publication_state = currency_publication_state::retrying_callback;
			pending.insert(std::move(node));
			return false;
		}
		// The authority already owns this result. Never report a rejected debit or
		// refund it merely because a live container vanished. Retire the hot retry;
		// the durable command and custody payload remain available for recovery.
		++health.publication_abandoned;
		char operation[33];
		critical_operation_id_to_hex(finished.completed.operation_id, operation,
					     sizeof(operation));
		persistence_alert(AVATAR, "currency", "coin_publication", operation, "none",
				  "publication_abandoned", "pid=%u committed=%d attempts=%u",
				  finished.pid, committed, finished.publication_attempts);
		if (committed && finished.coin_completion)
		{
			try
			{
				(void)finished.coin_completion(actor, true, *finished.coin, result,
							       EOWNERDEAD, finished.context.data(),
							       finished.context_size);
			}
			catch (const std::bad_alloc &)
			{
			}
		}
		if (actor)
			send_to_char(
				"Your coin transfer was saved, but its items could not be updated here. Please contact staff for recovery.\r\n",
				actor);
	}
	if (committed)
		++health.committed;
	else
		++health.rejected;
	return true;
}

bool publish(std::unordered_map<std::string, pending_currency>::iterator found, P_char character)
{
	pending_currency &entry = found->second;
	const critical_completion &completion = entry.completed;
	if (entry.disposition_blocked)
		return false;
	if (!critical_completion_disposition_valid(completion))
	{
		entry.disposition_blocked = true;
		return retain_unresolved_publication(entry, "invalid_completion_disposition", true);
	}
	const bool committed = completion.outcome == critical_apply_outcome::applied ||
			       completion.outcome == critical_apply_outcome::already_applied;
	// Retry exhaustion ends automatic execution, not the transaction's uncertainty.
	// Keep the original identity and continuation until an exact final receipt arrives.
	if (!committed && completion.outcome != critical_apply_outcome::terminal_failure)
		return retain_unresolved_publication(entry, "unresolved_outcome", false);
	if (entry.coin)
		return publish_coin(found, character);
	currency_command_result result = {};
	unsigned int error_code = completion.error_code;
	if (!currency_command_decode_result(completion.result_payload.data(),
					    completion.result_size, &result))
	{
		if (committed)
			return retain_unresolved_publication(entry, "invalid_result", true);
		// A known rejection can have no balance payload (for example validation failure).
		result = {};
		if (completion.result_size)
			++health.malformed_completions;
	}
	else if (!currency_transaction_publish_balances(
			 character, entry.account_name.data(), entry.racewar, result.wallet,
			 result.bank, result.wallet_revision, result.bank_revision))
	{
		if (committed)
			return retain_unresolved_publication(entry, "invalid_live_balances", true);
		++health.malformed_completions;
		result = {};
		if (!error_code)
			error_code = ERANGE;
	}
	// Schema-2 bank work remains fenced until its live projection is published.
	// A failed checkpoint must keep the original ID and callback for retry; do
	// not report a committed operation as rejected or issue another debit.
	if (entry.publication_required &&
	    completion.disposition != critical_completion_disposition::never_admitted &&
	    !critical_command_coordinator_acknowledge_publication(completion.operation_id))
	{
		entry.publication_state = currency_publication_state::retrying_callback;
		return false;
	}
	// The callback may submit/rehash pending or tear down the character. Own its
	// context independently and release this publication guard before calling it.
	auto node = pending.extract(found);
	const auto &finished = node.mapped();
	if (committed)
		++health.committed;
	else
		++health.rejected;
	if (finished.completion)
		finished.completion(character, committed, result, error_code,
				    finished.context.data(), finished.context_size);
	return true;
}

bool stage_publication_receipt(pending_currency &entry, const critical_completion &completion,
			       bool retry_same_blocked_receipt)
{
	if (entry.restored_coin_command && entry.coin && coin_has_physical_endpoint(*entry.coin))
	{
		if (entry.restored_coin_receipt_sealed)
		{
			if (!same_restored_coin_receipt(entry.completed, completion))
			{
				entry.disposition_blocked = true;
				return retain_unresolved_publication(
					entry, "changed_restored_coin_receipt", true);
			}
			// Keep the first definitive original receipt. Delivery attempts,
			// timestamps and applied/already_applied do not replace its body.
			entry.disposition_blocked = false;
			entry.publication_state = currency_publication_state::ready;
			return true;
		}
		if (restored_coin_definitive_receipt(entry, completion))
		{
			entry.completed = completion;
			entry.receipt_received = true;
			entry.restored_coin_receipt_sealed = true;
			entry.disposition_blocked = false;
			entry.publication_state = currency_publication_state::ready;
			return true;
		}
	}
	if (!critical_completion_disposition_valid(completion) ||
	    (entry.receipt_received && entry.completed.disposition != completion.disposition))
	{
		// A durable result cannot later become a no-obligation rejection. Keep
		// the original receipt; reconnect and ordinary pulses cannot discharge it.
		entry.disposition_blocked = true;
		return retain_unresolved_publication(entry, "invalid_completion_disposition", true);
	}
	const bool repairing_disposition = entry.disposition_blocked;
	if (entry.coin && entry.publication_required && entry.coin_publication_started)
	{
		const bool committed = completion.outcome == critical_apply_outcome::applied ||
				       completion.outcome ==
					       critical_apply_outcome::already_applied;
		const bool original_committed =
			entry.completed.outcome == critical_apply_outcome::applied ||
			entry.completed.outcome == critical_apply_outcome::already_applied;
		if (completion.disposition != entry.completed.disposition ||
		    committed != original_committed ||
		    completion.durable_revision != entry.completed.durable_revision ||
		    completion.error_code != entry.completed.error_code ||
		    completion.failure_stage != entry.completed.failure_stage ||
		    completion.result_size != entry.completed.result_size ||
		    completion.result_payload != entry.completed.result_payload)
			return retain_unresolved_publication(
				entry, "changed_coin_publication_receipt", true);
	}
	if (!retry_same_blocked_receipt && !repairing_disposition &&
	    currency_publication_state_is_blocked(entry.publication_state) &&
	    same_publication_receipt(entry.completed, completion))
		return false;
	entry.completed = completion;
	entry.receipt_received = true;
	entry.disposition_blocked = false;
	entry.publication_state = currency_publication_state::ready;
	return true;
}

bool publish_completed_if_available(const std::string &key,
				    const critical_operation_id &operation_id, P_char character)
{
	critical_completion completion = {};
	if (!critical_command_coordinator_get_completed(operation_id, &completion))
		return false;
	auto found = pending.find(key);
	if (found == pending.end())
		return false;
	if (!stage_publication_receipt(found->second, completion, true))
		return false;
	publish(found, character);
	return true;
}

void update_retained_health()
{
	health.pending = pending.size();
	health.retained_offline = 0;
	health.publication_blocked = 0;
	health.publication_retrying = 0;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.publication_state == currency_publication_state::waiting_for_player)
			++health.retained_offline;
		else if (currency_publication_state_is_blocked(entry.publication_state))
			++health.publication_blocked;
		else if (entry.publication_state == currency_publication_state::retrying_callback)
			++health.publication_retrying;
	}
}

currency_vector canonical_value(int64_t value)
{
	currency_vector result = {};
	static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100,
										     1000 };
	for (size_t index = values.size(); index-- > 0;)
	{
		result.amount[index] = value / values[index];
		value %= values[index];
	}
	return result;
}
bool wallet_value_delta(P_char character, int64_t value_delta, currency_vector *delta)
{
	if (!value_delta || value_delta == INT64_MIN)
		return false;
	currency_vector wallet_delta = {};
	if (value_delta > 0)
		wallet_delta = canonical_value(value_delta);
	else
	{
		if (!character || IS_NPC(character))
			return false;
		const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> current = {
			GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
			GET_PLATINUM(character)
		};
		static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10,
											     100,
											     1000 };
		int64_t total = 0;
		for (size_t index = 0; index < current.size(); ++index)
		{
			if (current[index] > (INT64_MAX - total) / values[index])
				return false;
			total += current[index] * values[index];
		}
		const int64_t spend = -value_delta;
		if (total < spend)
			return false;
		const currency_vector after = canonical_value(total - spend);
		for (size_t index = 0; index < current.size(); ++index)
			wallet_delta.amount[index] = after.amount[index] - current[index];
	}
	*delta = wallet_delta;
	return true;
}
} // namespace

bool currency_transaction_publish_balances(P_char character, const char *account_name,
					   uint8_t racewar, const currency_vector &wallet,
					   const currency_vector &bank, uint64_t wallet_revision,
					   uint64_t bank_revision)
{
	if (!character || !character->only.pc || !account_name)
		return false;
	for (int64_t amount : bank.amount)
		if (amount < 0 || amount > INT_MAX)
			return false;
	if (!currency_transaction_publish_wallet(character, wallet, wallet_revision))
		return false;
	return publish_bank(character, account_name, racewar, bank, bank_revision);
}

bool currency_transaction_publish_wallet(P_char character, const currency_vector &wallet,
					 uint64_t wallet_revision)
{
	if (!character || !character->only.pc)
		return false;
	for (int64_t amount : wallet.amount)
		if (amount < 0 || amount > INT_MAX)
			return false;
	// Re-entry may have loaded a later commit while this completion was retained
	// offline. Finish its callback without rolling the authoritative wallet back.
	if (wallet_revision < character->only.pc->wallet_revision)
		return true;
	GET_COPPER(character) = static_cast<int>(wallet.amount[0]);
	GET_SILVER(character) = static_cast<int>(wallet.amount[1]);
	GET_GOLD(character) = static_cast<int>(wallet.amount[2]);
	GET_PLATINUM(character) = static_cast<int>(wallet.amount[3]);
	character->only.pc->wallet_revision = wallet_revision;
	gmcp_char_vitals(character);
	return true;
}

static bool currency_transaction_can_admit(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    pending.size() >= CURRENCY_PENDING_MAX)
		return false;
#ifdef __NO_MYSQL__
	// A failed first save/hydration must not submit zero or partial revisions.
	if (IS_SET(character->runtime_flags, CHAR_RFLAG_NO_DB_BASELINE))
		return false;
#endif
	const char *account_name = get_account_name_safe(character);
	if (!account_name || !strcmp(account_name, "Unknown") ||
	    strlen(account_name) > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	critical_entity_key account_key = {};
	const critical_entity_key player_key = { critical_entity_type::player,
						 static_cast<uint64_t>(GET_PID(character)) };
	return currency_account_key(account_name, static_cast<uint8_t>(GET_RACEWAR(character)),
				    &account_key) &&
	       !critical_command_coordinator_is_fenced(player_key, nullptr) &&
	       !critical_command_coordinator_is_fenced(account_key, nullptr);
}

bool currency_transaction_can_submit_nonrebasable(P_char character)
{
	return currency_transaction_can_admit(character) &&
	       !currency_transaction_player_busy(character);
}

static bool pending_affects_character(const pending_currency &entry, uint32_t pid, uint8_t racewar,
				      bool account_known, const char *account_name)
{
	if (entry.coin && entry.coin_wallets_published && !entry.publication_required)
		return false;
	if (entry.coin)
	{
		for (const auto *endpoint : { &entry.coin->source, &entry.coin->destination })
		{
			currency_command_payload wallet;
			if (endpoint->change.type == critical_command_type::account_bank &&
			    currency_command_decode_payload(endpoint->change, &wallet) &&
			    (wallet.pid == pid ||
			     (account_known && wallet.racewar == racewar &&
			      !strcasecmp(wallet.account_name.data(), account_name))))
				return true;
		}
	}
	return entry.pid == pid || (account_known && entry.racewar == racewar &&
				    !strcasecmp(entry.account_name.data(), account_name));
}

static bool currency_transaction_publication_blocked(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return false;
	// The normal reward path stays O(1). Ownership is scanned only while an
	// exceptional unresolved publication exists.
	if (!health.publication_blocked)
		return false;
	const char *account_name = get_account_name_safe(character);
	const bool account_known = account_name && strcmp(account_name, "Unknown");
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	const uint8_t racewar = static_cast<uint8_t>(GET_RACEWAR(character));
	return std::any_of(pending.begin(), pending.end(),
			   [pid, racewar, account_known, account_name](const auto &item)
			   {
				   return currency_publication_state_is_blocked(
						  item.second.publication_state) &&
					  pending_affects_character(item.second, pid, racewar,
								    account_known, account_name);
			   });
}

bool currency_transaction_player_busy(P_char character)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0)
		return false;
	const char *account_name = get_account_name_safe(character);
	const bool account_known = account_name && strcmp(account_name, "Unknown");
	const uint32_t pid = static_cast<uint32_t>(GET_PID(character));
	const uint8_t racewar = static_cast<uint8_t>(GET_RACEWAR(character));
	return std::any_of(pending.begin(), pending.end(),
			   [pid, racewar, account_known, account_name](const auto &item) {
				   return pending_affects_character(item.second, pid, racewar,
								    account_known, account_name);
			   });
}

bool currency_transaction_coin_item_busy(uint64_t item_uid)
{
	if (!item_uid)
		return false;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (!entry.coin)
			continue;
		for (const auto *endpoint : { &entry.coin->source, &entry.coin->destination })
		{
			if (endpoint->change.type != critical_command_type::item_transfer)
				continue;
			item_transfer_payload pile;
			if (item_transfer_command_decode_payload(endpoint->change, &pile) &&
			    (pile.selected_item_uid == item_uid ||
			     pile.target_parent_item_uid == item_uid ||
			     pile.target_root_item_uid == item_uid ||
			     pile.items[0].root_item_uid == item_uid))
				return true;
		}
	}
	return false;
}

static bool coin_wallet_delta(P_char character, const currency_vector &delta,
			      coin_transfer_endpoint *endpoint)
{
	if (!endpoint || !currency_transaction_can_submit_nonrebasable(character))
		return false;
	coin_transfer_endpoint candidate;
	candidate.before = { GET_COPPER(character), GET_SILVER(character), GET_GOLD(character),
			     GET_PLATINUM(character) };
	currency_command_payload wallet = {};
	wallet.pid = GET_PID(character);
	wallet.racewar = GET_RACEWAR(character);
	wallet.reason = currency_reason_type::coin_transfer;
	const char *account = get_account_name_safe(character);
	memcpy(wallet.account_name.data(), account, strlen(account));
	for (size_t index = 0; index < 4; ++index)
	{
		const int64_t after = int64_t(candidate.before[index]) + delta.amount[index];
		if (candidate.before[index] < 0 || after < 0 || after > INT32_MAX)
			return false;
		candidate.after[index] = after;
		wallet.wallet_delta.amount[index] = delta.amount[index];
	}
	critical_operation_id id;
	if (!critical_operation_id_generate(&id) ||
	    !currency_command_build(
		    &candidate.change, id, wallet, character->only.pc->wallet_revision,
		    character->only.pc->bank_revision, critical_source_site::command,
		    critical_deadline_class::interactive))
		return false;
	*endpoint = std::move(candidate);
	return true;
}

bool currency_transaction_coin_wallet(P_char character, int64_t value_delta,
				      coin_transfer_endpoint *endpoint)
{
	currency_vector delta;
	return wallet_value_delta(character, value_delta, &delta) &&
	       coin_wallet_delta(character, delta, endpoint);
}

bool currency_transaction_coin_wallet_exact(P_char character, uint8_t denomination, int32_t amount,
					    bool debit, coin_transfer_endpoint *endpoint)
{
	if (denomination >= CURRENCY_DENOMINATION_COUNT || amount <= 0)
		return false;
	currency_vector delta = {};
	delta.amount[denomination] = debit ? -static_cast<int64_t>(amount) : amount;
	return coin_wallet_delta(character, delta, endpoint);
}

bool currency_transaction_submit_coin(P_char actor, const coin_transfer_payload &payload,
				      coin_completion_fn completion, const void *context,
				      size_t context_size, coin_publication_callbacks publication)
{
	if (!currency_transaction_can_submit_nonrebasable(actor) ||
	    context_size > CURRENCY_PENDING_CONTEXT_MAX_BYTES || (context_size && !context))
		return false;
	critical_operation_id id;
	critical_command command;
	const char *error = nullptr;
	if (!critical_operation_id_generate(&id))
		return false;
	if (!coin_transfer_command_build(&command, id, payload, critical_source_site::command,
					 critical_deadline_class::interactive, &error))
	{
		logit(LOG_DEBUG, "Coin transfer rejected for player %d: %s", GET_PID(actor),
		      error ? error : "unknown build failure");
		return false;
	}
	if (economic_gameplay_authority::prepare_coin_transfer(&command) !=
	    economic_accounting_error::ok)
		return false;
	const bool publication_required = command.schema_version ==
					  CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION;
	// Unsupported schema-2 physical routes refuse before coordinator admission.
	// Replay remains retained because that operation has already been admitted.
	if (publication_required && coin_has_physical_endpoint(payload) && !publication.publish)
		return false;
	for (const auto &key : command.keys)
		if (critical_command_coordinator_is_fenced(key, nullptr))
			return false;
	for (const auto *endpoint : { &payload.source, &payload.destination })
		if (endpoint->change.type == critical_command_type::account_bank)
		{
			currency_command_payload wallet;
			if (!currency_command_decode_payload(endpoint->change, &wallet))
				return false;
			P_char character = live_wallet_by_pid(wallet.pid);
			if (!character || currency_transaction_player_busy(character))
				return false;
		}
	const std::string key = operation_key(id);
	try
	{
		pending_currency entry = {};
		entry.pid = GET_PID(actor);
		entry.racewar = GET_RACEWAR(actor);
		const char *account = get_account_name_safe(actor);
		memcpy(entry.account_name.data(), account, strlen(account));
		entry.coin = payload;
		entry.coin_completion = completion;
		entry.coin_publication = publication;
		entry.publication_required = publication_required;
		entry.context_size = context_size;
		if (context_size)
			memcpy(entry.context.data(), context, context_size);
		pending.emplace(key, std::move(entry));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	const auto submitted =
		publication_required ?
			critical_command_coordinator_submit_for_publication(command) :
			critical_command_coordinator_submit(std::move(command));
	if (!critical_submit_result_keeps_operation(submitted))
	{
		pending.erase(key);
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	if (submitted == critical_submit_result::attached)
		publish_completed_if_available(key, id, actor);
	update_retained_health();
	return true;
}

bool currency_transaction_submit(P_char character, const currency_vector &wallet_delta,
				 const currency_vector &bank_delta, currency_reason_type reason,
				 int64_t reason_id, critical_source_site source_site,
				 critical_deadline_class deadline_class,
				 currency_completion_fn completion, const void *context,
				 size_t context_size)
{
	critical_operation_id operation_id = {};
	return critical_operation_id_generate(&operation_id) &&
	       currency_transaction_submit_identified(character, operation_id, wallet_delta,
						      bank_delta, reason, reason_id, source_site,
						      deadline_class, completion, context,
						      context_size);
}

bool currency_transaction_submit_identified(
	P_char character, const critical_operation_id &operation_id,
	const currency_vector &wallet_delta, const currency_vector &bank_delta,
	currency_reason_type reason, int64_t reason_id, critical_source_site source_site,
	critical_deadline_class deadline_class, currency_completion_fn completion,
	const void *context, size_t context_size)
{
	if (!character || IS_NPC(character) || GET_PID(character) <= 0 ||
	    context_size > CURRENCY_PENDING_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    pending.size() >= CURRENCY_PENDING_MAX)
		return false;
	const char *account_name = get_account_name_safe(character);
	if (!account_name || !strcmp(account_name, "Unknown") ||
	    strlen(account_name) > CURRENCY_ACCOUNT_NAME_MAX_BYTES)
		return false;
	critical_entity_key account_key = {};
	const critical_entity_key player_key = { critical_entity_type::player,
						 static_cast<uint64_t>(GET_PID(character)) };
	currency_command_payload payload = { .pid = static_cast<uint32_t>(GET_PID(character)),
					     .racewar =
						     static_cast<uint8_t>(GET_RACEWAR(character)),
					     .reason = reason,
					     .reason_id = reason_id,
					     .account_name = {},
					     .wallet_delta = wallet_delta,
					     .bank_delta = bank_delta };
	memcpy(payload.account_name.data(), account_name, strlen(account_name));
	const bool rebasable_reward = currency_command_is_rebasable_reward(payload);
	if (!currency_account_key(account_name, static_cast<uint8_t>(GET_RACEWAR(character)),
				  &account_key) ||
	    (!rebasable_reward && (critical_command_coordinator_is_fenced(player_key, nullptr) ||
				   critical_command_coordinator_is_fenced(account_key, nullptr))))
		return false;
	if (critical_operation_id_is_zero(operation_id))
		return false;
	critical_command command = {};
	const uint64_t expected_wallet_revision =
		rebasable_reward ? UINT64_MAX : character->only.pc->wallet_revision;
	const uint64_t expected_bank_revision =
		rebasable_reward ? UINT64_MAX : character->only.pc->bank_revision;
	if (!currency_command_build(&command, operation_id, payload, expected_wallet_revision,
				    expected_bank_revision, source_site, deadline_class))
		return false;
	// Freeze the lifecycle-issued lifetime/epoch before durable admission. This
	// is not a lookup from the game thread and never rewrites a retained command.
	if (economic_gameplay_authority::prepare_currency(&command) !=
	    economic_accounting_error::ok)
		return false;
	return currency_transaction_submit_prepared(character, command, completion, context,
						    context_size);
}

bool currency_transaction_submit_prepared(P_char character, const critical_command &command,
					  currency_completion_fn completion, const void *context,
					  size_t context_size)
{
	currency_command_payload payload = {};
	if (!character || IS_NPC(character) || !character->only.pc ||
	    context_size > CURRENCY_PENDING_CONTEXT_MAX_BYTES || (context_size && !context) ||
	    !currency_command_decode_payload(command, &payload) ||
	    payload.pid != static_cast<uint32_t>(GET_PID(character)) ||
	    payload.racewar != static_cast<uint8_t>(GET_RACEWAR(character)))
		return false;
	const char *account = get_account_name_safe(character);
	if (!account || strcasecmp(account, payload.account_name.data()))
		return false;
	const auto &operation_id = command.operation_id;
	const auto existing = pending.find(operation_key(operation_id));
	if (existing != pending.end())
		return true;
	// A producer-written timestamp is not proof of prior coordinator admission.
	// New prepared commands must already carry accounting intent in active mode;
	// never retag a command that may have been durably stored by its producer.
	// Already-pending attachments above and validated journal replay are separate.
	if (economic_gameplay_authority::active() &&
	    command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION)
		return false;
	if (pending.size() >= CURRENCY_PENDING_MAX)
		return false;
	// A database receipt can release coordinator fences while live publication is
	// still unresolved. Do not build a new debit from that stale wallet/bank view.
	const bool rebasable_reward = currency_command_is_rebasable_reward(payload);
	if (rebasable_reward)
	{
		if (currency_transaction_publication_blocked(character))
			return false;
	}
	else
	{
		if (currency_transaction_player_busy(character))
			return false;
		for (const auto &entity : command.keys)
		{
			critical_operation_id fence = {};
			if (critical_command_coordinator_is_fenced(entity, &fence) &&
			    !critical_operation_id_equal(fence, operation_id))
				return false;
		}
	}
	pending_currency entry = {
		.pid = static_cast<uint32_t>(GET_PID(character)),
		.account_name = payload.account_name,
		.racewar = payload.racewar,
		.completion = completion,
		.context = {},
		.context_size = context_size,
		.publication_state = currency_publication_state::awaiting_completion,
		.completed = {},
		.publication_required = command.schema_version ==
					CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION
	};
	if (context_size)
		memcpy(entry.context.data(), context, context_size);
	const std::string key = operation_key(operation_id);
	try
	{
		pending.emplace(key, entry);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	critical_submit_result submitted;
	try
	{
		submitted = entry.publication_required ?
				    critical_command_coordinator_submit_for_publication(command) :
				    critical_command_coordinator_submit(command);
	}
	catch (const std::bad_alloc &)
	{
		pending.erase(key);
		return false;
	}
	if (!critical_submit_result_keeps_operation(submitted))
	{
		pending.erase(key);
		++health.submission_failures;
		return false;
	}
	++health.submitted;
	if (submitted == critical_submit_result::attached)
		publish_completed_if_available(key, operation_id, character);
	health.pending = pending.size();
	return true;
}

bool currency_transaction_submit_wallet_value(P_char character, int64_t value_delta,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size)
{
	currency_vector wallet_delta;
	if (!wallet_value_delta(character, value_delta, &wallet_delta))
		return false;
	return currency_transaction_submit(character, wallet_delta, {}, reason, reason_id,
					   source_site, deadline_class, completion, context,
					   context_size);
}

bool currency_transaction_submit_wallet_value_identified(
	P_char character, const critical_operation_id &operation_id, int64_t value_delta,
	currency_reason_type reason, int64_t reason_id, critical_source_site source_site,
	critical_deadline_class deadline_class, currency_completion_fn completion,
	const void *context, size_t context_size)
{
	if (value_delta <= 0)
		return false;
	return currency_transaction_submit_identified(character, operation_id,
						      canonical_value(value_delta), {}, reason,
						      reason_id, source_site, deadline_class,
						      completion, context, context_size);
}

bool currency_transaction_submit_bank_reward(P_char character, int64_t value,
					     currency_reason_type reason, int64_t reason_id,
					     critical_source_site source_site,
					     critical_deadline_class deadline_class,
					     currency_completion_fn completion, const void *context,
					     size_t context_size)
{
	if (value <= 0)
		return false;
	return currency_transaction_submit(character, {}, canonical_value(value), reason, reason_id,
					   source_site, deadline_class, completion, context,
					   context_size);
}

static bool bank_payment_deltas(P_char character, int64_t value, currency_vector *wallet,
				currency_vector *bank)
{
	if (!character || IS_NPC(character) || value <= 0)
		return false;
	const std::array<int64_t, CURRENCY_DENOMINATION_COUNT> current = {
		GET_BALANCE_COPPER(character), GET_BALANCE_SILVER(character),
		GET_BALANCE_GOLD(character), GET_BALANCE_PLATINUM(character)
	};
	static constexpr std::array<int64_t, CURRENCY_DENOMINATION_COUNT> values = { 1, 10, 100,
										     1000 };
	int64_t total = 0;
	for (size_t index = 0; index < current.size(); ++index)
	{
		if (current[index] > (INT64_MAX - total) / values[index])
			return false;
		total += current[index] * values[index];
	}
	if (total < value)
		return false;
	currency_vector bank_delta = {};
	int64_t remaining = value;
	for (size_t index = 0; index < current.size() && remaining > 0; ++index)
	{
		const int64_t needed = (remaining + values[index] - 1) / values[index];
		const int64_t used = std::min(current[index], needed);
		bank_delta.amount[index] = -used;
		remaining -= used * values[index];
	}
	currency_vector wallet_delta = {};
	if (remaining < 0)
		wallet_delta = canonical_value(-remaining);
	*wallet = wallet_delta;
	*bank = bank_delta;
	return true;
}

bool currency_transaction_prepare_identify(P_char character, int64_t cost,
					   critical_command *command)
{
	if (!command || cost <= 0 || !currency_transaction_can_submit_nonrebasable(character))
		return false;
	currency_vector wallet_delta = {}, bank_delta = {};
	const bool use_bank = GET_MONEY(character) < cost;
	if (use_bank ? !bank_payment_deltas(character, cost, &wallet_delta, &bank_delta) :
		       !wallet_value_delta(character, -cost, &wallet_delta))
		return false;
	currency_command_payload payload = {};
	payload.pid = static_cast<uint32_t>(GET_PID(character));
	payload.racewar = static_cast<uint8_t>(GET_RACEWAR(character));
	payload.reason = use_bank ? currency_reason_type::bank_payment :
				    currency_reason_type::wallet_spend;
	const char *account = get_account_name_safe(character);
	if (!account || strlen(account) > CURRENCY_ACCOUNT_NAME_MAX_BYTES ||
	    !strcmp(account, "Unknown"))
		return false;
	memcpy(payload.account_name.data(), account, strlen(account));
	payload.wallet_delta = wallet_delta;
	payload.bank_delta = bank_delta;
	critical_operation_id id = {};
	if (!critical_operation_id_generate(&id) ||
	    !currency_command_build(command, id, payload, character->only.pc->wallet_revision,
				    character->only.pc->bank_revision,
				    critical_source_site::command,
				    critical_deadline_class::interactive))
		return false;
	// Locker identification persists this command before it is submitted. Refuse
	// unsupported active-mode payments before that durable request is created.
	if (economic_gameplay_authority::prepare_currency(command) != economic_accounting_error::ok)
		return false;
	command->accepted_at_usec = std::chrono::duration_cast<std::chrono::microseconds>(
					    std::chrono::system_clock::now().time_since_epoch())
					    .count();
	return critical_command_normalize(command);
}

bool currency_transaction_submit_bank_payment(P_char character, int64_t value,
					      currency_reason_type reason, int64_t reason_id,
					      critical_source_site source_site,
					      critical_deadline_class deadline_class,
					      currency_completion_fn completion,
					      const void *context, size_t context_size)
{
	currency_vector wallet_delta = {}, bank_delta = {};
	if (!bank_payment_deltas(character, value, &wallet_delta, &bank_delta))
		return false;
	return currency_transaction_submit(character, wallet_delta, bank_delta, reason, reason_id,
					   source_site, deadline_class, completion, context,
					   context_size);
}

void currency_transaction_handle_completions(const critical_completion *completions, size_t count)
{
	if (count && !completions)
		return;
	for (auto &[key, entry] : pending)
	{
		(void)key;
		entry.disposition_blocked_this_batch = false;
	}
	for (size_t index = 0; index < count; ++index)
	{
		auto found = pending.find(operation_key(completions[index].operation_id));
		if (found == pending.end())
			continue;
		if (found->second.disposition_blocked_this_batch)
			continue;
		if (!stage_publication_receipt(found->second, completions[index], false))
		{
			found->second.disposition_blocked_this_batch =
				found->second.disposition_blocked;
			continue;
		}
	}
	// Callbacks may submit the next bulk operation, so do not retain map iterators
	// across them. Coin publication retries once per ordinary coordinator pulse.
	std::array<critical_operation_id, CURRENCY_PENDING_MAX> ready;
	size_t ready_count = 0;
	for (const auto &[key, entry] : pending)
		if (currency_publication_state_is_ready(entry.publication_state) &&
		    ready_count < ready.size())
			memcpy(ready[ready_count++].bytes.data(), key.data(), key.size());
	for (size_t index = 0; index < ready_count; ++index)
	{
		auto found = pending.find(operation_key(ready[index]));
		if (found == pending.end())
			continue;
		P_char character = live_wallet_by_pid(found->second.pid);
		if (character || found->second.coin)
			publish(found, character);
		else
			found->second.publication_state =
				currency_publication_state::waiting_for_player;
	}
	update_retained_health();
}

bool currency_transaction_restore_replayed_command(const critical_command &command)
{
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !command.publication_required)
		return true;
	if (command.type == critical_command_type::coin_transfer)
	{
		coin_transfer_payload coin = {};
		if (!critical_command_envelope_valid(command) ||
		    !coin_transfer_command_decode_payload(command, &coin) ||
		    pending.size() >= CURRENCY_PENDING_MAX)
			return false;
		uint32_t actor_pid = 0;
		bool restored_room_coin = false;
		std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name = {};
		uint8_t racewar = 0;
		for (const auto *endpoint : { &coin.source, &coin.destination })
		{
			if (endpoint->change.type == critical_command_type::account_bank)
			{
				currency_command_payload wallet = {};
				if (!currency_command_decode_payload(endpoint->change, &wallet) ||
				    wallet.reason != currency_reason_type::coin_transfer ||
				    wallet.pid > INT32_MAX)
					return false;
				if (!actor_pid)
				{
					actor_pid = wallet.pid;
					account_name = wallet.account_name;
					racewar = wallet.racewar;
				}
				continue;
			}
			if (endpoint->change.type == critical_command_type::item_transfer)
			{
				item_transfer_payload pile = {};
				if (!item_transfer_command_decode_payload(endpoint->change,
									  &pile) ||
				    pile.item_count != 1 ||
				    pile.selected_item_uid != pile.items[0].item_uid)
					return false;
				restored_room_coin = restored_room_coin_shape(coin, pile);
				continue;
			}
			return false;
		}
		pending_currency entry = { .pid = actor_pid,
					   .account_name = account_name,
					   .racewar = racewar,
					   .completion = nullptr,
					   .context = {},
					   .context_size = 0,
					   .publication_state =
						   currency_publication_state::awaiting_completion,
					   .completed = {},
					   .coin = coin,
					   .coin_completion = nullptr,
					   .coin_wallets_published = false,
					   .publication_attempts = 0,
					   .publication_required = true };
		try
		{
			const std::string key = operation_key(command.operation_id);
			if (pending.find(key) != pending.end())
				return false;
			entry.restored_coin_command = command;
			entry.restored_room_coin = restored_room_coin;
			pending.emplace(key, std::move(entry));
			if (restored_room_coin &&
			    !player_save_pipeline_restore_sql_coin_obligation(command))
			{
				pending.erase(key);
				return false;
			}
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		update_retained_health();
		return true;
	}
	if (command.type != critical_command_type::account_bank)
		return true;
	currency_command_payload payload = {};
	if (!critical_command_envelope_valid(command) ||
	    !currency_command_decode_payload(command, &payload) ||
	    (payload.reason != currency_reason_type::atm_deposit &&
	     payload.reason != currency_reason_type::atm_withdraw) ||
	    pending.size() >= CURRENCY_PENDING_MAX)
		return false;
	pending_currency entry = { .pid = payload.pid,
				   .account_name = payload.account_name,
				   .racewar = payload.racewar,
				   .completion = nullptr,
				   .context = {},
				   .context_size = 0,
				   .publication_state =
					   currency_publication_state::awaiting_completion,
				   .completed = {},
				   .publication_required = true };
	try
	{
		const std::string key = operation_key(command.operation_id);
		if (pending.find(key) != pending.end())
			return false;
		pending.emplace(key, std::move(entry));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	update_retained_health();
	return true;
}

bool currency_transaction_restored_coin_receipt_current(
	const critical_command &original_command,
	const critical_completion &sealed_completion) noexcept
{
	// Bounded scan avoids operation_key's string allocation at the ACK boundary.
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.restored_coin_command &&
		    critical_operation_id_equal(entry.restored_coin_command->operation_id,
						original_command.operation_id))
			return entry.restored_room_coin && entry.restored_coin_receipt_sealed &&
			       entry.coin_publication_in_progress && !entry.disposition_blocked &&
			       !entry.disposition_blocked_this_batch &&
			       !currency_publication_state_is_blocked(entry.publication_state) &&
			       same_restored_coin_command(*entry.restored_coin_command,
							  original_command) &&
			       same_restored_coin_receipt(entry.completed, sealed_completion);
	}
	return false;
}

void currency_transaction_player_ready(P_char character)
{
	if (!character || IS_NPC(character))
		return;
	std::array<critical_operation_id, CURRENCY_PENDING_MAX> ready;
	size_t ready_count = 0;
	for (const auto &[key, entry] : pending)
		if (entry.pid == static_cast<uint32_t>(GET_PID(character)) &&
		    currency_publication_state_is_live_pending(entry.publication_state) &&
		    ready_count < ready.size())
			memcpy(ready[ready_count++].bytes.data(), key.data(), key.size());
	for (size_t index = 0; index < ready_count; ++index)
	{
		auto found = pending.find(operation_key(ready[index]));
		if (found != pending.end())
			publish(found, character);
	}
	update_retained_health();
}

currency_transaction_health currency_transaction_health_copy(void)
{
	update_retained_health();
	return health;
}

void currency_transaction_reset_for_tests(void)
{
	for (const auto &[key, entry] : pending)
		if (entry.coin && entry.publication_required && entry.coin_publication.release)
		{
			critical_operation_id id = {};
			memcpy(id.bytes.data(), key.data(), id.bytes.size());
			entry.coin_publication.release(id);
		}
	pending.clear();
	health = {};
}

namespace
{
[[maybe_unused]] bool currency_replay_add(size_t &bytes, size_t extra) noexcept
{
	if (extra > SIZE_MAX - bytes)
		return false;
	bytes += extra;
	return true;
}
[[maybe_unused]] bool currency_replay_command_heap(const critical_command &command,
						   size_t &bytes) noexcept
{
	return command.keys.capacity() <= SIZE_MAX / sizeof(critical_entity_key) &&
	       currency_replay_add(bytes, command.keys.capacity() * sizeof(critical_entity_key)) &&
	       command.expected_revisions.capacity() <=
		       SIZE_MAX / sizeof(critical_expected_revision) &&
	       currency_replay_add(bytes, command.expected_revisions.capacity() *
						  sizeof(critical_expected_revision)) &&
	       currency_replay_add(bytes, command.payload.capacity()) &&
	       currency_replay_add(bytes, command.accounting_intent.capacity());
}
[[maybe_unused]] bool currency_replay_admit(size_t outer, size_t local, size_t extra,
					    bool (*reserve)(size_t, void *) noexcept,
					    void *context) noexcept
{
	size_t bytes = 0;
	return reserve && currency_transaction_current_storage_bytes(&bytes) &&
	       currency_replay_add(bytes, outer) && currency_replay_add(bytes, local) &&
	       currency_replay_add(bytes, extra) && reserve(bytes, context);
}
}

// Complete CURRENT currency owner, including retained SQL coin commands from
// original routes. Inline table/health, actual buckets/nodes/keys and every deep
// command vector appear once; inline optional endpoint bodies already live in nodes.
bool currency_transaction_current_storage_bytes(size_t *output) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)output;
	return false;
#else
	if (!output)
		return false;
	size_t bytes = 0;
	if (!pending.current_table_heap_bytes(&bytes) ||
	    !currency_replay_add(bytes, sizeof(pending)) ||
	    !currency_replay_add(bytes, sizeof(health)))
		return false;
	for (const auto &[key, entry] : pending)
	{
		(void)key;
		if (entry.coin &&
		    (!currency_replay_command_heap(entry.coin->source.change, bytes) ||
		     !currency_replay_command_heap(entry.coin->destination.change, bytes)))
			return false;
		if (entry.restored_coin_command &&
		    !currency_replay_command_heap(*entry.restored_coin_command, bytes))
			return false;
	}
	*output = bytes;
	return true;
#endif
}

// Explicit bank-only companion, not a complete currency/coin dispatcher. Full
// original schema/publication skips and ATM predicates remain unchanged. The
// caller's authentic exclusive outer excludes this currency owner exactly once.
bool currency_transaction_restore_bank_replayed_command_bounded(
	const critical_command &command, bool (*reserve)(size_t, void *) noexcept, void *context,
	size_t outer_live) noexcept
{
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI || defined(_GLIBCXX_DEBUG)
	(void)command;
	(void)reserve;
	(void)context;
	(void)outer_live;
	return false;
#else
	if (!reserve)
		return false;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !command.publication_required)
		return currency_replay_admit(outer_live, 0, 0, reserve, context);
	if (command.type != critical_command_type::account_bank)
	{
		currency_replay_admit(outer_live, 0, 0, reserve, context);
		return false; // Separate SQL coin owner is intentionally not supplied here.
	}
	using insertion_result = std::pair<currency_pending_table::iterator, bool>;
	const size_t fixed = sizeof(currency_command_payload) + sizeof(pending_currency) +
			     sizeof(std::string) + sizeof(currency_pending_table::iterator);
	const size_t policy_frames =
		sizeof(std::__detail::_Prime_rehash_policy) + 2 * sizeof(std::pair<bool, size_t>);
	// Genuine installed hashtable's _Scoped_node two-pointer owner, temporary
	// saved _State, original policy result/return pairs, actual bucket allocator
	// object, returned iterator and fresh copied-string _Guard. These are dead
	// after emplace returns and never become persistent currency ownership.
	const size_t insertion_frames = 3 * sizeof(void *) +
					sizeof(std::__detail::_Prime_rehash_policy::_State) +
					2 * sizeof(std::pair<bool, size_t>) +
					sizeof(std::allocator<std::__detail::_Hash_node_base *>) +
					sizeof(currency_pending_table::iterator);
	if (!currency_replay_admit(outer_live, fixed, 0, reserve, context))
		return false;
	currency_pending_table::iterator inserted_node = pending.end();
	bool inserted = false, completed = false;
	try
	{
		{
			currency_command_payload payload{};
			pending_currency entry{};
			std::string key;
			do
			{
				if (!critical_command_envelope_valid(command))
					break;
				size_t current = 0;
				if (!currency_transaction_current_storage_bytes(&current) ||
				    !currency_replay_add(current, outer_live) ||
				    !currency_replay_add(current, fixed) ||
				    !currency_command_decode_payload_bounded(
					    command, &payload, reserve, context, current) ||
				    !currency_replay_admit(outer_live, fixed, 0, reserve,
							   context) ||
				    (payload.reason != currency_reason_type::atm_deposit &&
				     payload.reason != currency_reason_type::atm_withdraw) ||
				    pending.size() >= CURRENCY_PENDING_MAX)
					break;
				entry.pid = payload.pid;
				entry.account_name = payload.account_name;
				entry.racewar = payload.racewar;
				entry.publication_required = true;
				const size_t key_request =
					command.operation_id.bytes.size() > 15 ?
						command.operation_id.bytes.size() + 1 :
						0;
				if (!currency_replay_admit(outer_live,
							   fixed + sizeof(std::string) +
								   sizeof(void *),
							   key_request, reserve, context))
					break;
				// Admit the actual original fresh constructor's returned object and
				// 16-byte key heap before its allocation-free move into empty key.
				key = std::string(reinterpret_cast<const char *>(
							  command.operation_id.bytes.data()),
						  command.operation_id.bytes.size());
				const size_t key_heap = key.capacity() > 15 ? key.capacity() + 1 :
									      0;
				if (!currency_replay_admit(outer_live, fixed, key_heap, reserve,
							   context) ||
				    pending.find(key) != pending.end())
					break; // Original duplicates refuse; do not replace or attach.
				size_t extra = 0;
				if (!currency_replay_admit(outer_live, fixed,
							   key_heap + policy_frames, reserve,
							   context) ||
				    !pending.next_bank_insert_extra_peak(key, &extra) ||
				    extra > SIZE_MAX - key_heap - sizeof(insertion_result) -
						    insertion_frames ||
				    !currency_replay_admit(outer_live, fixed,
							   key_heap + extra +
								   sizeof(insertion_result) +
								   insertion_frames,
							   reserve, context))
					break;
				const auto actual = pending.emplace(key, std::move(entry));
				// Mark actual returned insertion before later fallible CURRENT rebase.
				inserted_node = actual.first;
				inserted = actual.second;
				completed = inserted;
			} while (false);
		}
	}
	catch (...)
	{
		completed = false;
	}
	// Private key/payload/entry and profile/returned pair frames have died. Only
	// the actual iterator survives; a snapshot is not a borrowed storage lease.
	const bool refreshed =
		currency_replay_admit(outer_live, sizeof(inserted_node), 0, reserve, context);
	if (!completed || !refreshed)
	{
		// No callback or allocation during refusal cleanup. Remove only this
		// genuinely returned new node; existing entries and actual bucket growth stay.
		if (inserted)
		{
			pending.erase(inserted_node);
			inserted_node = pending.end();
			// Cleanup is complete before refreshing its actual surviving buckets.
			currency_replay_admit(outer_live, sizeof(inserted_node), 0, reserve,
					      context);
		}
		return false;
	}
	update_retained_health(); // Original allocation-free full health tail.
	return true;
#endif
}

#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
namespace
{
constexpr size_t currency_full_allocator_frames =
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
constexpr size_t currency_full_copy_frames =
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
constexpr size_t currency_full_relocate_frames =
	// _S_relocate/__relocate_a/__relocate_a_1, each3 pointers+allocatorref
	// +returned pointer; real niter-base calls/count/memmove scope.
	3 * (4 * sizeof(void *) + sizeof(void *)) + 3 * (sizeof(void *) + sizeof(void *)) +
	sizeof(std::ptrdiff_t) + 3 * sizeof(void *) + sizeof(size_t);
constexpr size_t currency_full_default_frames =
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
constexpr size_t currency_full_vector_frames =
	currency_full_allocator_frames + currency_full_copy_frames + currency_full_relocate_frames +
	currency_full_default_frames +
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
constexpr size_t currency_full_move_frames =
	// vector operator=(vector&&), _M_move_assign(true), actual vector __tmp,
	// _M_swap_data's actual three-pointer _Vector_impl_data __tmp and
	// _M_copy_data reference parameters; real allocator-return/forward.
	3 * sizeof(void *) + sizeof(bool) + 2 * sizeof(void *) + sizeof(char) +
	sizeof(std::vector<uint8_t>) + 3 * sizeof(void *) + 2 * sizeof(void *) +
	2 * sizeof(void *) + sizeof(char) + 2 * sizeof(void *) +
	// temporary destructor and actual default destroy/deallocate closure.
	sizeof(void *) + currency_full_allocator_frames;
constexpr size_t currency_full_vector_constructor_frames =
	2 * sizeof(void *) + 3 * sizeof(std::allocator<int32_t>) + 2 * sizeof(void *) +
	sizeof(size_t) + 4 * sizeof(void *) + sizeof(void *) + sizeof(void *) + sizeof(size_t) +
	8 * (sizeof(void *) + sizeof(size_t)) + currency_full_vector_frames;
constexpr size_t currency_full_command_defaults =
	// Real command generated default/destructor and four vector default
	// constructor/_Vector_base/_Vector_impl/_Vector_impl_data/allocator
	// carriers; current object inline is separately owned by its lifetime.
	2 * sizeof(void *) + 4 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	4 * (sizeof(void *) + currency_full_allocator_frames);
constexpr size_t currency_full_pile_defaults =
	// Same actual ten aggregate defaults/destructors, six vector/base/impl/
	// data defaults, six string/hider/local/NUL defaults in item payload.
	2 * 10 * sizeof(void *) + 6 * (4 * sizeof(void *) + sizeof(std::allocator<uint8_t>)) +
	6 * (7 * sizeof(void *) + sizeof(std::allocator<char>) + sizeof(size_t) + sizeof(char));

constexpr size_t currency_full_optional_defaults =
	// Two actual optional/base/payload/storage disengaged default/destructor
	// this carriers. Actual members inline already owned by pending entry.
	2 * (5 * sizeof(void *));
constexpr size_t currency_full_optional_frames =
	// Actual optional converting constructor -> base(in_place,ref) ->
	// payload(in_place,ref) -> storage(in_place,ref), true empty tag carrier
	// at each layer and forward source refs; direct T member construction.
	4 * (2 * sizeof(void *) + sizeof(std::in_place_t)) + 4 * (2 * sizeof(void *)) +
	// Actual optional operator=(T ref) -> _M_is_engaged/_M_construct ->
	// payload _M_construct -> _Construct(addressof(value),forward(ref)).
	3 * (2 * sizeof(void *)) + 3 * (2 * sizeof(void *)) + 2 * sizeof(void *) +
	3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	// Actual pending-generated move and two optional nontrivial base/payload
	// move constructors: this/ref/bool, _M_get refs, nested construct/move;
	// original optional reset/destruction source scopes, no heap request.
	2 * sizeof(void *) + 2 * (3 * sizeof(void *) + sizeof(bool)) + 4 * (2 * sizeof(void *)) +
	2 * (4 * sizeof(void *) + sizeof(bool)) + currency_full_optional_defaults;
constexpr size_t currency_full_operation_key_frames =
	// Original operation_key reference and returned string carrier, actual
	// string(char*,n,allocator) this/source/n/allocator, _Alloc_hider,
	// _M_construct forward begin/end/dnew/_Guard/_M_create parameters.
	sizeof(void *) + sizeof(std::string) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(std::allocator<char>) + 3 * sizeof(void *) + 2 * sizeof(void *) + sizeof(size_t) +
	sizeof(std::string *) + 3 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(void *) +
	// Actual traits copy(dest,source,n,result), _M_data/_M_capacity/setlength,
	// original string destruction and equal-allocator dispose/deallocate.
	3 * sizeof(void *) + sizeof(size_t) + 6 * (sizeof(void *) + sizeof(size_t)) +
	4 * sizeof(void *) + 2 * sizeof(size_t) + sizeof(bool) + currency_full_allocator_frames;
constexpr size_t currency_full_lookup_erase_frames =
	// Actual hashtable find/erase(key) parameters/return/current node/bucket/
	// before node/hash/code/result, iterative equality/key getters; string
	// hash input and actual string bytes equality argument/size/int result.
	3 * (3 * sizeof(void *) + 3 * sizeof(size_t) + sizeof(bool)) +
	4 * (2 * sizeof(void *) + sizeof(size_t)) + 3 * sizeof(void *) + sizeof(size_t) +
	sizeof(int) +
	// erase unlinks original actual node, destroys pair/key/pending/optional
	// and up to three commands; node/key allocator destructor/deallocation.
	5 * sizeof(void *) + 2 * sizeof(size_t) + currency_full_allocator_frames +
	currency_full_operation_key_frames + currency_full_optional_frames;
struct currency_full_replay_budget
{
	bool (*reserve)(size_t, void *) noexcept;
	bool (*restore)(const critical_command &, void *, size_t) noexcept;
	void *context;
	size_t outer, frames;
	const coin_transfer_payload *coin = nullptr;
	const item_transfer_payload *pile = nullptr;
	const pending_currency *entry = nullptr;
	const currency_command_payload *wallet = nullptr;
	const std::string *key = nullptr;
	bool prefix(size_t &result, size_t extra = 0) const noexcept
	{
		constexpr size_t observation = 13 * sizeof(void *) + 10 * sizeof(size_t) +
					       8 * sizeof(bool) +
					       4 * (sizeof(void *) + sizeof(size_t));
		size_t total = outer, heap = 0;
		if (!currency_transaction_current_storage_bytes(&heap) ||
		    !currency_replay_add(total, heap) ||
		    !currency_replay_add(total, sizeof(*this)) ||
		    !currency_replay_add(total, frames) ||
		    !currency_replay_add(total, observation) ||
		    !currency_replay_add(total, critical_command_copy_frame_bytes()) ||
		    !currency_replay_add(total, critical_command_valid_frame_bytes()) ||
		    !currency_replay_add(total, item_transfer_payload_copy_frame_bytes()))
			return false;
		if (coin && (!currency_replay_add(total, sizeof(*coin)) ||
			     !coin_transfer_payload_current_heap_bytes(*coin, &heap) ||
			     !currency_replay_add(total, heap)))
			return false;
		if (pile && (!currency_replay_add(total, sizeof(*pile)) ||
			     !item_transfer_payload_current_heap_bytes(*pile, &heap) ||
			     !currency_replay_add(total, heap)))
			return false;
		if (entry)
		{
			if (!currency_replay_add(total, sizeof(*entry)))
				return false;
			if (entry->coin &&
			    (!coin_transfer_payload_current_heap_bytes(*entry->coin, &heap) ||
			     !currency_replay_add(total, heap)))
				return false;
			if (entry->restored_coin_command &&
			    (!critical_command_current_heap_bytes(*entry->restored_coin_command,
								  &heap) ||
			     !currency_replay_add(total, heap)))
				return false;
		}
		if (wallet && !currency_replay_add(total, sizeof(*wallet)))
			return false;
		if (key &&
		    (!currency_replay_add(total, sizeof(*key)) ||
		     (key->capacity() > 15 && (!currency_replay_add(total, key->capacity()) ||
					       !currency_replay_add(total, 1)))))
			return false;
		if (!currency_replay_add(total, extra))
			return false;
		result = total;
		return true;
	}
	bool peak(size_t extra = 0) const noexcept
	{
		size_t total = 0;
		return prefix(total, extra) && reserve && reserve(total, context);
	}
	bool coin_copy_request(const coin_transfer_payload &value, size_t &result) const noexcept
	{
		size_t total = 0, request = 0;
		if (!critical_command_fresh_copy_request_bytes(value.source.change, &request) ||
		    !currency_replay_add(total, request) ||
		    !critical_command_fresh_copy_request_bytes(value.destination.change,
							       &request) ||
		    !currency_replay_add(total, request))
			return false;
		result = total;
		return true;
	}
	bool insertion_request(const std::string &value, size_t &result) const noexcept
	{
		constexpr size_t policy_frames = sizeof(std::__detail::_Prime_rehash_policy) +
						 2 * sizeof(std::pair<bool, size_t>) +
						 3 * sizeof(void *) + 3 * sizeof(size_t) +
						 sizeof(bool);
		if (!peak(policy_frames))
			return false;
		size_t request = 0;
		if (!pending.next_bank_insert_extra_peak(value, &request))
			return false;
		using insertion_result = std::pair<currency_pending_table::iterator, bool>;
		// Genuine original table SAME request provider for emplace(key,moved
		// pending_currency), including actual node inline/copy key/new buckets.
		// Preadmit original false-save erase/removal and source destruction
		// before insertion; no reservation callback inside cleanup on denial.
		constexpr size_t insertion_frames =
			3 * sizeof(void *) + sizeof(std::__detail::_Prime_rehash_policy::_State) +
			2 * sizeof(std::pair<bool, size_t>) +
			sizeof(std::allocator<std::__detail::_Hash_node_base *>) +
			sizeof(currency_pending_table::iterator) +
			// Actual emplace/allocate-node/value pair/key-copy member carriers,
			// returned iterator/bool pair and original string copy _Guard.
			4 * (3 * sizeof(void *) + sizeof(size_t)) + sizeof(void *) +
			currency_full_operation_key_frames + currency_full_optional_frames +
			currency_full_lookup_erase_frames;
		return currency_replay_add(request,
					   sizeof(insertion_result) + insertion_frames +
						   3 * critical_command_copy_frame_bytes() +
						   4 * sizeof(void *) + sizeof(bool)) &&
		       (result = request, true);
	}
};
bool currency_full_replay_owned(const critical_command &command,
				currency_full_replay_budget &budget)
{
	size_t admission_prefix = 0, admission_request = 0;
	if (command.schema_version != CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION ||
	    !command.publication_required)
		return true;
	if (command.type == critical_command_type::coin_transfer)
	{
		if (!budget.peak(sizeof(coin_transfer_payload) +
				 currency_full_command_defaults * 2))
			return false;
		coin_transfer_payload coin = {};
		budget.coin = &coin;
		if (!critical_command_envelope_valid(command) ||
		    !(budget.prefix(admission_prefix) &&
		      coin_transfer_command_decode_payload_bounded(
			      command, &coin, budget.reserve, budget.context, admission_prefix)) ||
		    pending.size() >= CURRENCY_PENDING_MAX)
			return false;
		uint32_t actor_pid = 0;
		bool restored_room_coin = false;
		std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1> account_name = {};
		uint8_t racewar = 0;
		for (const auto *endpoint : { &coin.source, &coin.destination })
		{
			if (endpoint->change.type == critical_command_type::account_bank)
			{
				if (!budget.peak(sizeof(currency_command_payload) +
						 2 * sizeof(void *)))
					return false;
				currency_command_payload wallet = {};
				if (!(budget.prefix(admission_prefix, sizeof(wallet)) &&
				      currency_command_decode_payload_bounded(
					      endpoint->change, &wallet, budget.reserve,
					      budget.context, admission_prefix)) ||
				    wallet.reason != currency_reason_type::coin_transfer ||
				    wallet.pid > INT32_MAX)
					return false;
				if (!actor_pid)
				{
					actor_pid = wallet.pid;
					account_name = wallet.account_name;
					racewar = wallet.racewar;
				}
				continue;
			}
			if (endpoint->change.type == critical_command_type::item_transfer)
			{
				if (!budget.peak(sizeof(item_transfer_payload) +
						 currency_full_pile_defaults))
					return false;
				item_transfer_payload pile = {};
				budget.pile = &pile;
				if (!(budget.prefix(admission_prefix) &&
				      item_transfer_command_decode_payload_bounded(
					      endpoint->change, &pile, budget.reserve,
					      budget.context, admission_prefix)) ||
				    pile.item_count != 1 ||
				    pile.selected_item_uid != pile.items[0].item_uid)
					return false;
				restored_room_coin = restored_room_coin_shape(coin, pile);
				budget.pile = nullptr;
				continue;
			}
			return false;
		}
		if (!budget.coin_copy_request(coin, admission_request) ||
		    !currency_replay_add(admission_request,
					 sizeof(pending_currency) + currency_full_optional_frames +
						 2 * critical_command_copy_frame_bytes()) ||
		    !budget.peak(admission_request))
			return false;
		pending_currency entry = { .pid = actor_pid,
					   .account_name = account_name,
					   .racewar = racewar,
					   .completion = nullptr,
					   .context = {},
					   .context_size = 0,
					   .publication_state =
						   currency_publication_state::awaiting_completion,
					   .completed = {},
					   .coin = coin,
					   .coin_completion = nullptr,
					   .coin_wallets_published = false,
					   .publication_attempts = 0,
					   .publication_required = true };
		budget.entry = &entry;
		try
		{
			if (!budget.peak(sizeof(std::string) + currency_full_operation_key_frames +
					 (command.operation_id.bytes.size() > 15 ?
						  command.operation_id.bytes.size() + 1 :
						  0)))
				return false;
			const std::string key = operation_key(command.operation_id);
			budget.key = &key;
			if (pending.find(key) != pending.end())
				return false;
			if (!critical_command_fresh_copy_request_bytes(command,
								       &admission_request) ||
			    !currency_replay_add(admission_request,
						 critical_command_copy_frame_bytes() +
							 currency_full_optional_frames) ||
			    !budget.peak(admission_request))
				return false;
			entry.restored_coin_command = command;
			entry.restored_room_coin = restored_room_coin;
			if (!budget.insertion_request(key, admission_request) ||
			    !budget.peak(admission_request))
				return false;
			pending.emplace(key, std::move(entry));
			if (restored_room_coin &&
			    !(budget.prefix(admission_prefix) &&
			      budget.restore(command, budget.context, admission_prefix)))
			{
				pending.erase(key);
				return false;
			}
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		update_retained_health();
		return true;
	}
	if (command.type != critical_command_type::account_bank)
		return true;
	if (!budget.peak(sizeof(currency_command_payload) + 2 * sizeof(void *)))
		return false;
	currency_command_payload payload = {};
	budget.wallet = &payload;
	if (!critical_command_envelope_valid(command) ||
	    !(budget.prefix(admission_prefix) &&
	      currency_command_decode_payload_bounded(command, &payload, budget.reserve,
						      budget.context, admission_prefix)) ||
	    (payload.reason != currency_reason_type::atm_deposit &&
	     payload.reason != currency_reason_type::atm_withdraw) ||
	    pending.size() >= CURRENCY_PENDING_MAX)
		return false;
	if (!budget.peak(sizeof(pending_currency) + currency_full_optional_defaults))
		return false;
	pending_currency entry = { .pid = payload.pid,
				   .account_name = payload.account_name,
				   .racewar = payload.racewar,
				   .completion = nullptr,
				   .context = {},
				   .context_size = 0,
				   .publication_state =
					   currency_publication_state::awaiting_completion,
				   .completed = {},
				   .publication_required = true };
	budget.entry = &entry;
	try
	{
		if (!budget.peak(sizeof(std::string) + currency_full_operation_key_frames +
				 (command.operation_id.bytes.size() > 15 ?
					  command.operation_id.bytes.size() + 1 :
					  0)))
			return false;
		const std::string key = operation_key(command.operation_id);
		budget.key = &key;
		if (pending.find(key) != pending.end())
			return false;
		if (!budget.insertion_request(key, admission_request) ||
		    !budget.peak(admission_request))
			return false;
		pending.emplace(key, std::move(entry));
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	update_retained_health();
	return true;
}
} // namespace
#endif // genuine complete GCC13 private pending/profile consumers
bool currency_transaction_restore_replayed_command_bounded(const critical_command &command,
							   bool (*reserve)(size_t, void *) noexcept,
							   void *context,
							   size_t outer_live) noexcept
{
	if (!reserve)
		return false;
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI && !defined(_GLIBCXX_DEBUG)
	try
	{
		// Genuine actual private stack scope, not a lease/capability flag.
		// Startup coordinator/main-thread ownership supplies pending stability;
		// original selected currency implementation has no currency mutex.
		player_save_coin_replay_budget_scope_owner scope(reserve, context);
		const auto scoped_reserve = +[](size_t exclusive, void *actual) noexcept {
			return static_cast<player_save_coin_replay_budget_scope_owner *>(actual)
				->admit(exclusive);
		};
		const auto scoped_restore = +[](const critical_command &original, void *actual,
						size_t exclusive) noexcept
		{
			return static_cast<player_save_coin_replay_budget_scope_owner *>(actual)
				->restore(original, exclusive);
		};
		constexpr size_t frames =
			// Genuine public+owned signature/result, two admission scalars,
			// actual named reserve/restore pointer objects and genuine lambda
			// argument/result/conversion carriers. Scope adds its own inline.
			8 * sizeof(void *) + 4 * sizeof(size_t) + 3 * sizeof(bool) +
			2 * sizeof(void *) + 5 * sizeof(void *) + 2 * sizeof(size_t) +
			2 * sizeof(bool) +
			// Original actor/account/racewar/room flag, two endpoint backing
			// pointers/initializer descriptor/current endpoint reference.
			sizeof(uint32_t) + sizeof(bool) +
			sizeof(std::array<char, CURRENCY_ACCOUNT_NAME_MAX_BYTES + 1>) +
			sizeof(uint8_t) + 2 * sizeof(void *) +
			sizeof(std::initializer_list<const coin_transfer_endpoint *>) +
			3 * sizeof(void *) +
			// Actual original restored_room_coin_shape params/drop/pickup,
			// empty lambda and owner predicate refs/results/index getters.
			2 * sizeof(void *) + 2 * sizeof(bool) + sizeof(char) + sizeof(void *) +
			sizeof(bool) + 2 * sizeof(void *) + sizeof(bool) +
			8 * (sizeof(void *) + sizeof(size_t)) +
			// Original successful health update full table iterator/reference,
			// allocation-free publication state scalar predicate and results.
			4 * sizeof(void *) + sizeof(currency_publication_state) + sizeof(bool) +
			currency_full_lookup_erase_frames + currency_full_optional_frames;
		currency_full_replay_budget budget{ scoped_reserve, scoped_restore, &scope,
						    outer_live, frames };
		if (!budget.peak())
			return false;
		// Full original mixed body is the sole owner of insert/duplicate/erase.
		// No callback after actual save hold success or original health tail.
		return currency_full_replay_owned(command, budget);
	}
	catch (...)
	{
		return false;
	}
#else
	(void)command;
	(void)context;
	(void)outer_live;
	return false;
#endif
}
