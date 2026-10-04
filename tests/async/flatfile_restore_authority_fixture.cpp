// Synthetic, private fixture encoded by the native metadata writers. Never
// selects an active epoch or runs a production lifecycle/activation owner.
#include "flatfile/flatfile_accounting_authority.h"
#include "economy/economic_currency_adapter.h"
#include <cassert>
#include <cstring>
#include <filesystem>

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto initialize_native_bucket =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto create_mapping =
		&flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto retire_mapping =
		&flatfile_accounting_authority_storage::retire_mapping;
	static constexpr auto rename_bank = &flatfile_accounting_authority_storage::rename_bank;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto initialize_evidence_bucket =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto stage = &flatfile_accounting_storage::stage;
	static auto commit(const std::string &root, const flatfile_authority_lock &lock,
			   const std::vector<flatfile_authority_operation> &operations,
			   std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};
static critical_operation_id id(uint64_t value)
{
	critical_operation_id result = {};
	for (size_t i = 0; i < 8; ++i)
		result.bytes[i] = static_cast<uint8_t>(value >> (8 * i));
	return result;
}
static flatfile_accounting_record record(uint32_t sequence, bool large)
{
	flatfile_accounting_record value;
	critical_operation_id operation = {};
	operation.bytes[0] = 1;
	for (size_t i = 0; i < 4; ++i)
		operation.bytes[15 - i] = static_cast<uint8_t>(sequence >> (8 * i));
	currency_command_payload payload = {};
	payload.pid = 11;
	payload.racewar = 1;
	payload.reason = currency_reason_type::atm_deposit;
	strcpy(payload.account_name.data(), "renamed");
	payload.wallet_delta.amount[0] = -10;
	payload.bank_delta.amount[0] = 10;
	assert(currency_command_build(&value.command, operation, payload, UINT64_MAX, UINT64_MAX,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	value.command.accepted_at_usec = 1;
	economic_currency_authority state;
	state.epoch = id(50);
	state.wallet_account = { id(1), economic_account_kind::wallet, 3, 0 };
	state.bank_account = { id(1), economic_account_kind::bank, 2, 1 };
	state.player_fence = value.command.keys[0];
	state.bank_fence = value.command.keys[1];
	state.state.wallet.amount[0] = 100;
	state.state.bank.amount[0] = 50;
	assert(economic_bank_transfer_intent(
		       value.command, state.epoch, state.wallet_account, state.bank_account,
		       &value.command.accounting_intent) == economic_accounting_error::ok);
	value.command.schema_version = 2;
	economic_frozen_intent intent;
	assert(economic_intent_decode(value.command.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	if (large)
	{
		// Native storage's existing segment-crossing fixture technique: a
		// retained rejected command needs no game execution or activation.
		value.command.schema_version = 1;
		value.command.accounting_intent.clear();
		value.command.payload.resize(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES, 42);
		assert(economic_intent_freeze(value.command, intent.admission,
					      &value.command.accounting_intent) ==
		       economic_accounting_error::ok);
		value.command.schema_version = 2;
		value.result_code = ENOSPC;
		value.failure_stage = critical_failure_stage::coin_source_wallet_revision;
		value.result.resize(CRITICAL_COMPLETION_RESULT_MAX_BYTES, 7);
	}
	else
	{
		std::optional<economic_prepared_currency> prepared;
		assert(economic_bank_transfer_prepare(value.command, intent, state,
						      currency_revision_policy::flatfile_legacy,
						      &prepared) == economic_accounting_error::ok);
		assert(economic_plan_encode(prepared->plan(), &value.plan) ==
		       economic_accounting_error::ok);
		std::array<uint8_t, CURRENCY_RESULT_PAYLOAD_BYTES> result = {};
		assert(currency_command_encode_result(prepared->mutation().after(), &result));
		value.result.assign(result.begin(), result.end());
		value.durable_revision = 1;
	}
	return value;
}
int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string root = argv[1], mode = argv[2];
	for (auto name : { "domains", "economic-evidence" })
	{
		auto path = std::filesystem::path(root) / name;
		std::filesystem::create_directories(path);
		std::filesystem::permissions(path, std::filesystem::perms::owner_all);
	}
	flatfile_authority_lock lock;
	std::string error;
	assert(lock.acquire(root, &error));
	using access = flatfile_accounting_test_access;
	std::vector<flatfile_authority_operation> operations;
	auto commit = [&]
	{
		assert(access::commit(root, lock, operations, &error) ==
		       flatfile_authority_transaction_result::ok);
		operations.clear();
	};
	auto control = [&]
	{
		flatfile_economic_control value;
		assert(flatfile_economic_control_read(root, lock, &value, &error) == 0);
		return value;
	};
	assert(access::bootstrap(root, lock, id(1), id(2), &operations, &error) == 0);
	commit();
	if (mode == "bootstrap")
		return 0;
	if (mode == "evidence-bootstrap")
	{
		assert(access::initialize_evidence_bucket(root, lock, control().revision, 200,
							  id(8), &operations, &error) == 0);
		commit();
		assert(control().epoch_count == 0 &&
		       critical_operation_id_is_zero(control().active_epoch));
		return 0;
	}
	assert(mode == "lifetimes" || mode == "records");
	for (size_t bucket = 0; bucket < 256; ++bucket)
	{
		assert(access::initialize_native_bucket(root, lock, control().revision, bucket,
							id(3), &operations, &error) == 0);
		commit();
	}
	auto create =
		[&](economic_account_kind kind, uint64_t context, flatfile_economic_locator locator)
	{
		flatfile_economic_mapping value;
		assert(access::create_mapping(root, lock, control().revision, kind, context,
					      locator, id(4), &value, &operations, &error) == 0);
		commit();
		return value;
	};
	auto wallet = create(economic_account_kind::wallet, 0, { 1, 11, {} });
	auto bank = create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	assert(access::rename_bank(root, lock, control().revision, bank.account, bank.revision,
				   "renamed", id(5), &operations, &error) == 0);
	commit();
	assert(access::retire_mapping(root, lock, control().revision, wallet.account,
				      wallet.revision, id(6), &operations, &error) == 0);
	commit();
	// A recreated native identity gets a new lifetime; the retired one remains.
	create(economic_account_kind::wallet, 0, { 1, 11, {} });
	create(economic_account_kind::auction_escrow, 0, { 4, UINT32_MAX, {} });
	create(economic_account_kind::pending_claim, 0, { 5, INT32_MAX, {} });
	create(economic_account_kind::treasury, 0, { 6, uint64_t{ UINT32_MAX } + 1, {} });
	flatfile_economic_epoch epoch;
	epoch.epoch = id(50);
	epoch.ordinal = 1;
	epoch.transition_kind = 1;
	epoch.transition_digest[0] = 42;
	epoch.creating_operation = id(7);
	assert(access::append_epoch(root, lock, control().revision, epoch, &operations, &error) ==
	       0);
	commit();
	epoch.predecessor = epoch.epoch;
	epoch.epoch = id(51);
	epoch.ordinal = 2;
	assert(access::append_epoch(root, lock, control().revision, epoch, &operations, &error) ==
	       0);
	commit();
	assert(critical_operation_id_is_zero(control().active_epoch));
	if (mode == "records")
	{
		for (size_t bucket : { 1, 2 })
		{
			assert(access::initialize_evidence_bucket(root, lock, control().revision,
								  bucket, id(8), &operations,
								  &error) == 0);
			commit();
		}
		// More than 8 MiB seals the first segment; every record is native-encoded.
		// Operation identities are distinct, and one successful receipt/plan is retained.
		for (uint32_t sequence = 1; sequence <= 23; ++sequence)
		{
			auto value = record(sequence, sequence != 1);
			assert(access::stage(root, lock, value, &operations, &error) ==
			       flatfile_accounting_status::ok);
			commit();
		}
		assert(critical_operation_id_is_zero(control().active_epoch));
	}
}
