// Synthetic, private fixture encoded by the native metadata writers. Never
// selects an active epoch or runs a production lifecycle/activation owner.
#include "flatfile/flatfile_accounting_authority.h"
#include "economy/economic_currency_adapter.h"
#include "flatfile/flatfile_accounting_baseline.h"
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
	static constexpr auto stage_source_claim = &flatfile_accounting_storage::stage_source_claim;
	static constexpr auto initialize_baseline =
		&flatfile_accounting_baseline_storage::initialize;
	static constexpr auto stage_baseline = &flatfile_accounting_baseline_storage::stage;
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
static flatfile_accounting_record record(uint32_t sequence, bool large, bool source = false,
					 int32_t pid = 11)
{
	flatfile_accounting_record value;
	critical_operation_id operation = {};
	operation.bytes[0] = 1;
	for (size_t i = 0; i < 4; ++i)
		operation.bytes[15 - i] = static_cast<uint8_t>(sequence >> (8 * i));
	currency_command_payload payload = {};
	payload.pid = pid;
	payload.racewar = 1;
	payload.reason = source ? currency_reason_type::wallet_reward :
				  currency_reason_type::atm_deposit;
	payload.reason_id = source ? 3 : 0;
	strcpy(payload.account_name.data(), "renamed");
	payload.wallet_delta.amount[0] = source ? 10 : -10;
	payload.bank_delta.amount[0] = source ? 0 : 10;
	assert(currency_command_build(
		&value.command, operation, payload, UINT64_MAX, UINT64_MAX,
		source ? critical_source_site::recovery : critical_source_site::command,
		source ? critical_deadline_class::recovery : critical_deadline_class::interactive));
	value.command.accepted_at_usec = 1;
	economic_currency_authority state;
	state.epoch = id(source && sequence == 2 ? 51 : 50);
	state.wallet_account = { id(1), economic_account_kind::wallet, 3, 0 };
	state.bank_account = { id(1), economic_account_kind::bank, 2, 1 };
	state.player_fence = value.command.keys[0];
	state.bank_fence = value.command.keys[1];
	state.state.wallet.amount[0] = 100;
	state.state.bank.amount[0] = 50;
	const auto freeze = source ? economic_quest_wallet_reward_intent :
				     economic_bank_transfer_intent;
	assert(freeze(value.command, state.epoch, state.wallet_account, state.bank_account,
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
		const auto prepare = source ? economic_quest_wallet_reward_prepare :
					      economic_bank_transfer_prepare;
		assert(prepare(value.command, intent, state,
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
	assert(mode == "lifetimes" || mode == "records" || mode == "source-claims" ||
	       mode == "retention" || mode == "baseline" || mode == "baseline-empty" ||
	       mode == "baseline-rich" || mode == "baseline-maximum" ||
	       mode == "baseline-full-index");
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
	const int32_t pid = mode == "retention" ? 1 : 11;
	auto wallet =
		create(economic_account_kind::wallet, 0, { 1, static_cast<uint64_t>(pid), {} });
	auto bank = create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	assert(access::rename_bank(root, lock, control().revision, bank.account, bank.revision,
				   "renamed", id(5), &operations, &error) == 0);
	commit();
	assert(access::retire_mapping(root, lock, control().revision, wallet.account,
				      wallet.revision, id(6), &operations, &error) == 0);
	commit();
	// A recreated native identity gets a new lifetime; the retired one remains.
	create(economic_account_kind::wallet, 0, { 1, static_cast<uint64_t>(pid), {} });
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
	if (mode.starts_with("baseline"))
	{
		economic_baseline_batch batch;
		batch.lineage = id(1);
		batch.epoch = id(50);
		batch.preparation_id = id(52);
		batch.actor_id = 7;
		batch.opening_account = { id(1), economic_account_kind::opening, 1, 0 };
		batch.boundary_digest.fill(1);
		batch.coverage_digest.fill(2);
		economic_digest native_digest;
		native_digest.fill(3);
		batch.holdings.push_back({ { id(1), economic_account_kind::wallet, 3, 0 },
					   { 100, 0, 0, 0 },
					   0,
					   native_digest });
		assert(access::initialize_baseline(root, lock, id(1), batch.epoch,
						   batch.opening_account, id(53), &operations,
						   &error) == flatfile_accounting_status::ok);
		commit();
		if (mode == "baseline-empty")
			return 0;
		auto item = [&](uint64_t uid, item_owner_identity owner, uint64_t root_uid,
				uint64_t parent, uint64_t revision, item_custody_state state)
		{
			economic_item_snapshot snapshot;
			snapshot.uid = uid;
			snapshot.position = { owner, root_uid, parent, revision, state, 0 };
			batch.items.push_back({ snapshot, native_digest });
		};
		if (mode == "baseline-rich")
		{
			batch.holdings.clear();
			// Deliberately unsorted inputs cross little-endian numeric boundaries.
			for (uint64_t lifetime : { 512u, 256u, 255u })
				batch.holdings.push_back(
					{ { id(1), economic_account_kind::wallet, lifetime, 0 },
					  { lifetime == 512 ? 0 : 10, 2, 3, 4 },
					  UINT64_MAX,
					  native_digest });
			batch.holdings.back().balance = {};
			batch.holdings.push_back(
				{ { id(1), economic_account_kind::bank, UINT64_MAX, UINT64_MAX },
				  { INT64_MAX, 0, 0, 0 },
				  UINT64_MAX,
				  native_digest });
			item(256, { item_owner_type::player, 7, 0 }, 255, 255, 0,
			     item_custody_state::quarantined);
			item(255, { item_owner_type::player, 7, 0 }, 255, 0, UINT64_MAX,
			     item_custody_state::active);
			item(512, { item_owner_type::destruction, 0, 0 }, UINT64_MAX, UINT64_MAX,
			     UINT64_MAX, item_custody_state::destroyed);
			item(513, { item_owner_type::pet, 8, INT32_MAX }, 513, 0, UINT64_MAX,
			     item_custody_state::active);
			item(514, { item_owner_type::collector, 9, 0 }, 514, 0, 0,
			     item_custody_state::quarantined);
			item(515, { item_owner_type::system, 0, 0 }, 515, 0, 0,
			     item_custody_state::active);
		}
		if (mode == "baseline-maximum")
		{
			batch.holdings.clear();
			for (uint64_t lifetime = 1; lifetime <= ECONOMIC_BASELINE_MAX_HOLDINGS;
			     ++lifetime)
				batch.holdings.push_back(
					{ { id(1), economic_account_kind::wallet, lifetime, 0 },
					  { 1, 2, 3, 4 },
					  UINT64_MAX,
					  native_digest });
			for (uint64_t uid = 10000;
			     uid < 10000 + ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++uid)
				item(uid, { item_owner_type::player, 7, 0 }, 10000,
				     uid == 10000 ? 0 : uid - 1, UINT64_MAX,
				     item_custody_state::active);
		}
		auto stage = [&]
		{
			std::optional<economic_prepared_baseline> prepared;
			assert(economic_baseline_prepare(batch, &prepared) ==
			       economic_accounting_error::ok);
			critical_command command;
			assert(economic_baseline_command_build(*prepared, UINT64_MAX, &command) ==
			       economic_accounting_error::ok);
			const auto bucket = command.operation_id.bytes[0];
			if (!(control().evidence_initialized[bucket / 8] & (1u << (bucket % 8))))
			{
				assert(access::initialize_evidence_bucket(
					       root, lock, control().revision, bucket, id(8),
					       &operations, &error) == 0);
				commit();
			}
			assert(access::stage_baseline(root, lock, command, *prepared, &operations,
						      &error) == flatfile_accounting_status::ok);
			commit();
		};
		if (mode == "baseline-full-index")
		{
			// A full native shard is 88 bytes larger than a generic 2 MiB file.
			for (uint64_t first = 1; first <= 65536;
			     first += ECONOMIC_BASELINE_MAX_HOLDINGS)
			{
				batch.batch_index = (first - 1) / ECONOMIC_BASELINE_MAX_HOLDINGS;
				batch.holdings.clear();
				for (uint64_t lifetime = first;
				     lifetime <= 65536 &&
				     lifetime < first + ECONOMIC_BASELINE_MAX_HOLDINGS;
				     ++lifetime)
					batch.holdings.push_back(
						{ { id(1), economic_account_kind::wallet,
						    lifetime * 16, 0 },
						  {},
						  UINT64_MAX,
						  native_digest });
				stage();
			}
		}
		else
			stage();
		if (mode == "baseline-rich")
		{
			const auto first = batch;
			batch.batch_index = 1;
			batch.holdings = { { { id(1), economic_account_kind::treasury, 1025, 0 },
					     {},
					     UINT64_MAX,
					     native_digest } };
			batch.items.clear();
			item(1025, { item_owner_type::room, 300, UINT64_MAX }, 1025, 0, 0,
			     item_custody_state::active);
			stage();
			batch.batch_index = UINT64_MAX;
			batch.holdings.clear();
			batch.items.clear();
			stage();
			batch = first;
			batch.preparation_id = id(54);
			batch.epoch = id(51);
			assert(access::initialize_baseline(root, lock, id(1), batch.epoch,
							   batch.opening_account, id(55),
							   &operations, &error) ==
			       flatfile_accounting_status::ok);
			commit();
			stage();
		}
		assert(critical_operation_id_is_zero(control().active_epoch));
		return 0;
	}
	if (mode == "records" || mode == "source-claims" || mode == "retention")
	{
		const bool source = mode != "records";
		for (size_t bucket : { 1, 2 })
		{
			assert(access::initialize_evidence_bucket(root, lock, control().revision,
								  bucket, id(8), &operations,
								  &error) == 0);
			commit();
		}
		// Records mode seals the first segment. Source mode retains two successful
		// claims and two claimless rejections. No native domain effect is applied.
		for (uint32_t sequence = 1; sequence <= (mode == "records" ? 23u : 4u); ++sequence)
		{
			auto value =
				record(sequence, mode == "records" && sequence != 1, source, pid);
			if (source && sequence >= 3)
			{
				value.plan.clear();
				value.result_code = EEXIST;
				if (sequence == 4)
				{
					// A rejected retry may share a successful root's source.
					economic_frozen_intent intent, original;
					assert(economic_intent_decode(
						       value.command.accounting_intent, &intent) ==
					       economic_accounting_error::ok);
					assert(economic_intent_decode(
						       record(1, false, true, pid)
							       .command.accounting_intent,
						       &original) == economic_accounting_error::ok);
					intent.admission.metadata.source_event =
						original.admission.metadata.source_event;
					value.command.schema_version = 1;
					value.command.accounting_intent.clear();
					assert(economic_intent_freeze(
						       value.command, intent.admission,
						       &value.command.accounting_intent) ==
					       economic_accounting_error::ok);
					value.command.schema_version = 2;
				}
			}
			assert(access::stage(root, lock, value, &operations, &error) ==
			       flatfile_accounting_status::ok);
			assert(access::stage_source_claim(root, lock, value, &operations, &error) ==
			       flatfile_accounting_status::ok);
			commit();
		}
		assert(critical_operation_id_is_zero(control().active_epoch));
	}
}
