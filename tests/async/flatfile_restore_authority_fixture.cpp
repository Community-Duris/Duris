// Synthetic, private fixture encoded by the native metadata writers. Never
// selects an active epoch or runs a production lifecycle/activation owner.
#include "flatfile/flatfile_accounting_authority.h"
#include <cassert>
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
	assert(mode == "lifetimes");
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
}
