#include "flatfile/flatfile_accounting_bank_transaction.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_identity_repository.h"
#include "flatfile/flatfile_player_domain_repository.h"
#include "flatfile/currency_flatfile_mutation_writer.h"
#include "flatfile/flatfile_store.h"
#include "economy/economic_currency_adapter.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <new>

namespace
{
struct failure
{
	unsigned int code;
};
void need(bool value, unsigned int code = EILSEQ)
{
	if (!value)
		throw failure{ code };
}
void checked(unsigned int code)
{
	if (code)
		throw failure{ code };
}
void checked(economic_accounting_error value)
{
	need(value == economic_accounting_error::ok,
	     value == economic_accounting_error::capacity ? ENOMEM : EILSEQ);
}
void checked(flatfile_accounting_status value, bool read_only = false)
{
	if (value == flatfile_accounting_status::ok)
		return;
	if (value == flatfile_accounting_status::conflict ||
	    value == flatfile_accounting_status::already_exists)
		throw failure{ EEXIST };
	if (value == flatfile_accounting_status::capacity)
		throw failure{ static_cast<unsigned int>(read_only ? ENOMEM : ENOSPC) };
	if (value == flatfile_accounting_status::io_error)
		throw failure{ EIO };
	throw failure{ EILSEQ };
}
void checked(flatfile_player_domain_result value)
{
	if (value == flatfile_player_domain_result::ok)
		return;
	if (value == flatfile_player_domain_result::not_found)
		throw failure{ ENOENT };
	if (value == flatfile_player_domain_result::conflict)
		throw failure{ EACCES };
	if (value == flatfile_player_domain_result::io_error)
		throw failure{ EIO };
	throw failure{ EILSEQ };
}
std::string canonical(const std::string &name)
{
	need(!name.empty() && name.size() <= CURRENCY_ACCOUNT_NAME_MAX_BYTES, EINVAL);
	std::string result;
	for (auto c : name)
	{
		if (c >= 'A' && c <= 'Z')
			c = static_cast<char>(c + ('a' - 'A'));
		need((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-',
		     EINVAL);
		result += c;
	}
	return result;
}
uint64_t number(std::span<const uint8_t> input, size_t offset)
{
	uint64_t value = 0;
	for (size_t byte = 0; byte < 8; ++byte)
		value |= uint64_t(input[offset + byte]) << (8 * byte);
	return value;
}
struct bank_identity
{
	economic_frozen_intent intent;
	currency_command_payload payload = {};
	economic_account_key wallet, bank;
};
bank_identity decode(const critical_command &command)
{
	bank_identity value;
	need(currency_command_decode_payload(command, &value.payload), EINVAL);
	need(value.payload.pid <= INT32_MAX && value.payload.racewar <= INT8_MAX, EINVAL);
	checked(economic_intent_decode(command.accounting_intent, &value.intent));
	const auto &facts = value.intent.admission.facts;
	need(facts.size() == ECONOMIC_BANK_FACT_BYTES, EINVAL);
	const auto &meta = value.intent.admission.metadata;
	value.wallet = { meta.lineage, economic_account_kind::wallet, number(facts, 0), 0 };
	value.bank = { meta.lineage, economic_account_kind::bank, number(facts, 8),
		       number(facts, 16) };
	auto admission = command;
	admission.schema_version = CRITICAL_COMMAND_SCHEMA_VERSION;
	admission.accounting_intent.clear();
	std::vector<uint8_t> expected;
	checked(economic_bank_transfer_intent(admission, meta.epoch, value.wallet, value.bank,
					      &expected));
	need(expected == command.accounting_intent, EACCES);
	return value;
}
bool equal(const currency_command_result &a, const currency_command_result &b)
{
	return a.wallet.amount == b.wallet.amount && a.bank.amount == b.bank.amount &&
	       a.wallet_revision == b.wallet_revision && a.bank_revision == b.bank_revision;
}
currency_command_result balances(const flatfile_player_domain_record &record)
{
	currency_command_result value;
	for (size_t part = 0; part < 4; ++part)
	{
		need(record.domains.wallet[part] <= INT_MAX &&
		     record.domains.bank[part] <= INT_MAX);
		value.wallet.amount[part] = record.domains.wallet[part];
		value.bank.amount[part] = record.domains.bank[part];
	}
	value.wallet_revision = record.domains.wallet_revision;
	value.bank_revision = record.domains.bank_revision;
	return value;
}
bool business_error(unsigned int code)
{
	return code == ESTALE || code == ENOSPC || code == ERANGE;
}
void retained_identity(const std::string &root, const flatfile_authority_lock &lock,
		       const critical_command &command, const bank_identity &identity)
{
	flatfile_economic_control control;
	checked(flatfile_economic_control_read(root, lock, &control, nullptr));
	const size_t bucket = command.operation_id.bytes[0];
	need(control.lineage.bytes == identity.wallet.lineage.bytes &&
	     (control.evidence_initialized[bucket / 8] & (1U << (bucket % 8))));
	flatfile_economic_epoch epoch;
	checked(flatfile_economic_epoch_read(root, lock, identity.wallet.lineage,
					     identity.intent.admission.metadata.epoch, &epoch,
					     nullptr));
	flatfile_economic_mapping wallet, bank;
	checked(flatfile_economic_mapping_read(root, lock, identity.wallet, &wallet, nullptr));
	checked(flatfile_economic_mapping_read(root, lock, identity.bank, &bank, nullptr));
	need(wallet.locator.kind == 1 && wallet.locator.native_id == identity.payload.pid &&
	     bank.locator.kind == 2 && bank.locator.native_id == identity.bank.authority_id);
}
void verify(const std::string &root, const flatfile_authority_lock &lock,
	    const flatfile_accounting_record &record)
{
	need(record.failure_stage == critical_failure_stage::none);
	const auto identity = decode(record.command);
	retained_identity(root, lock, record.command, identity);
	currency_command_result result;
	need(currency_command_decode_result(record.result.data(), record.result.size(), &result));
	need(record.durable_revision == std::max(result.wallet_revision, result.bank_revision));
	if (record.result_code)
	{
		need(record.plan.empty() && business_error(record.result_code));
		std::optional<currency_prepared_mutation> mutation;
		need(currency_prepare_mutation(identity.payload, result,
					       record.command.expected_revisions[0].revision,
					       record.command.expected_revisions[1].revision,
					       currency_revision_policy::flatfile_legacy,
					       &mutation) == record.result_code);
		return;
	}
	economic_accounting_plan plan;
	checked(economic_plan_decode(record.plan, &plan));
	need(plan.accounts.size() == 2);
	economic_currency_authority authority = { identity.intent.admission.metadata.epoch,
						  identity.wallet,
						  identity.bank,
						  record.command.keys[0],
						  record.command.keys[1],
						  {} };
	bool wallet = false, bank = false;
	for (const auto &account : plan.accounts)
	{
		if (economic_account_key_equal(account.key, identity.wallet))
		{
			need(!wallet);
			wallet = true;
			authority.state.wallet.amount = account.before;
			authority.state.wallet_revision = account.before_revision;
		}
		else if (economic_account_key_equal(account.key, identity.bank))
		{
			need(!bank);
			bank = true;
			authority.state.bank.amount = account.before;
			authority.state.bank_revision = account.before_revision;
		}
		else
			need(false);
	}
	need(wallet && bank);
	std::optional<economic_prepared_currency> prepared;
	checked(economic_bank_transfer_prepare(record.command, identity.intent, authority,
					       currency_revision_policy::flatfile_legacy,
					       &prepared));
	checked(prepared->agrees_with(plan));
	need(equal(prepared->mutation().after(), result));
}
critical_apply_result completion(const flatfile_accounting_record &record, bool replay)
{
	need(record.result.size() <= CRITICAL_COMPLETION_RESULT_MAX_BYTES);
	critical_apply_result result = { record.result_code ?
						 critical_apply_outcome::terminal_failure :
					 replay ? critical_apply_outcome::already_applied :
						  critical_apply_outcome::applied,
					 record.durable_revision, record.result_code };
	result.failure_stage = record.failure_stage;
	result.result_size = record.result.size();
	std::copy(record.result.begin(), record.result.end(), result.result_payload.begin());
	return result;
}
}

critical_apply_result flatfile_accounting_bank_transaction::apply(const std::string &root,
								  const critical_command &command)
{
	bool publishing = false;
	try
	{
		need(!root.empty() &&
			     command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			     command.type == critical_command_type::account_bank &&
			     critical_command_envelope_valid(command),
		     EINVAL);
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
			return completion(retained, true);
		}
		if (lookup != flatfile_accounting_status::not_found)
			checked(lookup, true);
		const auto identity = decode(command);
		// A legacy receipt can never prove this schema-2 mutation was accounted.
		std::optional<flatfile_legacy_domain_receipt> legacy;
		checked(flatfile_player_domain_legacy_receipt_locked(
			root, lock, identity.payload.pid, command.operation_id, &legacy, nullptr));
		need(!legacy, EEXIST);
		retained_identity(root, lock, command, identity);
		flatfile_identity_record native_identity;
		const auto identity_status = flatfile_identity_lookup_pid_locked(
			root, identity_lock, lock, identity.payload.pid, &native_identity, nullptr);
		need(identity_status == flatfile_identity_result::ok,
		     identity_status == flatfile_identity_result::io_error  ? EIO :
		     identity_status == flatfile_identity_result::not_found ? ENOENT :
									      EILSEQ);
		const auto account = canonical(identity.payload.account_name.data());
		need(native_identity.active && canonical(native_identity.account) == account &&
			     native_identity.racewar == identity.payload.racewar,
		     EACCES);
		const flatfile_economic_mapping_request requests[] = {
			{ identity.wallet, { 1, identity.payload.pid, {} } },
			{ identity.bank, { 2, identity.bank.authority_id, account } }
		};
		flatfile_economic_authority_snapshot snapshot;
		checked(economic_flatfile_lock_authority(root, lock, identity.wallet.lineage,
							 identity.intent.admission.metadata.epoch,
							 requests, &snapshot, nullptr));
		flatfile_player_domain_record native;
		checked(flatfile_player_domain_load_locked(root, lock, identity.payload.pid,
							   account, identity.payload.racewar,
							   &native, nullptr));
		economic_currency_authority authority = { identity.intent.admission.metadata.epoch,
							  identity.wallet,
							  identity.bank,
							  command.keys[0],
							  command.keys[1],
							  balances(native) };
		std::optional<economic_prepared_currency> prepared;
		const auto preparation = economic_bank_transfer_prepare(
			command, identity.intent, authority,
			currency_revision_policy::flatfile_legacy, &prepared);
		currency_command_result result = authority.state;
		flatfile_accounting_record record;
		record.command = command;
		std::vector<flatfile_authority_operation> operations;
		if (preparation == economic_accounting_error::ok)
		{
			result = prepared->mutation().after();
			checked(economic_plan_encode(prepared->plan(), &record.plan));
			checked(currency_flatfile_mutation_writer::stage(
				root, lock, command, prepared->mutation(), &operations, nullptr));
		}
		else
		{
			need(preparation != economic_accounting_error::capacity, ENOMEM);
			std::optional<currency_prepared_mutation> rejected;
			record.result_code = currency_prepare_mutation(
				identity.payload, authority.state,
				command.expected_revisions[0].revision,
				command.expected_revisions[1].revision,
				currency_revision_policy::flatfile_legacy, &rejected);
			need(business_error(record.result_code));
		}
		record.durable_revision = std::max(result.wallet_revision, result.bank_revision);
		std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> encoded;
		need(currency_command_encode_result(result, &encoded));
		record.result.assign(encoded.begin(), encoded.end());
		const auto domain_images = operations.size();
		checked(flatfile_accounting_storage::stage(root, lock, record, &operations,
							   nullptr));
		publishing = true;
		need(flatfile_accounting_storage::commit(root, lock, operations, nullptr) ==
			     flatfile_authority_transaction_result::ok,
		     EIO);
		checked(flatfile_accounting_lookup(root, lock, command, &retained, nullptr), true);
		verify(root, lock, retained);
		need(retained.plan == record.plan && retained.result == record.result &&
		     retained.result_code == record.result_code &&
		     retained.failure_stage == record.failure_stage &&
		     retained.durable_revision == record.durable_revision);
		for (size_t index = 0; index < domain_images; ++index)
		{
			std::vector<uint8_t> bytes;
			need(flatfile_read(root + "/domains", operations[index].filename, 64 * 1024,
					   &bytes, nullptr) == flatfile_read_result::ok,
			     EIO);
			need(bytes == operations[index].bytes);
		}
		flatfile_player_domain_record after;
		checked(flatfile_player_domain_load_locked(root, lock, identity.payload.pid,
							   account, identity.payload.racewar,
							   &after, nullptr));
		need(equal(balances(after), result));
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
