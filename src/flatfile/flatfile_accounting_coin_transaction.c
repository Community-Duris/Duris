#include "flatfile/flatfile_accounting_coin_transaction.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_accounting_pile_state.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_item_accounting_reference.h"
#include "flatfile/flatfile_item_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/flatfile_store.h"
#include "player/player_snapshot_codec.h"
#include "core/defines.h"
#include "economy/coin_transfer_accounting.h"
#include "economy/economic_accounting_intent.h"
#include "economy/economic_accounting_plan.h"

#include <algorithm>
#include <cerrno>
#include <climits>
#include <new>
#include <optional>

namespace
{
struct failure
{
	unsigned int code;
};
void need(bool condition, unsigned int code = EILSEQ)
{
	if (!condition)
		throw failure{ code };
}
void checked(unsigned int code)
{
	if (code)
		throw failure{ code };
}
void checked(economic_accounting_error code)
{
	need(code == economic_accounting_error::ok,
	     code == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}
void checked(flatfile_accounting_status status)
{
	if (status == flatfile_accounting_status::ok)
		return;
	if (status == flatfile_accounting_status::already_exists ||
	    status == flatfile_accounting_status::conflict)
		throw failure{ EEXIST };
	throw failure{ static_cast<unsigned int>(
		status == flatfile_accounting_status::capacity ? ENOSPC :
		status == flatfile_accounting_status::io_error ? EIO :
								 EILSEQ) };
}
void checked_retained_claim(flatfile_accounting_status status)
{
	// A conflicting immutable claim is unavailable replay proof. New-operation
	// staging still uses checked(status), preserving duplicate-source refusal.
	need(status == flatfile_accounting_status::ok,
	     status == flatfile_accounting_status::io_error ? EIO :
	     status == flatfile_accounting_status::capacity ? ENOSPC :
							      EILSEQ);
}
void checked(flatfile_player_domain_result status)
{
	if (status == flatfile_player_domain_result::ok)
		return;
	throw failure{ static_cast<unsigned int>(
		status == flatfile_player_domain_result::not_found ? ENOENT :
		status == flatfile_player_domain_result::io_error  ? EIO :
								     EILSEQ) };
}
void checked(flatfile_item_repository_result status)
{
	if (status == flatfile_item_repository_result::ok ||
	    status == flatfile_item_repository_result::unchanged)
		return;
	throw failure{ static_cast<unsigned int>(
		status == flatfile_item_repository_result::not_found ? ENOENT :
		status == flatfile_item_repository_result::io_error  ? EIO :
								       EILSEQ) };
}
void checked(flatfile_item_accounting_status status)
{
	if (status == flatfile_item_accounting_status::ok)
		return;
	throw failure{ static_cast<unsigned int>(
		status == flatfile_item_accounting_status::already_exists ? EEXIST :
		status == flatfile_item_accounting_status::not_found	  ? ENOENT :
		status == flatfile_item_accounting_status::capacity	  ? ENOSPC :
		status == flatfile_item_accounting_status::io_error	  ? EIO :
									    EILSEQ) };
}
std::string canonical(const std::string &name)
{
	need(!name.empty() && name.size() <= CURRENCY_ACCOUNT_NAME_MAX_BYTES, EINVAL);
	std::string normalized;
	for (char letter : name)
	{
		if (letter >= 'A' && letter <= 'Z')
			letter = static_cast<char>(letter + ('a' - 'A'));
		need((letter >= 'a' && letter <= 'z') || (letter >= '0' && letter <= '9') ||
			     letter == '_' || letter == '-',
		     EINVAL);
		normalized += letter;
	}
	return normalized;
}
uint64_t number(std::span<const uint8_t> bytes, size_t offset)
{
	uint64_t value = 0;
	for (size_t index = 0; index < 8; ++index)
		value |= uint64_t(bytes[offset + index]) << (index * 8);
	return value;
}
struct identity
{
	economic_frozen_intent intent;
	coin_transfer_payload payload;
	std::array<currency_command_payload, 2> changes;
	std::array<economic_account_key, 2> accounts;
	std::array<std::string, 2> names;
};
economic_account_kind account_kind(const coin_transfer_endpoint &endpoint)
{
	if (endpoint.change.type == critical_command_type::account_bank)
		return economic_account_kind::wallet;
	need(endpoint.change.type == critical_command_type::item_transfer, EACCES);
	return economic_account_kind::pile;
}
uint64_t native_id(const coin_transfer_endpoint &endpoint)
{
	if (endpoint.change.type == critical_command_type::account_bank)
	{
		currency_command_payload change;
		need(currency_command_decode_payload(endpoint.change, &change), EINVAL);
		return change.pid;
	}
	item_transfer_payload change;
	need(item_transfer_command_decode_payload(endpoint.change, &change) &&
		     change.item_count == 1 && change.selected_item_uid == change.items[0].item_uid,
	     EINVAL);
	return change.selected_item_uid;
}
identity decode(const critical_command &root)
{
	need(root.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
		     root.type == critical_command_type::coin_transfer &&
		     critical_command_envelope_valid(root),
	     EINVAL);
	identity value;
	need(coin_transfer_command_decode_payload(root, &value.payload), EINVAL);
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		if (account_kind(endpoint) == economic_account_kind::wallet)
		{
			need(currency_command_decode_payload(endpoint.change,
							     &value.changes[index]) &&
				     value.changes[index].pid > 0 &&
				     value.changes[index].pid <= INT32_MAX &&
				     value.changes[index].racewar <= INT8_MAX &&
				     value.changes[index].reason ==
					     currency_reason_type::coin_transfer,
			     EINVAL);
			value.names[index] = canonical(value.changes[index].account_name.data());
		}
	}
	checked(economic_intent_decode(root.accounting_intent, &value.intent));
	const auto &facts = value.intent.admission.facts;
	need(facts.size() == ECONOMIC_COIN_TRANSFER_FACT_BYTES, EINVAL);
	const auto &metadata = value.intent.admission.metadata;
	for (size_t index = 0; index < 2; ++index)
	{
		value.accounts[index] = { metadata.lineage, account_kind(*endpoints[index]),
					  number(facts, index * 8), 0 };
		if (value.accounts[index].kind == economic_account_kind::pile)
			need(value.accounts[index].authority_id == native_id(*endpoints[index]),
			     EACCES);
	}
	need(value.accounts[0].kind != economic_account_kind::pile ||
		     value.accounts[1].kind != economic_account_kind::pile ||
		     value.accounts[0].authority_id != value.accounts[1].authority_id,
	     EOPNOTSUPP);
	auto admission = root;
	admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	admission.accounting_intent.clear();
	admission.publication_required = false;
	std::vector<uint8_t> expected;
	checked(coin_transfer_accounting_intent(admission, metadata.epoch, value.accounts[0],
						value.accounts[1], &expected));
	need(expected == root.accounting_intent, EACCES);
	return value;
}
economic_coin_vector vector(const std::array<int32_t, 4> &coins)
{
	return { coins[0], coins[1], coins[2], coins[3] };
}
uint64_t revision(const coin_transfer_result &result)
{
	return std::max({ result.wallets[0].wallet_revision, result.wallets[0].bank_revision,
			  result.wallets[1].wallet_revision, result.wallets[1].bank_revision,
			  result.piles[0].max_item_revision, result.piles[1].max_item_revision });
}
unsigned int rejection(const identity &value, const coin_transfer_result &before,
		       critical_failure_stage *failure_stage = nullptr)
{
	const bool shared_bank = value.accounts[0].kind == economic_account_kind::wallet &&
				 value.accounts[1].kind == economic_account_kind::wallet &&
				 value.names[0] == value.names[1] &&
				 value.changes[0].racewar == value.changes[1].racewar;
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		if (value.accounts[index].kind != economic_account_kind::wallet)
			continue;
		const auto &wallet = before.wallets[index];
		const auto &fences = endpoints[index]->change.expected_revisions;
		need(fences.size() == 2);
		if (fences[0].revision != wallet.wallet_revision ||
		    fences[1].revision != wallet.bank_revision)
		{
			if (failure_stage)
			{
				const auto wallet_stage =
					index ? critical_failure_stage::
							coin_destination_wallet_revision :
						critical_failure_stage::coin_source_wallet_revision;
				const auto bank_stage =
					index ? critical_failure_stage::
							coin_destination_bank_revision :
						critical_failure_stage::coin_source_bank_revision;
				*failure_stage = static_cast<critical_failure_stage>(
					(fences[0].revision != wallet.wallet_revision ?
						 static_cast<uint16_t>(wallet_stage) :
						 0) |
					(fences[1].revision != wallet.bank_revision ?
						 static_cast<uint16_t>(bank_stage) :
						 0));
			}
			return ESTALE;
		}
		const auto bank_revision = shared_bank && index ?
						   before.wallets[0].bank_revision + 1 :
						   wallet.bank_revision;
		if (wallet.wallet_revision == UINT64_MAX || bank_revision == UINT64_MAX)
			return ERANGE;
		if (wallet.wallet.amount != vector(endpoints[index]->before))
		{
			if (failure_stage)
				*failure_stage = critical_failure_stage::coin_revision_unknown;
			return ESTALE;
		}
	}
	return 0;
}
economic_accounting_plan plan(const critical_command &root, const identity &value,
			      const coin_transfer_result &result,
			      const std::array<uint64_t, 2> &pile_before_revisions)
{
	economic_accounting_plan candidate;
	checked(economic_intent_plan_metadata(root, value.intent, &candidate.metadata));
	critical_command destination_after_source;
	need(coin_transfer_command_destination_after_source(value.payload, result,
							    &destination_after_source));
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		const auto &endpoint = *endpoints[index];
		economic_account_effect effect;
		effect.key = value.accounts[index];
		effect.before = vector(endpoint.before);
		effect.after = vector(endpoint.after);
		if (effect.key.kind == economic_account_kind::wallet)
		{
			const auto &change = value.changes[index];
			const auto &wallet = result.wallets[index];
			const auto &fences = index ? destination_after_source.expected_revisions :
						     endpoint.change.expected_revisions;
			need(fences.size() == 2 &&
			     wallet.wallet_revision ==
				     endpoint.change.expected_revisions[0].revision + 1 &&
			     wallet.bank_revision == fences[1].revision + 1 &&
			     wallet.wallet.amount == effect.after);
			effect.before_revision = endpoint.change.expected_revisions[0].revision;
			effect.after_revision = wallet.wallet_revision;
			for (size_t part = 0; part < 4; ++part)
				need(change.bank_delta.amount[part] == 0 &&
				     change.wallet_delta.amount[part] ==
					     effect.after[part] - effect.before[part]);
		}
		else
		{
			item_transfer_payload pile;
			need(item_transfer_command_decode_payload(endpoint.change, &pile) &&
			     pile.item_count == 1);
			const auto &item = pile.items[0];
			const bool creation = pile.from_owner.type == item_owner_type::system;
			const auto &native = result.piles[index];
			need(native.item_count == 1 && native.root_item_uid == item.item_uid &&
			     native.max_item_revision > 0 &&
			     (creation ? native.max_item_revision == 1 :
					 item.expected_item_revision < UINT64_MAX &&
						 native.max_item_revision ==
							 item.expected_item_revision + 1));
			effect.before_revision = pile_before_revisions[index];
			effect.after_revision = native.max_item_revision;
			need(creation ?
				     effect.before_revision == 0 &&
					     effect.before == economic_coin_vector{} :
				     effect.before_revision > 0 &&
					     effect.before_revision <= item.expected_item_revision);
			uint64_t target_root = 0, target_parent = 0;
			need(item_transfer_target_topology(pile, item.item_uid, &target_root,
							   &target_parent));
			economic_item_position before = {};
			if (!creation)
			{
				before.owner = pile.from_owner;
				before.root_uid = item.root_item_uid;
				before.parent_uid = item.parent_item_uid;
				before.revision = item.expected_item_revision;
				before.state = item_custody_state::active;
			}
			economic_item_position after = {};
			after.owner = pile.to_owner;
			after.root_uid = target_root;
			after.parent_uid = target_parent;
			after.revision = native.max_item_revision;
			after.state = pile.to_owner.type == item_owner_type::destruction ?
					      item_custody_state::destroyed :
					      item_custody_state::active;
			candidate.items_before.push_back({ item.item_uid, before });
			candidate.items_after.push_back({ item.item_uid, after });
			candidate.item_events.push_back(
				{ static_cast<uint32_t>(candidate.item_events.size()), 0,
				  item.item_uid, before, after });
		}
		economic_coin_posting posting;
		posting.event_index = index;
		posting.account_index = index;
		// Flatfile stages both native wallet images under the parent receipt.
		// Its evidence store has no independently reserved child receipts.
		posting.child_index = 0;
		for (size_t part = 0; part < 4; ++part)
			posting.delta[part] = effect.after[part] - effect.before[part];
		checked(economic_coin_value(posting.delta, &posting.copper));
		candidate.accounts.push_back(effect);
		candidate.postings.push_back(posting);
	}
	checked(economic_plan_normalize(&candidate));
	return candidate;
}
void retained_identity(const std::string &root, const flatfile_authority_lock &lock,
		       const critical_command &command, const identity &value)
{
	flatfile_economic_control control;
	checked(flatfile_economic_control_read(root, lock, &control, nullptr));
	const size_t bucket = command.operation_id.bytes[0];
	need(control.lineage.bytes == value.accounts[0].lineage.bytes &&
	     (control.evidence_initialized[bucket / 8] & (1U << (bucket % 8))));
	flatfile_economic_epoch epoch;
	checked(flatfile_economic_epoch_read(root, lock, value.accounts[0].lineage,
					     value.intent.admission.metadata.epoch, &epoch,
					     nullptr));
	for (size_t index = 0; index < 2; ++index)
	{
		if (value.accounts[index].kind == economic_account_kind::pile)
			continue;
		flatfile_economic_mapping mapping;
		checked(flatfile_economic_mapping_read(root, lock, value.accounts[index], &mapping,
						       nullptr));
		need(mapping.locator.kind == 1 &&
		     mapping.locator.native_id == value.changes[index].pid);
	}
}
std::vector<economic_accounting_item_reference> item_references(const critical_command &command,
								const identity &value,
								const coin_transfer_result &result)
{
	std::vector<economic_accounting_item_reference> references;
	const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
						      &value.payload.destination };
	for (size_t index = 0; index < 2; ++index)
	{
		if (value.accounts[index].kind != economic_account_kind::pile)
			continue;
		item_transfer_payload transfer;
		need(item_transfer_command_decode_payload(endpoints[index]->change, &transfer) &&
		     transfer.item_count == 1);
		economic_accounting_item_reference reference = {};
		reference.operation_id = command.operation_id;
		reference.line_index = static_cast<uint16_t>(references.size());
		reference.event_index = reference.line_index;
		reference.child_index = static_cast<uint16_t>(index + 1);
		reference.item_uid = transfer.items[0].item_uid;
		reference.before_revision = transfer.from_owner.type == item_owner_type::system ?
						    0 :
						    transfer.items[0].expected_item_revision;
		reference.after_revision = result.piles[index].max_item_revision;
		reference.legacy_operation_id = endpoints[index]->change.operation_id;
		reference.legacy_event_index = 0;
		need(economic_accounting_item_reference_validate(reference));
		references.push_back(reference);
	}
	return references;
}
void verify(const std::string &root, const flatfile_authority_lock &lock,
	    const flatfile_accounting_record &record)
{
	need(record.failure_stage == critical_failure_stage::none);
	const auto value = decode(record.command);
	retained_identity(root, lock, record.command, value);
	coin_transfer_result result;
	need(coin_transfer_command_decode_result(value.payload, record.result.data(),
						 record.result.size(), &result));
	need(record.durable_revision == revision(result));
	if (record.result_code)
	{
		const bool piles = value.accounts[0].kind == economic_account_kind::pile ||
				   value.accounts[1].kind == economic_account_kind::pile;
		need(record.plan.empty() &&
		     (record.result_code == rejection(value, result) ||
		      (piles && (record.result_code == ESTALE || record.result_code == EEXIST ||
				 record.result_code == ENODATA || record.result_code == ENOENT ||
				 record.result_code == ERANGE || record.result_code == ENOSPC))));
		return;
	}
	economic_accounting_plan retained_plan;
	checked(economic_plan_decode(record.plan, &retained_plan));
	std::array<uint64_t, 2> pile_before_revisions = {};
	for (size_t index = 0; index < 2; ++index)
		if (value.accounts[index].kind == economic_account_kind::pile)
		{
			const auto found = std::find_if(
				retained_plan.accounts.begin(), retained_plan.accounts.end(),
				[&](const economic_account_effect &effect) {
					return economic_account_key_equal(effect.key,
									  value.accounts[index]);
				});
			need(found != retained_plan.accounts.end());
			pile_before_revisions[index] = found->before_revision;
		}
	const auto expected = plan(record.command, value, result, pile_before_revisions);
	std::vector<uint8_t> encoded;
	checked(economic_plan_encode(expected, &encoded));
	need(encoded == record.plan);
	for (const auto &effect : retained_plan.accounts)
	{
		if (effect.key.kind != economic_account_kind::pile)
			continue;
		flatfile_accounting_pile_state head;
		checked(flatfile_accounting_pile_state_read(root, lock, effect.key.authority_id,
							    &head, nullptr));
		need(economic_account_key_equal(head.account, effect.key) &&
		     head.item_revision >= effect.after_revision);
		if (head.item_revision == effect.after_revision)
			need(head.balance == effect.after &&
			     head.retired == (effect.after == economic_coin_vector{}) &&
			     head.operation_id.bytes == record.command.operation_id.bytes &&
			     head.epoch.bytes == value.intent.admission.metadata.epoch.bytes);
	}
	for (const auto &reference : item_references(record.command, value, result))
		checked(flatfile_item_accounting_reference_verify_operation(
			root, reference.legacy_operation_id,
			std::span<const economic_accounting_item_reference>(&reference, 1),
			nullptr));
}
critical_apply_result completion(const flatfile_accounting_record &record, bool replay)
{
	need(record.result.size() <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
	critical_apply_result result = { record.result_code ?
						 critical_apply_outcome::terminal_failure :
					 replay ? critical_apply_outcome::already_applied :
						  critical_apply_outcome::applied,
					 record.durable_revision, record.result_code };
	if (record.result_code)
	{
		// Keep the existing full authority record for exact replay verification,
		// but expose only the classified stale currency domain to publication.
		if (record.result_code == ESTALE)
		{
			const auto value = decode(record.command);
			coin_transfer_result authority;
			need(coin_transfer_command_decode_result(value.payload,
								 record.result.data(),
								 record.result.size(), &authority));
			std::array<uint8_t, COIN_TRANSFER_STALE_RESULT_BYTES> bytes;
			coin_transfer_stale_result stale;
			if (rejection(value, authority, &result.failure_stage) == ESTALE &&
			    coin_transfer_command_encode_stale_result(
				    value.payload, authority, result.failure_stage, &bytes) &&
			    coin_transfer_command_decode_stale_result(
				    value.payload, result.failure_stage, bytes.data(), bytes.size(),
				    &stale))
			{
				result.result_size = bytes.size();
				std::copy(bytes.begin(), bytes.end(),
					  result.result_payload.begin());
			}
			if (result.failure_stage == critical_failure_stage::none)
				result.failure_stage =
					critical_failure_stage::coin_revision_unknown;
		}
		return result;
	}
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
	return result;
}
}

critical_apply_result flatfile_accounting_coin_transaction::verify_retained_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &original) noexcept
{
	try
	{
		need(!root.empty() && lock.matches(root), EINVAL);
		// Validate the caller's original typed identity before recovering lookup.
		// The lookup compares its canonical bytes to the immutable retained root.
		(void)decode(original);
		flatfile_accounting_record retained;
		const auto lookup =
			flatfile_accounting_lookup(root, lock, original, &retained, nullptr);
		if (lookup == flatfile_accounting_status::not_found)
			return { critical_apply_outcome::retryable_failure, 0, EAGAIN };
		checked(lookup);
		verify(root, lock, retained);
		if (!retained.result_code)
			checked_retained_claim(flatfile_accounting_storage::verify_source_claim(
				root, lock, retained, nullptr));
		return completion(retained, true);
	}
	catch (const failure &error)
	{
		// This is unavailable proof, not an authenticated business rejection.
		return { critical_apply_outcome::retryable_failure, 0, error.code };
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, 0, ENOMEM };
	}
	catch (...)
	{
		return { critical_apply_outcome::retryable_failure, 0, EFAULT };
	}
}

unsigned int flatfile_accounting_coin_transaction::read_room_pile_locked(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t uid,
	flatfile_room_coin_pile *output, std::string *error) noexcept
{
	try
	{
		need(!root.empty() && lock.matches(root) && output && uid && uid != UINT64_MAX,
		     EINVAL);
		flatfile_accounting_pile_state head;
		const auto head_status =
			flatfile_accounting_pile_state_read(root, lock, uid, &head, error);
		need(head_status != flatfile_accounting_status::not_found, ENOENT);
		checked(head_status);
		need(!head.retired && head.account.kind == economic_account_kind::pile &&
			     head.account.authority_id == uid && !head.account.context_id,
		     ESTALE);

		// The existing current head selects the original immutable indexed record;
		// no player journal envelope is needed or constructed after ACK retirement.
		flatfile_accounting_record record;
		const auto found = flatfile_accounting_storage::lookup_retained_locked(
			root, lock, head.operation_id, &record, error);
		need(found != flatfile_accounting_status::not_found, ENOENT);
		checked(found);
		const auto value = decode(record.command);
		need(record.command.publication_required &&
			     coin_transfer_accounting_command_supported(record.command) &&
			     !record.result_code &&
			     record.failure_stage == critical_failure_stage::none,
		     EILSEQ);
		verify(root, lock, record);
		checked_retained_claim(flatfile_accounting_storage::verify_source_claim(
			root, lock, record, error));
		// Full native catalog receipt and command digest are independent of the
		// evidence record. Neither path manufactures or applies a successful root.
		checked(flatfile_item_repository_verify_coin_root_locked(root, lock, record.command,
									 record.result, error));

		const bool drop = value.accounts[0].kind == economic_account_kind::wallet &&
				  value.accounts[1].kind == economic_account_kind::pile;
		const bool pickup = value.accounts[0].kind == economic_account_kind::pile &&
				    value.accounts[1].kind == economic_account_kind::wallet;
		need(drop || pickup, EOPNOTSUPP);
		const size_t pile_index = drop ? 1 : 0;
		const auto &endpoint = drop ? value.payload.destination : value.payload.source;
		need(economic_account_key_equal(head.account, value.accounts[pile_index]) &&
			     head.epoch.bytes == value.intent.admission.metadata.epoch.bytes &&
			     head.operation_id.bytes == record.command.operation_id.bytes,
		     ESTALE);
		item_transfer_payload pile;
		need(item_transfer_command_decode_payload(endpoint.change, &pile) &&
			     pile.item_count == 1 && !pile.multi_root &&
			     pile.selected_item_uid == uid && pile.items[0].item_uid == uid &&
			     pile.items[0].root_item_uid == uid && !pile.items[0].parent_item_uid &&
			     pile.target_root_item_uid == uid && !pile.target_parent_item_uid &&
			     !pile.expected_target_parent_revision && !pile.from_owner.context_id &&
			     !pile.to_owner.context_id &&
			     pile.continuation.kind == item_transfer_continuation_kind::none &&
			     pile.continuation.data.empty(),
		     EOPNOTSUPP);
		const auto room = drop ? pile.to_owner : pile.from_owner;
		need(room.type == item_owner_type::room && room.id && room.id <= INT_MAX &&
			     !room.context_id && item_owner_identity_equal(pile.to_owner, room),
		     EOPNOTSUPP);
		if (drop)
			need(pile.from_owner.type == item_owner_type::system &&
				     !pile.from_owner.id &&
				     pile.reason == item_transfer_reason::creation &&
				     pile.items[0].expected_item_revision ==
					     ITEM_TRANSFER_ABSENT_REVISION &&
				     pile.items[0].expected_state == item_custody_state::absent,
			     EOPNOTSUPP);
		else
			need(pile.reason == item_transfer_reason::player_put &&
				     pile.items[0].expected_state == item_custody_state::active &&
				     pile.items[0].expected_item_revision &&
				     pile.items[0].expected_item_revision != UINT64_MAX,
			     EOPNOTSUPP);

		// Current lineage and selected epoch only. Historical wallet identities and
		// their receipt values do not have to remain current after this transfer.
		flatfile_economic_authority_snapshot authority;
		checked(economic_flatfile_lock_authority(root, lock, head.account.lineage,
							 head.epoch, {}, &authority, error));
		economic_accounting_plan retained_plan;
		checked(economic_plan_decode(record.plan, &retained_plan));
		need(retained_plan.accounts.size() == 2 && retained_plan.postings.size() == 2 &&
		     retained_plan.children.empty() && retained_plan.item_events.size() == 1 &&
		     retained_plan.items_before.size() == 1 &&
		     retained_plan.items_after.size() == 1);
		const auto effect = std::find_if(
			retained_plan.accounts.begin(), retained_plan.accounts.end(),
			[&](const economic_account_effect &entry)
			{ return economic_account_key_equal(entry.key, head.account); });
		need(effect != retained_plan.accounts.end() &&
		     effect->after_revision == head.item_revision && effect->after == head.balance);
		const auto &event = retained_plan.item_events[0];
		need(event.uid == uid && !event.child_index && event.after.root_uid == uid &&
		     !event.after.parent_uid && !event.after.equipment_slot &&
		     event.after.state == item_custody_state::active &&
		     event.after.revision == head.item_revision &&
		     item_owner_identity_equal(event.after.owner, room));

		coin_transfer_result result;
		need(coin_transfer_command_decode_result(value.payload, record.result.data(),
							 record.result.size(), &result));
		const auto &receipt = result.piles[pile_index];
		need(pile.expected_from_revision != UINT64_MAX &&
		     pile.expected_to_revision != UINT64_MAX && receipt.item_count == 1 &&
		     receipt.root_item_uid == uid &&
		     receipt.max_item_revision == head.item_revision &&
		     receipt.from_owner_revision == pile.expected_from_revision + 1 &&
		     receipt.to_owner_revision == pile.expected_to_revision + 1 &&
		     !receipt.corpse_revision && !receipt.collector_catalog_changed);

		// Complete current catalog rejects every ambiguous UID/root/descendant.
		std::vector<flatfile_item_ownership_record> catalog;
		checked(flatfile_item_repository_recovery_catalog_locked(root, lock, &catalog,
									 error));
		const flatfile_item_ownership_record *selected = nullptr;
		for (const auto &entry : catalog)
			if (entry.item_uid == uid || entry.root_item_uid == uid ||
			    entry.parent_item_uid == uid)
			{
				need(!selected && entry.item_uid == uid &&
				     entry.root_item_uid == uid && !entry.parent_item_uid &&
				     !entry.equipment_slot &&
				     entry.state == item_custody_state::active &&
				     entry.item_revision == head.item_revision &&
				     entry.vnum == pile.items[0].vnum &&
				     item_owner_identity_equal(entry.owner, room));
				selected = &entry;
			}
		need(selected && !selected->coin_payload.empty(), ENODATA);
		std::vector<flatfile_item_ownership_record> owned;
		uint64_t owner_revision = 0;
		checked(flatfile_item_repository_load_owner_locked(root, lock, room,
								   &owner_revision, &owned, error));
		need(owner_revision && owner_revision >= receipt.to_owner_revision);
		flatfile_coin_pile_source native;
		checked(flatfile_item_repository_read_coin_pile_locked(root, lock, uid, &native,
								       error));
		const auto &literal = native.item;
		need(literal.object_uid == uid && literal.vnum == selected->vnum &&
		     literal.type == ITEM_MONEY &&
		     literal.parent_index == PLAYER_SNAPSHOT_NO_PARENT &&
		     literal.equipment_slot == -1 &&
		     literal.string_mask ==
			     (STRUNG_KEYS | STRUNG_DESC1 | STRUNG_DESC2 | STRUNG_DESC3) &&
		     literal.dynamic_affects.empty() && literal.extra_descriptions.size() <= 1 &&
		     !(literal.extra_flags &
		       (ITEM_LIT | ITEM_TRANSIENT | ITEM_ARTIFACT | ITEM_PROCLIB)));
		bool nonempty = false;
		for (size_t index = 0; index < 4; ++index)
		{
			need(literal.values[index] >= 0 &&
			     literal.values[index] == head.balance[index] &&
			     literal.values[index] == endpoint.after[index]);
			nonempty = nonempty || literal.values[index] != 0;
		}
		need(nonempty);
		std::vector<uint8_t> canonical;
		need(player_item_snapshot_list_encode({ literal }, &canonical) ==
			     player_snapshot_codec_result::ok &&
		     canonical == selected->coin_payload &&
		     canonical.size() == pile.item_blob_size &&
		     std::equal(canonical.begin(), canonical.end(), pile.item_blob.begin()));
		flatfile_room_coin_pile candidate;
		candidate.lineage = authority.lineage;
		candidate.epoch = authority.epoch;
		candidate.lineage_revision = authority.lineage_revision;
		candidate.root_operation = record.command.operation_id;
		candidate.pile_endpoint_operation = endpoint.change.operation_id;
		candidate.retained_root_revision = record.durable_revision;
		candidate.retained_pile_result = receipt;
		candidate.identity = { uid,
				       uid,
				       0,
				       room,
				       head.item_revision,
				       owner_revision,
				       selected->vnum,
				       item_custody_state::active };
		candidate.item = std::move(native.item);
		*output = std::move(candidate);
		return 0;
	}
	catch (const failure &failure)
	{
		return failure.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EFAULT;
	}
}

critical_apply_result flatfile_accounting_coin_transaction::apply(const std::string &root,
								  const critical_command &command)
{
	bool publishing = false;
	try
	{
		need(!root.empty(), EINVAL);
		flatfile_identity_lock identity_lock;
		flatfile_authority_lock lock;
		need(identity_lock.acquire(root, nullptr) && lock.acquire(root, nullptr), EIO);
		checked(flatfile_player_domain_recover_locked(root, lock, nullptr));
		flatfile_accounting_record retained;
		const auto lookup =
			flatfile_accounting_lookup(root, lock, command, &retained, nullptr);
		if (lookup == flatfile_accounting_status::ok)
		{
			verify(root, lock, retained);
			if (!retained.result_code)
				checked_retained_claim(
					flatfile_accounting_storage::verify_source_claim(
						root, lock, retained, nullptr));
			return completion(retained, true);
		}
		if (lookup != flatfile_accounting_status::not_found)
			checked(lookup);
		const auto value = decode(command);
		retained_identity(root, lock, command, value);
		std::vector<flatfile_economic_mapping_request> requests;
		for (size_t index = 0; index < 2; ++index)
			if (value.accounts[index].kind == economic_account_kind::wallet)
				requests.push_back({ value.accounts[index],
						     { 1, value.changes[index].pid, {} } });
		flatfile_economic_authority_snapshot snapshot;
		checked(economic_flatfile_lock_authority(root, lock, value.accounts[0].lineage,
							 value.intent.admission.metadata.epoch,
							 requests, &snapshot, nullptr));
		for (size_t index = 0; index < 2; ++index)
		{
			if (value.accounts[index].kind != economic_account_kind::wallet)
				continue;
			const auto &change = value.changes[index];
			std::optional<flatfile_legacy_domain_receipt> legacy;
			checked(flatfile_player_domain_legacy_receipt_locked(
				root, lock, change.pid, command.operation_id, &legacy, nullptr));
			need(!legacy, EEXIST);
			flatfile_identity_record native_identity;
			const auto status = flatfile_identity_lookup_pid_locked(
				root, identity_lock, lock, change.pid, &native_identity, nullptr);
			need(status == flatfile_identity_result::ok,
			     status == flatfile_identity_result::io_error ? EIO : ENOENT);
			need(native_identity.active &&
				     canonical(native_identity.account) == value.names[index] &&
				     native_identity.racewar == change.racewar,
			     EACCES);
		}
		coin_transfer_result opening;
		const coin_transfer_endpoint *endpoints[] = { &value.payload.source,
							      &value.payload.destination };
		for (size_t index = 0; index < 2; ++index)
		{
			if (value.accounts[index].kind == economic_account_kind::pile)
			{
				item_transfer_payload transfer;
				need(item_transfer_command_decode_payload(endpoints[index]->change,
									  &transfer));
				// A refusal still needs an encodable two-endpoint completion.
				opening.piles[index].root_item_uid =
					item_transfer_result_root(transfer);
				opening.piles[index].item_count = transfer.item_count;
				continue;
			}
			if (value.accounts[index].kind != economic_account_kind::wallet)
				continue;
			flatfile_player_domain_record native;
			const auto &change = value.changes[index];
			checked(flatfile_player_domain_load_locked(
				root, lock, change.pid, value.names[index], change.racewar, &native,
				nullptr));
			auto &wallet = opening.wallets[index];
			wallet.wallet_revision = native.domains.wallet_revision;
			wallet.bank_revision = native.domains.bank_revision;
			for (size_t part = 0; part < 4; ++part)
			{
				need(native.domains.wallet[part] <= INT_MAX &&
				     native.domains.bank[part] <= INT_MAX);
				wallet.wallet.amount[part] = native.domains.wallet[part];
				wallet.bank.amount[part] = native.domains.bank[part];
			}
		}
		coin_transfer_result result = opening;
		std::vector<flatfile_authority_after_image> images;
		unsigned int result_code = 0;
		checked(flatfile_player_domain_prepare_coin_wallets(
			root, lock, value.payload, &result, &images, &result_code, nullptr));
		const unsigned int wallet_result_code = result_code;
		std::array<uint64_t, 2> pile_before_revisions = {};
		for (size_t index = 0; !result_code && index < 2; ++index)
		{
			if (value.accounts[index].kind != economic_account_kind::pile)
				continue;
			item_transfer_payload transfer;
			need(item_transfer_command_decode_payload(endpoints[index]->change,
								  &transfer) &&
			     transfer.item_count == 1);
			flatfile_accounting_pile_state prior;
			const auto status = flatfile_accounting_pile_state_read(
				root, lock, value.accounts[index].authority_id, &prior, nullptr);
			if (transfer.from_owner.type == item_owner_type::system)
			{
				if (status == flatfile_accounting_status::ok)
					result_code = EEXIST;
				else if (status != flatfile_accounting_status::not_found)
					checked(status);
			}
			else if (status == flatfile_accounting_status::not_found)
				result_code = ENODATA;
			else
			{
				checked(status);
				opening.piles[index].max_item_revision = prior.item_revision;
				result.piles[index].max_item_revision = prior.item_revision;
				if (!economic_account_key_equal(prior.account,
								value.accounts[index]) ||
				    prior.retired)
					result_code = ESTALE;
				else if (prior.item_revision >
						 transfer.items[0].expected_item_revision ||
					 prior.balance != vector(endpoints[index]->before))
					result_code = ESTALE;
				else
					pile_before_revisions[index] = prior.item_revision;
			}
		}
		if (!result_code && (value.accounts[0].kind == economic_account_kind::pile ||
				     value.accounts[1].kind == economic_account_kind::pile))
		{
			checked(flatfile_item_repository_prepare_coin_piles(
				root, lock, command, &result, &images, &result_code, nullptr));
		}
		flatfile_accounting_record record;
		record.command = command;
		record.result_code = result_code;
		if (result_code)
		{
			need(wallet_result_code ?
				     result_code == rejection(value, opening) :
				     result_code == ESTALE || result_code == EEXIST ||
					     result_code == ENODATA || result_code == ENOENT ||
					     result_code == ERANGE || result_code == ENOSPC);
			result = opening;
			images.clear();
		}
		else
		{
			const auto expected = plan(command, value, result, pile_before_revisions);
			checked(economic_plan_encode(expected, &record.plan));
		}
		std::array<uint8_t, COIN_TRANSFER_RESULT_BYTES> encoded;
		need(coin_transfer_command_encode_result(value.payload, result, &encoded));
		record.result.assign(encoded.begin(), encoded.end());
		record.durable_revision = revision(result);
		std::vector<flatfile_authority_operation> operations;
		for (auto &image : images)
			operations.push_back({ flatfile_authority_store::domains,
					       flatfile_authority_operation_kind::write,
					       std::move(image.filename), std::move(image.bytes) });
		const size_t domain_images = operations.size();
		if (!record.result_code)
			for (const auto &reference : item_references(command, value, result))
				checked(flatfile_item_accounting_reference_stage(
					root, lock, reference.legacy_operation_id,
					std::span<const economic_accounting_item_reference>(
						&reference, 1),
					&operations, nullptr));
		checked(flatfile_accounting_storage::stage(root, lock, record, &operations,
							   nullptr));
		// Claim the original lifecycle source in the same authority transaction.
		checked(flatfile_accounting_storage::stage_source_claim(root, lock, record,
									&operations, nullptr));
		if (!record.result_code)
		{
			economic_accounting_plan expected;
			checked(economic_plan_decode(record.plan, &expected));
			for (const auto &effect : expected.accounts)
			{
				if (effect.key.kind != economic_account_kind::pile)
					continue;
				const size_t index =
					economic_account_key_equal(effect.key, value.accounts[0]) ?
						0 :
						1;
				item_transfer_payload transfer;
				need(item_transfer_command_decode_payload(endpoints[index]->change,
									  &transfer));
				checked(flatfile_accounting_pile_state_stage(
					root, lock, effect, value.intent.admission.metadata.epoch,
					command.operation_id,
					transfer.to_owner.type == item_owner_type::destruction,
					&operations, nullptr));
			}
		}
		publishing = true;
		need(flatfile_accounting_storage::commit(root, lock, operations, nullptr) ==
			     flatfile_authority_transaction_result::ok,
		     EIO);
		checked(flatfile_accounting_lookup(root, lock, command, &retained, nullptr));
		verify(root, lock, retained);
		if (!retained.result_code)
			checked_retained_claim(flatfile_accounting_storage::verify_source_claim(
				root, lock, retained, nullptr));
		need(retained.plan == record.plan && retained.result == record.result &&
		     retained.result_code == record.result_code &&
		     retained.durable_revision == record.durable_revision);
		for (size_t index = 0; index < domain_images; ++index)
		{
			std::vector<uint8_t> bytes;
			need(flatfile_read(root + "/domains", operations[index].filename, 64 * 1024,
					   &bytes, nullptr) == flatfile_read_result::ok,
			     EIO);
			need(bytes == operations[index].bytes);
		}
		for (size_t index = 0; index < 2; ++index)
		{
			if (value.accounts[index].kind != economic_account_kind::wallet)
				continue;
			flatfile_player_domain_record native;
			const auto &change = value.changes[index];
			checked(flatfile_player_domain_load_locked(
				root, lock, change.pid, value.names[index], change.racewar, &native,
				nullptr));
			const bool shared_bank =
				value.accounts[0].kind == economic_account_kind::wallet &&
				value.accounts[1].kind == economic_account_kind::wallet &&
				value.names[0] == value.names[1] &&
				value.changes[0].racewar == value.changes[1].racewar;
			const auto bank_revision = !record.result_code && shared_bank ?
							   result.wallets[1].bank_revision :
							   result.wallets[index].bank_revision;
			need(native.domains.wallet_revision ==
				     result.wallets[index].wallet_revision &&
			     native.domains.bank_revision == bank_revision);
			for (size_t part = 0; part < 4; ++part)
				need(native.domains.wallet[part] ==
					     static_cast<uint64_t>(
						     result.wallets[index].wallet.amount[part]) &&
				     native.domains.bank[part] ==
					     static_cast<uint64_t>(
						     result.wallets[index].bank.amount[part]));
		}
		return completion(retained, false);
	}
	catch (const failure &error)
	{
		return { publishing	      ? critical_apply_outcome::ambiguous_commit :
			 error.code == EEXIST ? critical_apply_outcome::terminal_failure :
						critical_apply_outcome::retryable_failure,
			 0, error.code };
	}
	catch (const std::bad_alloc &)
	{
		return { publishing ? critical_apply_outcome::ambiguous_commit :
				      critical_apply_outcome::retryable_failure,
			 0, ENOMEM };
	}
}
