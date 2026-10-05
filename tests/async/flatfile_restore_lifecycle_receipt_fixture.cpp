// Native-codec receipt fixture only. Never calls lifecycle install or selects
// an active epoch. Source descriptors/assertions are modeled, not authenticated
// native cutover authority or complete installer/recovery qualification.
#include "flatfile/flatfile_accounting_lifecycle_transaction.c"
#include <cassert>
#include <filesystem>
#include <fstream>

class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto native_bucket =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto mapping = &flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto rename = &flatfile_accounting_authority_storage::rename_bank;
	static constexpr auto retire = &flatfile_accounting_authority_storage::retire_mapping;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto initialize = &flatfile_accounting_baseline_storage::initialize;
	static constexpr auto initialize_lifecycle =
		&flatfile_accounting_baseline_storage::initialize_lifecycle_staged;
	static constexpr auto evidence_bucket =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto baseline = &flatfile_accounting_baseline_storage::stage;
	static auto commit(const std::string &root, const flatfile_authority_lock &lock,
			   const std::vector<flatfile_authority_operation> &operations,
			   std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, error);
	}
};
static critical_operation_id fixture_id(uint64_t number)
{
	critical_operation_id result = {};
	for (size_t i = 0; i < 8; ++i)
		result.bytes[i] = static_cast<uint8_t>(number >> (i * 8));
	return result;
}
int main(int argc, char **argv)
{
	assert(argc == 3);
	const std::string root = argv[1], mode = argv[2];
	assert(mode == "mixed" || mode == "empty" || mode == "renamed" || mode == "retired" ||
	       mode == "maximum" || mode == "generic" || mode == "retained");
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
		assert(critical_operation_id_is_zero(value.active_epoch));
		return value;
	};
	assert(access::bootstrap(root, lock, fixture_id(1), fixture_id(2), &operations, &error) ==
	       0);
	commit();
	for (size_t bucket = 0; bucket < 256; ++bucket)
	{
		assert(access::native_bucket(root, lock, control().revision, bucket, fixture_id(3),
					     &operations, &error) == 0);
		commit();
	}
	retained_lifecycle value;
	auto &request = value.request;
	request.operation_id = fixture_id(20);
	request.lineage = fixture_id(1);
	request.epoch = fixture_id(50);
	request.actor_id = 7;
	request.accepted_at_usec = 1;
	request.boundary_digest.fill(11);
	request.frozen_boundary_proven = request.virgin_state_proven = true;
	value.opening = { request.lineage, economic_account_kind::opening, 1, UINT64_MAX };
	auto create = [&](economic_account_kind kind, uint64_t context,
			  const flatfile_economic_locator &locator)
	{
		flatfile_economic_mapping mapping;
		assert(access::mapping(root, lock, control().revision, kind, context, locator,
				       request.operation_id, &mapping, &operations, &error) == 0);
		commit();
		value.receipt.mappings.push_back(mapping);
	};
	if (mode != "empty" && mode != "maximum")
	{
		flatfile_accounting_lifecycle_wallet_source wallet;
		wallet.pid = 1;
		wallet.account_name = "synthetic";
		wallet.racewar = 1;
		wallet.balance = { 100, 2, 3, 4 };
		wallet.native_revision = UINT64_MAX;
		wallet.source_digest =
			holding_source_digest(1, wallet.pid, wallet.racewar, wallet.account_name,
					      wallet.native_revision, wallet.balance);
		value.sources.wallets.push_back(wallet);
		create(economic_account_kind::wallet, 0, { 1, 1, {} });
	}
	const size_t bank_count = mode == "empty"   ? 0 :
				  mode == "maximum" ? ECONOMIC_BASELINE_MAX_HOLDINGS :
						      1;
	for (size_t index = 0; index < bank_count; ++index)
	{
		flatfile_accounting_lifecycle_bank_source bank;
		bank.name = mode == "maximum" ?
				    std::string(40, 'a') +
					    std::string(10 - std::to_string(index).size(), '0') +
					    std::to_string(index) :
				    "synthetic";
		bank.racewar = 1;
		bank.balance = { 50, 1, 1, 0 };
		bank.native_revision = UINT64_MAX;
		bank.source_digest = holding_source_digest(2, 0, bank.racewar, bank.name,
							   bank.native_revision, bank.balance);
		value.sources.banks.push_back(bank);
		create(economic_account_kind::bank, bank.racewar, { 2, 0, bank.name });
	}
	const auto coverage = compute_coverage_digest(value.sources.wallets, value.sources.banks);
	if (mode == "retained")
		request.coverage_digest = coverage;
	value.epoch.epoch = request.epoch;
	value.epoch.ordinal = 1;
	value.epoch.transition_kind = 1;
	value.epoch.transition_digest = coverage;
	value.epoch.creating_operation = request.operation_id;
	assert(access::append_epoch(root, lock, control().revision, value.epoch, &operations,
				    &error) == 0);
	commit();
	const auto initialized =
		mode == "generic" ?
			access::initialize(root, lock, request.lineage, request.epoch,
					   value.opening, request.operation_id, &operations,
					   &error) :
			access::initialize_lifecycle(root, lock, request.lineage, request.epoch,
						     value.opening, request.operation_id,
						     &operations, &error, nullptr);
	assert(initialized == flatfile_accounting_status::ok);
	commit();
	economic_baseline_batch batch;
	batch.lineage = request.lineage;
	batch.epoch = request.epoch;
	batch.preparation_id = request.operation_id;
	batch.actor_id = request.actor_id;
	batch.opening_account = value.opening;
	batch.boundary_digest = request.boundary_digest;
	batch.coverage_digest = coverage;
	for (size_t i = 0; i < value.receipt.mappings.size(); ++i)
	{
		const bool wallet = i < value.sources.wallets.size();
		const auto balance =
			wallet ? value.sources.wallets[i].balance :
				 value.sources.banks[i - value.sources.wallets.size()].balance;
		const auto revision = wallet ? value.sources.wallets[i].native_revision :
					       value.sources.banks[i - value.sources.wallets.size()]
						       .native_revision;
		const auto digest =
			wallet ?
				value.sources.wallets[i].source_digest :
				value.sources.banks[i - value.sources.wallets.size()].source_digest;
		batch.holdings.push_back(
			{ value.receipt.mappings[i].account, balance, revision, digest });
	}
	std::optional<economic_prepared_baseline> prepared;
	assert(economic_baseline_prepare(batch, &prepared) == economic_accounting_error::ok);
	critical_command command;
	assert(economic_baseline_command_build(*prepared, request.accepted_at_usec, &command) ==
	       economic_accounting_error::ok);
	assert(access::evidence_bucket(root, lock, control().revision,
				       command.operation_id.bytes[0], fixture_id(8), &operations,
				       &error) == 0);
	commit();
	assert(access::baseline(root, lock, command, *prepared, &operations, &error) ==
	       flatfile_accounting_status::ok);
	commit();
	if (mode == "generic")
	{
		(void)control();
		return 0; // Coincident original IDs do not turn a generic book into an installer receipt.
	}
	flatfile_accounting_record stored;
	std::vector<uint8_t> stored_witness;
	assert(flatfile_accounting_baseline_lookup(root, lock, command, &stored, &stored_witness,
						   &error) == flatfile_accounting_status::ok);
	assert(!stored.result_code && stored.failure_stage == critical_failure_stage::none &&
	       stored.result.empty());
	assert(flatfile_economic_epoch_read(root, lock, request.lineage, request.epoch,
					    &value.epoch, &error) == 0);
	assert(value.epoch.initialization_origin ==
	       flatfile_baseline_initialization_origin::lifecycle_owner);
	value.lineage_creating_operation = control().creating_operation;
	value.selected_control_revision = control().revision;
	auto &receipt = value.receipt;
	receipt.operation_id = request.operation_id;
	receipt.lineage = request.lineage;
	receipt.epoch = request.epoch;
	receipt.baseline_operation_id = command.operation_id;
	receipt.coverage_digest = coverage;
	receipt.boundary_digest = request.boundary_digest;
	receipt.baseline_revision = stored.durable_revision;
	assert(critical_command_encode(command, &value.baseline_command) ==
	       critical_command_codec_result::ok);
	assert(economic_baseline_encode(*prepared, &value.baseline_witness) ==
	       economic_accounting_error::ok);
	economic_accounting_plan plan;
	assert(economic_baseline_command_plan(command, *prepared, &plan) ==
	       economic_accounting_error::ok);
	assert(economic_plan_encode(plan, &value.baseline_plan) == economic_accounting_error::ok);
	assert(value.baseline_witness == stored_witness && value.baseline_plan == stored.plan);
	auto encoded = encode_lifecycle_receipt(value);
	assert(encode_lifecycle_receipt(decode_lifecycle_receipt(encoded)) == encoded);
	const auto path = std::filesystem::path(root) / "economic-evidence" /
			  lifecycle_receipt_name(request.operation_id);
	std::ofstream out(path, std::ios::binary);
	out.write(reinterpret_cast<const char *>(encoded.data()), encoded.size());
	out.close();
	assert(out);
	std::filesystem::permissions(path, std::filesystem::perms::owner_read |
						   std::filesystem::perms::owner_write);
	if (mode == "renamed" || mode == "retired")
	{
		const auto &bank = receipt.mappings.back();
		assert(access::rename(root, lock, control().revision, bank.account, bank.revision,
				      "changed", fixture_id(99), &operations, &error) == 0);
		commit();
		if (mode == "retired")
		{
			const auto &wallet = receipt.mappings.front();
			assert(access::retire(root, lock, control().revision, wallet.account,
					      wallet.revision, fixture_id(100), &operations,
					      &error) == 0);
			commit();
		}
	}
	if (mode == "retained")
	{
		flatfile_economic_epoch newer;
		newer.epoch = fixture_id(51);
		newer.predecessor = request.epoch;
		newer.ordinal = 2;
		newer.transition_kind = 1;
		newer.transition_digest.fill(42);
		newer.creating_operation = fixture_id(99);
		assert(access::append_epoch(root, lock, control().revision, newer, &operations,
					    &error) == 0);
		commit();
	}
	(void)control(); // Every fixture remains inactive.
}
