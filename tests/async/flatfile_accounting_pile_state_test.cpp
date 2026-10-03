#include "flatfile/flatfile_accounting_pile_state.h"

#include <cassert>
#include <cstdlib>
#include <filesystem>
#include <fstream>

namespace fs = std::filesystem;

class flatfile_accounting_test_access
{
    public:
	static flatfile_authority_transaction_result
	commit(const std::string &root, const flatfile_authority_lock &lock,
	       const std::vector<flatfile_authority_operation> &operations)
	{
		return flatfile_accounting_storage::commit(root, lock, operations, nullptr);
	}
};

static critical_operation_id id(uint8_t value)
{
	critical_operation_id result;
	result.bytes[0] = value;
	return result;
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const std::string root = argv[1];
	fs::create_directories(root);
	fs::create_directories(fs::path(root) / "domains");
	fs::create_directories(fs::path(root) / "economic-evidence");
	for (const auto &path :
	     { fs::path(root), fs::path(root) / "domains", fs::path(root) / "economic-evidence" })
		fs::permissions(path, fs::perms::owner_all);
	flatfile_authority_lock lock;
	assert(lock.acquire(root, nullptr));
	constexpr uint64_t uid = (uint64_t{ 1 } << 40) + 23;
	const economic_account_key key{ id(1), economic_account_kind::pile, uid, 0 };
	economic_account_effect create{ key, {}, { 0, 0, 1, 0 }, 0, 1 };
	flatfile_accounting_pile_state state;
	assert(flatfile_accounting_pile_state_read(root, lock, uid, &state, nullptr) ==
	       flatfile_accounting_status::not_found);
	std::vector<flatfile_authority_operation> operations;
	assert(flatfile_accounting_pile_state_stage(root, lock, create, id(2), id(3), false,
						    &operations,
						    nullptr) == flatfile_accounting_status::ok);
	assert(operations.size() == 1 &&
	       operations[0].filename == "pile-head-0000010000000017.eph");
	assert(flatfile_accounting_pile_state_read(root, lock, uid, &state, nullptr) ==
	       flatfile_accounting_status::not_found);
	assert(flatfile_accounting_test_access::commit(root, lock, operations) ==
	       flatfile_authority_transaction_result::ok);
	assert(flatfile_accounting_pile_state_read(root, lock, uid, &state, nullptr) ==
		       flatfile_accounting_status::ok &&
	       state.account.authority_id == uid && state.balance == create.after &&
	       state.item_revision == 1 && !state.retired);
	operations.clear();
	assert(flatfile_accounting_pile_state_stage(root, lock, create, id(2), id(4), false,
						    &operations, nullptr) ==
		       flatfile_accounting_status::already_exists &&
	       operations.empty());

	economic_account_effect stale{ key, { 0, 0, 2, 0 }, { 0, 0, 3, 0 }, 1, 2 };
	assert(flatfile_accounting_pile_state_stage(root, lock, stale, id(2), id(4), false,
						    &operations, nullptr) ==
		       flatfile_accounting_status::conflict &&
	       operations.empty());
	economic_account_effect merge{ key, create.after, { 0, 0, 3, 0 }, 1, 4 };
	assert(flatfile_accounting_pile_state_stage(root, lock, merge, id(2), id(5), false,
						    &operations,
						    nullptr) == flatfile_accounting_status::ok);
	setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL", "1", 1);
	assert(flatfile_accounting_test_access::commit(root, lock, operations) ==
	       flatfile_authority_transaction_result::io_error);
	unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL");
	assert(flatfile_accounting_pile_state_read(root, lock, uid, &state, nullptr) ==
		       flatfile_accounting_status::ok &&
	       state.balance == merge.after && state.item_revision == 4);
	operations.clear();
	economic_account_effect retire{ key, merge.after, {}, 4, 5 };
	assert(flatfile_accounting_pile_state_stage(root, lock, retire, id(2), id(6), true,
						    &operations,
						    nullptr) == flatfile_accounting_status::ok);
	assert(flatfile_accounting_test_access::commit(root, lock, operations) ==
	       flatfile_authority_transaction_result::ok);
	assert(flatfile_accounting_pile_state_read(root, lock, uid, &state, nullptr) ==
		       flatfile_accounting_status::ok &&
	       state.retired && state.balance == economic_coin_vector{});
	operations.clear();
	assert(flatfile_accounting_pile_state_stage(root, lock, create, id(2), id(7), false,
						    &operations, nullptr) ==
		       flatfile_accounting_status::already_exists &&
	       operations.empty());

	const economic_account_key existing_key{ id(1), economic_account_kind::pile, uid + 1, 0 };
	flatfile_accounting_pile_state baseline{ existing_key,	 id(2), id(8),
						 { 0, 5, 0, 0 }, 7,	false };
	assert(flatfile_accounting_pile_state_stage_baseline(root, lock, baseline, &operations,
							     nullptr) ==
	       flatfile_accounting_status::ok);
	assert(flatfile_accounting_pile_state_stage_baseline(root, lock, baseline, &operations,
							     nullptr) ==
	       flatfile_accounting_status::conflict);
	assert(flatfile_accounting_test_access::commit(root, lock, operations) ==
	       flatfile_authority_transaction_result::ok);
	operations.clear();
	assert(flatfile_accounting_pile_state_read(root, lock, uid + 1, &state, nullptr) ==
		       flatfile_accounting_status::ok &&
	       state.balance == baseline.balance && state.item_revision == 7);
	assert(flatfile_accounting_pile_state_stage_baseline(root, lock, baseline, &operations,
							     nullptr) ==
		       flatfile_accounting_status::already_exists &&
	       operations.empty());
	auto changed_baseline = baseline;
	changed_baseline.balance[1] = 6;
	assert(flatfile_accounting_pile_state_stage_baseline(root, lock, changed_baseline,
							     &operations, nullptr) ==
		       flatfile_accounting_status::conflict &&
	       operations.empty());
	economic_account_effect updated{ existing_key, baseline.balance, { 0, 3, 0, 0 }, 7, 9 };
	assert(flatfile_accounting_pile_state_stage(root, lock, updated, id(2), id(9), false,
						    &operations,
						    nullptr) == flatfile_accounting_status::ok);
	assert(flatfile_accounting_test_access::commit(root, lock, operations) ==
	       flatfile_authority_transaction_result::ok);
	assert(flatfile_accounting_pile_state_read(root, lock, uid + 1, &state, nullptr) ==
		       flatfile_accounting_status::ok &&
	       state.balance == updated.after && state.item_revision == 9);

	const auto path = fs::path(root) / "economic-evidence" / "pile-head-0000010000000017.eph";
	std::fstream file(path, std::ios::in | std::ios::out | std::ios::binary);
	assert(file);
	file.seekp(52);
	file.put('\x7f');
	file.close();
	assert(flatfile_accounting_pile_state_read(root, lock, uid, &state, nullptr) ==
	       flatfile_accounting_status::invalid);
}
