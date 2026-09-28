#include "flatfile/flatfile_accounting_store.h"
#include "flatfile/flatfile_store.h"
#include "economy/economic_currency_adapter.h"
#include <cassert>
#include <cerrno>
#include <cstring>
#include <filesystem>
#include <fstream>
#include <new>
#include <sys/wait.h>
#include <unistd.h>
#include <openssl/sha.h>

// Linker wrappers inject failures in the real authority publication protocol.
// They are armed only around a commit/recovery call, never fixture setup.
enum class io_call
{
	none,
	write,
	data_sync,
	rename,
	directory_sync,
	remove
};
enum class io_fault
{
	fail,
	zero_write,
	short_write,
	interrupted_write,
	exit_after
};
struct io_injection
{
	io_call call = io_call::none;
	io_fault fault = io_fault::fail;
	size_t target = 0, observed = 0;
	bool reached = false;
} injection;
bool inject(io_call call)
{
	if (injection.call != call || ++injection.observed != injection.target)
		return false;
	injection.reached = true;
	return true;
}
extern "C" ssize_t __real_write(int, const void *, size_t);
extern "C" int __real_fdatasync(int);
extern "C" int __real_fsync(int);
extern "C" int __real_renameat(int, const char *, int, const char *);
extern "C" int __real_unlinkat(int, const char *, int);
extern "C" ssize_t __wrap_write(int fd, const void *bytes, size_t size)
{
	const bool selected = inject(io_call::write);
	if (selected)
	{
		switch (injection.fault)
		{
		case io_fault::fail:
			errno = ENOSPC;
			return -1;
		case io_fault::zero_write:
			return 0;
		case io_fault::short_write:
			assert(size > 1);
			return __real_write(fd, bytes, size / 2);
		case io_fault::interrupted_write:
			errno = EINTR;
			return -1;
		case io_fault::exit_after:
			break;
		}
	}
	auto result = __real_write(fd, bytes, size);
	if (selected && injection.fault == io_fault::exit_after && result >= 0)
		_exit(77);
	return result;
}
template <typename Work> int inject_io(io_call call, Work work)
{
	const bool selected = inject(call);
	if (selected && injection.fault == io_fault::fail)
	{
		errno = EIO;
		return -1;
	}
	const int result = work();
	if (selected && injection.fault == io_fault::exit_after && result == 0)
		_exit(77);
	return result;
}
extern "C" int __wrap_fdatasync(int fd)
{
	return inject_io(io_call::data_sync, [&] { return __real_fdatasync(fd); });
}
extern "C" int __wrap_fsync(int fd)
{
	return inject_io(io_call::directory_sync, [&] { return __real_fsync(fd); });
}
extern "C" int __wrap_renameat(int a, const char *b, int c, const char *d)
{
	return inject_io(io_call::rename, [&] { return __real_renameat(a, b, c, d); });
}
extern "C" int __wrap_unlinkat(int a, const char *b, int c)
{
	return inject_io(io_call::remove, [&] { return __real_unlinkat(a, b, c); });
}

// Wrap only calls from this harness and linked production objects. The real
// allocator remains paired with its normal delete implementation under ASan.
static bool fail_allocation = false, allocation_reached = false;
static size_t allocation_size = 0;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__wrap__Znwm(size_t size)
{
	if (fail_allocation && (!allocation_size || size == allocation_size))
	{
		fail_allocation = false;
		allocation_reached = true;
		throw std::bad_alloc();
	}
	return __real__Znwm(size);
}

class flatfile_accounting_test_access
{
    public:
	static auto initialize(const std::string &root, const flatfile_authority_lock &lock,
			       const critical_operation_id &lineage, size_t bucket,
			       std::vector<flatfile_authority_operation> *ops, std::string *error)
	{
		return flatfile_accounting_storage::initialize_bucket(root, lock, lineage, bucket,
								      ops, error);
	}
	static auto stage(const std::string &root, const flatfile_authority_lock &lock,
			  const flatfile_accounting_record &record,
			  std::vector<flatfile_authority_operation> *ops, std::string *error)
	{
		return flatfile_accounting_storage::stage(root, lock, record, ops, error);
	}
	static auto commit(const std::string &root, const flatfile_authority_lock &lock,
			   const std::vector<flatfile_authority_operation> &ops, std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, ops, error);
	}
};
using status = flatfile_accounting_status;
namespace fs = std::filesystem;
critical_operation_id id(uint32_t number)
{
	critical_operation_id value = {};
	value.bytes[0] = 1;
	for (size_t i = 0; i < 4; ++i)
		value.bytes[15 - i] = static_cast<uint8_t>(number >> (8 * i));
	return value;
}
flatfile_accounting_record record(uint32_t sequence, bool rejected = false)
{
	flatfile_accounting_record value;
	currency_command_payload payload = {};
	payload.pid = 1;
	payload.racewar = 1;
	payload.reason = currency_reason_type::atm_deposit;
	strcpy(payload.account_name.data(), "synthetic");
	payload.wallet_delta.amount[0] = -10;
	payload.bank_delta.amount[0] = 10;
	assert(currency_command_build(&value.command, id(sequence), payload, UINT64_MAX, UINT64_MAX,
				      critical_source_site::command,
				      critical_deadline_class::interactive));
	value.command.accepted_at_usec = 1;
	economic_currency_authority state;
	state.epoch = id(2);
	state.wallet_account = { id(1), economic_account_kind::wallet, 1, 0 };
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
	if (rejected)
	{
		value.result_code = ENOSPC;
		value.failure_stage = critical_failure_stage::coin_source_wallet_revision;
		value.result = { 1, 2, 3 };
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
flatfile_accounting_record large_record(uint32_t sequence)
{
	auto value = record(sequence, true);
	economic_frozen_intent intent;
	assert(economic_intent_decode(value.command.accounting_intent, &intent) ==
	       economic_accounting_error::ok);
	value.command.schema_version = 1;
	value.command.accounting_intent.clear();
	value.command.payload.resize(CRITICAL_COMMAND_MAX_PAYLOAD_BYTES, 42);
	assert(economic_intent_freeze(value.command, intent.admission,
				      &value.command.accounting_intent) ==
	       economic_accounting_error::ok);
	value.command.schema_version = 2;
	value.result.resize(CRITICAL_COMPLETION_RESULT_MAX_BYTES, 7);
	return value;
}
void number(std::vector<uint8_t> &bytes, uint64_t value, size_t width)
{
	for (size_t i = 0; i < width; ++i)
		bytes.push_back(static_cast<uint8_t>(value >> (i * 8)));
}
void raw(std::vector<uint8_t> &bytes, std::span<const uint8_t> value)
{
	bytes.insert(bytes.end(), value.begin(), value.end());
}
std::array<uint8_t, 32> hash(std::span<const uint8_t> bytes)
{
	std::array<uint8_t, 32> result = {};
	SHA256(bytes.data(), bytes.size(), result.data());
	return result;
}
std::vector<uint8_t> envelope(const char *magic, const std::vector<uint8_t> &payload)
{
	std::vector<uint8_t> result(magic, magic + 8);
	number(result, 1, 4);
	number(result, payload.size(), 4);
	raw(result, hash(payload));
	raw(result, payload);
	return result;
}
std::vector<uint8_t> read(const fs::path &path)
{
	std::ifstream file(path, std::ios::binary);
	return { std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>() };
}
void write(const fs::path &path, const std::vector<uint8_t> &bytes)
{
	std::ofstream file(path, std::ios::binary | std::ios::trunc);
	file.write(reinterpret_cast<const char *>(bytes.data()), bytes.size());
	file.close();
	fs::permissions(path, fs::perms::owner_read | fs::perms::owner_write);
}
void provision(const fs::path &root)
{
	for (const auto &path : { root, root / "domains", root / "economic-evidence" })
	{
		fs::create_directories(path);
		fs::permissions(path, fs::perms::owner_all);
	}
}
void qualify_io_faults(const fs::path &parent)
{
	const auto value = record(777);
	const std::vector<std::pair<io_call, size_t>> boundaries = { { io_call::write, 4 },
								     { io_call::data_sync, 4 },
								     { io_call::rename, 4 },
								     { io_call::directory_sync, 5 },
								     { io_call::remove, 1 } };
	size_t cases = 0;
	for (bool recovery : { false, true })
		for (const auto &[call, count] : boundaries)
			for (size_t ordinal = 1;
			     ordinal <=
			     count - static_cast<size_t>(recovery && call != io_call::remove);
			     ++ordinal)
				for (auto fault :
				     { io_fault::fail, io_fault::exit_after, io_fault::zero_write,
				       io_fault::short_write, io_fault::interrupted_write })
				{
					if (call != io_call::write && fault != io_fault::fail &&
					    fault != io_fault::exit_after)
						continue;
					const auto root = parent / std::to_string(++cases);
					provision(root);
					std::string error;
					std::vector<flatfile_authority_operation> operations;
					{
						flatfile_authority_lock lock;
						assert(lock.acquire(root.string(), &error));
						assert(flatfile_accounting_test_access::initialize(
							       root.string(), lock, id(1), 1,
							       &operations, &error) == status::ok);
						assert(flatfile_accounting_test_access::commit(
							       root.string(), lock, operations,
							       &error) ==
						       flatfile_authority_transaction_result::ok);
						write(root / "domains/value", { 0, 0 });
						operations = {
							{ flatfile_authority_store::domains,
							  flatfile_authority_operation_kind::write,
							  "value",
							  { 1, 1 } }
						};
						assert(flatfile_accounting_test_access::stage(
							       root.string(), lock, value,
							       &operations, &error) == status::ok);
						if (recovery)
						{
							setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION",
							       "1", 1);
							assert(flatfile_accounting_test_access::
								       commit(root.string(), lock,
									      operations, &error) ==
							       flatfile_authority_transaction_result::
								       io_error);
							unsetenv(
								"DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION");
						}
					}
					const bool crash = fault == io_fault::exit_after;
					pid_t child = -1;
					if (crash)
					{
						child = fork();
						assert(child >= 0);
					}
					if (!crash || child == 0)
					{
						flatfile_authority_lock lock;
						assert(lock.acquire(root.string(), &error));
						injection = { call, fault, ordinal, 0, false };
						const bool succeeds =
							fault == io_fault::short_write ||
							fault == io_fault::interrupted_write;
						if (recovery)
						{
							flatfile_accounting_record probe;
							probe.result = { 99 };
							const auto result =
								flatfile_accounting_lookup(
									root.string(), lock,
									value.command, &probe,
									&error);
							const bool reached = injection.reached;
							injection = {};
							assert(!crash && reached &&
							       result ==
								       (succeeds ?
										status::ok :
										status::io_error));
							assert(probe.result ==
							       (succeeds ? value.result :
									   std::vector<uint8_t>{
										   99 }));
						}
						else
						{
							const auto result =
								flatfile_accounting_test_access::
									commit(root.string(), lock,
									       operations, &error);
							const bool reached = injection.reached;
							injection = {};
							assert(!crash && reached &&
							       result ==
								       (succeeds ?
										flatfile_authority_transaction_result::
											ok :
										flatfile_authority_transaction_result::
											io_error));
						}
					}
					if (crash)
					{
						int outcome = 0;
						assert(waitpid(child, &outcome, 0) == child &&
						       WIFEXITED(outcome) &&
						       WEXITSTATUS(outcome) == 77);
					}
					// A process exit after writing/syncing the journal temp file has not
					// published it. A rename makes the whole recovery bundle observable.
					const bool before_journal =
						!recovery && ordinal == 1 &&
						((call == io_call::write &&
						  fault != io_fault::short_write &&
						  fault != io_fault::interrupted_write) ||
						 call == io_call::data_sync ||
						 (call == io_call::rename && !crash));
					flatfile_authority_lock lock;
					assert(lock.acquire(root.string(), &error));
					flatfile_accounting_record retained;
					retained.result = { 99 };
					assert(flatfile_accounting_lookup(root.string(), lock,
									  value.command, &retained,
									  &error) ==
					       (before_journal ? status::not_found : status::ok));
					assert(read(root / "domains/value") ==
					       std::vector<uint8_t>(
						       2, static_cast<uint8_t>(!before_journal)));
					if (before_journal)
					{
						assert(retained.result ==
						       std::vector<uint8_t>{ 99 });
						// Retry this same ID only after proving the original bundle absent.
						operations.resize(1);
						assert(flatfile_accounting_test_access::stage(
							       root.string(), lock, value,
							       &operations, &error) == status::ok);
						assert(flatfile_accounting_test_access::commit(
							       root.string(), lock, operations,
							       &error) ==
						       flatfile_authority_transaction_result::ok);
					}
					const auto index =
						read(root / "economic-evidence/bucket-01.eai");
					const auto segment =
						read(root / "economic-evidence/bucket-01-0.eas");
					assert(flatfile_accounting_lookup(root.string(), lock,
									  value.command, &retained,
									  &error) == status::ok);
					assert(retained.result == value.result &&
					       retained.plan == value.plan &&
					       retained.durable_revision ==
						       value.durable_revision &&
					       retained.failure_stage == value.failure_stage);
					operations.clear();
					assert(flatfile_accounting_test_access::stage(
						       root.string(), lock, value, &operations,
						       &error) == status::already_exists);
					assert(read(root / "economic-evidence/bucket-01.eai") ==
						       index &&
					       read(root / "economic-evidence/bucket-01-0.eas") ==
						       segment &&
					       read(root / "domains/value") ==
						       std::vector<uint8_t>({ 1, 1 }) &&
					       !fs::exists(
						       root /
						       "domains/.critical-authority-transaction"));
				}
	assert(cases == 85);
	printf("flatfile accounting: %zu commit/recovery write/sync/rename/remove failure and process-exit cases passed\n",
	       cases);
}

void qualify_storage_guards(const fs::path &root)
{
	provision(root);
	const std::string root_name = root.string();
	const auto journal = root / "domains/.critical-authority-transaction";
	const auto index = root / "economic-evidence/bucket-01.eai";
	const auto segment = root / "economic-evidence/bucket-01-0.eas";
	std::string error;
	{
		flatfile_authority_lock lock;
		// The same object must remain usable after acquisition allocation fails.
		allocation_size = 0;
		allocation_reached = false;
		fail_allocation = true;
		errno = 0;
		const bool acquired = lock.acquire(root_name, &error);
		const int acquire_errno = errno;
		fail_allocation = false;
		assert(allocation_reached && !acquired && acquire_errno == ENOMEM);
		assert(lock.acquire(root_name, &error));
		std::vector<flatfile_authority_operation> operations;
		assert(flatfile_accounting_test_access::initialize(
			       root_name, lock, id(1), 1, &operations, &error) == status::ok);
		// A one-byte vector allocation is the journal encoder's first number,
		// after path allocation and the missing-journal check have succeeded.
		allocation_size = 1;
		allocation_reached = false;
		fail_allocation = true;
		errno = 0;
		const auto allocation_result = flatfile_accounting_test_access::commit(
			root_name, lock, operations, &error);
		const int encode_errno = errno;
		fail_allocation = false;
		assert(allocation_reached &&
		       allocation_result == flatfile_authority_transaction_result::io_error &&
		       encode_errno == ENOMEM);
		assert(!fs::exists(journal) && !fs::exists(index));
		assert(flatfile_accounting_test_access::commit(root_name, lock, operations,
							       &error) ==
		       flatfile_authority_transaction_result::ok);

		auto first = record(60001);
		operations = { { flatfile_authority_store::domains,
				 flatfile_authority_operation_kind::write,
				 "guard-domain",
				 { 1 } } };
		assert(flatfile_accounting_test_access::stage(root_name, lock, first, &operations,
							      &error) == status::ok);
		const auto original_index = read(index);
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL", "1", 1);
		assert(flatfile_accounting_test_access::commit(root_name, lock, operations,
							       &error) ==
		       flatfile_authority_transaction_result::io_error);
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL");
		const auto pending = read(journal);
		assert(!pending.empty() && read(index) == original_index && !fs::exists(segment));
		const std::vector<flatfile_authority_operation> replacement = {
			{ flatfile_authority_store::domains,
			  flatfile_authority_operation_kind::write,
			  "guard-domain",
			  { 2 } }
		};
		assert(flatfile_accounting_test_access::commit(root_name, lock, replacement,
							       &error) ==
		       flatfile_authority_transaction_result::invalid);
		assert(read(journal) == pending && read(index) == original_index &&
		       !fs::exists(segment) && !fs::exists(root / "domains/guard-domain"));
		flatfile_accounting_record retained;
		assert(flatfile_accounting_lookup(root_name, lock, first.command, &retained,
						  &error) == status::ok);
		assert(retained.plan == first.plan && retained.result == first.result &&
		       read(root / "domains/guard-domain") == std::vector<uint8_t>{ 1 } &&
		       !fs::exists(journal));

		for (const auto &path : { index, segment })
		{
			const auto alias = root / "hardlink";
			fs::create_hard_link(path, alias);
			std::vector<uint8_t> before, after;
			assert(flatfile_accounting_record_encode(retained, &before) == status::ok);
			assert(flatfile_accounting_lookup(root_name, lock, first.command, &retained,
							  &error) == status::invalid);
			assert(flatfile_accounting_record_encode(retained, &after) == status::ok &&
			       after == before);
			fs::remove(alias);
			assert(flatfile_accounting_lookup(root_name, lock, first.command, &retained,
							  &error) == status::ok);
		}
		auto compound = record(60002);
		economic_accounting_plan plan;
		assert(economic_plan_decode(compound.plan, &plan) == economic_accounting_error::ok);
		critical_operation_id child;
		assert(critical_operation_id_derive(compound.command.operation_id, 1, 1, &child));
		plan.children.push_back({ child, 1, 1, 0, 1 });
		assert(economic_plan_encode(plan, &compound.plan) == economic_accounting_error::ok);
		std::vector<uint8_t> output = { 42 };
		assert(flatfile_accounting_record_encode(compound, &output) == status::invalid &&
		       output == std::vector<uint8_t>{ 42 });
		operations = replacement;
		const auto saved_index = read(index), saved_segment = read(segment);
		assert(flatfile_accounting_test_access::stage(
			       root_name, lock, compound, &operations, &error) == status::invalid);
		assert(operations.size() == 1 &&
		       operations[0].filename == replacement[0].filename &&
		       operations[0].bytes == replacement[0].bytes && read(index) == saved_index &&
		       read(segment) == saved_segment && !fs::exists(journal));
	}
	// A hardlinked lock must also fail before entering the authority region.
	const auto alias = root / "lock-alias";
	fs::create_hard_link(root / "domains/.critical-authority.lock", alias);
	flatfile_authority_lock lock;
	assert(!lock.acquire(root_name, &error));
	fs::remove(alias);
	assert(lock.acquire(root_name, &error));
	puts("flatfile accounting: pending journal, allocation recovery, hardlink and child-plan guards passed");
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const fs::path root = argv[1];
	qualify_storage_guards(root / "storage-guards");
	provision(root);
	std::string error;
	auto first = record(100);
	std::vector<uint8_t> encoded;
	assert(flatfile_accounting_record_encode(first, &encoded) == status::ok);
	flatfile_accounting_record decoded;
	assert(flatfile_accounting_record_decode(encoded, &decoded) == status::ok &&
	       decoded.plan == first.plan && decoded.result == first.result);
	for (size_t at : { size_t(0), size_t(8), size_t(12), size_t(20), encoded.size() - 1 })
	{
		auto damaged = encoded;
		damaged[at] ^= 1;
		assert(flatfile_accounting_record_decode(damaged, &decoded) == status::invalid &&
		       decoded.plan == first.plan);
	}
	assert(flatfile_accounting_record_decode(
		       std::span<const uint8_t>(encoded).first(encoded.size() - 1), &decoded) ==
	       status::invalid);
	auto invalid = first;
	invalid.result_code = ENOSPC;
	auto preserved = encoded;
	assert(flatfile_accounting_record_encode(invalid, &encoded) == status::invalid &&
	       encoded == preserved);
	invalid = first;
	invalid.plan.clear();
	assert(flatfile_accounting_record_encode(invalid, &encoded) == status::invalid);
	invalid = first;
	++invalid.command.accepted_at_usec;
	assert(flatfile_accounting_record_encode(invalid, &encoded) ==
	       status::ok); // Exact admission retained independently of policy binding.
	auto rejected = record(101, true);
	assert(flatfile_accounting_record_encode(rejected, &encoded) == status::ok);
	assert(flatfile_accounting_record_decode(encoded, &decoded) == status::ok &&
	       decoded.plan.empty() && decoded.result_code == ENOSPC &&
	       decoded.failure_stage == rejected.failure_stage);
	{
		auto damaged = encoded;
		// Stage is at payload offset 24; recompute the checksum to test validation.
		damaged[48 + 25] = 0x80;
		auto checksum = hash(std::span<const uint8_t>(damaged).subspan(48));
		std::copy(checksum.begin(), checksum.end(), damaged.begin() + 16);
		assert(flatfile_accounting_record_decode(damaged, &decoded) == status::invalid &&
		       decoded.failure_stage == rejected.failure_stage);
		auto invalid_stage = rejected;
		invalid_stage.failure_stage = static_cast<critical_failure_stage>(0x8000);
		const auto saved = encoded;
		assert(flatfile_accounting_record_encode(invalid_stage, &encoded) ==
			       status::invalid &&
		       encoded == saved);
		invalid_stage = first;
		invalid_stage.failure_stage = rejected.failure_stage;
		assert(flatfile_accounting_record_encode(invalid_stage, &encoded) ==
			       status::invalid &&
		       encoded == saved);
	}
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), &error));
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::invalid);
		std::vector<flatfile_authority_operation> operations;
		assert(flatfile_accounting_test_access::initialize(
			       root.string(), lock, id(1), 1, &operations, &error) == status::ok);
		assert(flatfile_authority_transaction_commit_operations(root.string(), lock,
									operations, &error) ==
		       flatfile_authority_transaction_result::invalid);
		assert(flatfile_accounting_test_access::commit(root.string(), lock, operations,
							       &error) ==
		       flatfile_authority_transaction_result::ok);
		operations.clear();
		assert(flatfile_accounting_test_access::initialize(root.string(), lock, id(1), 1,
								   &operations, &error) ==
		       status::already_exists);
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::not_found);
		for (auto &value : { first, rejected })
		{
			operations.clear();
			assert(flatfile_accounting_test_access::stage(root.string(), lock, value,
								      &operations,
								      &error) == status::ok &&
			       operations.size() == 2);
			assert(flatfile_authority_transaction_commit_operations(
				       root.string(), lock, operations, &error) ==
			       flatfile_authority_transaction_result::invalid);
			assert(flatfile_accounting_test_access::commit(root.string(), lock,
								       operations, &error) ==
			       flatfile_authority_transaction_result::ok);
		}
		operations.clear();
		assert(flatfile_accounting_test_access::stage(root.string(), lock, first,
							      &operations,
							      &error) == status::already_exists &&
		       operations.empty());
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::ok &&
		       decoded.result == first.result);
		auto changed = first.command;
		++changed.accepted_at_usec;
		assert(flatfile_accounting_lookup(root.string(), lock, changed, &decoded, &error) ==
		       status::conflict);
		// Retained lookup has no dependency on a bounded player's hot receipts.
		for (uint32_t sequence = 102; sequence < 620; ++sequence)
		{
			auto value = record(sequence, true);
			operations.clear();
			assert(flatfile_accounting_test_access::stage(root.string(), lock, value,
								      &operations,
								      &error) == status::ok);
			assert(flatfile_accounting_test_access::commit(root.string(), lock,
								       operations, &error) ==
			       flatfile_authority_transaction_result::ok);
		}
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::ok);
		// Thirty domain images plus two accounting images fit the real boundary.
		operations.clear();
		for (size_t i = 0; i < 30; ++i)
			operations.push_back({ flatfile_authority_store::domains,
					       flatfile_authority_operation_kind::write,
					       "domain-" + std::to_string(i),
					       { 1 } });
		auto maximum = record(700);
		assert(flatfile_accounting_test_access::stage(root.string(), lock, maximum,
							      &operations, &error) == status::ok &&
		       operations.size() == 32);
		assert(flatfile_accounting_test_access::commit(root.string(), lock, operations,
							       &error) ==
		       flatfile_authority_transaction_result::ok);
		operations.resize(31);
		assert(flatfile_accounting_test_access::stage(root.string(), lock, record(701),
							      &operations,
							      &error) == status::capacity);
	}
	// Real process exit after each publication image, then lock/recovery/readback.
	for (unsigned boundary = 1; boundary <= 3; ++boundary)
	{
		const auto value = record(800 + boundary);
		const pid_t child = fork();
		assert(child >= 0);
		if (!child)
		{
			flatfile_authority_lock lock;
			assert(lock.acquire(root.string(), &error));
			std::vector<flatfile_authority_operation> operations = {
				{ flatfile_authority_store::domains,
				  flatfile_authority_operation_kind::write,
				  "crash-domain",
				  { static_cast<uint8_t>(boundary) } }
			};
			assert(flatfile_accounting_test_access::stage(root.string(), lock, value,
								      &operations,
								      &error) == status::ok);
			setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION",
			       std::to_string(boundary).c_str(), 1);
			assert(flatfile_accounting_test_access::commit(root.string(), lock,
								       operations, &error) ==
			       flatfile_authority_transaction_result::io_error);
			_exit(77);
		}
		int outcome = 0;
		assert(waitpid(child, &outcome, 0) == child && WIFEXITED(outcome) &&
		       WEXITSTATUS(outcome) == 77);
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), &error));
		assert(flatfile_accounting_lookup(root.string(), lock, value.command, &decoded,
						  &error) == status::ok);
		assert(read(root / "domains/crash-domain") ==
		       std::vector<uint8_t>{ static_cast<uint8_t>(boundary) });
		assert(!fs::exists(root / "domains/.critical-authority-transaction"));
	}
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), &error));
		const auto index_path = root / "economic-evidence/bucket-01.eai";
		const auto segment_path = root / "economic-evidence/bucket-01-0.eas";
		const auto index = read(index_path), segment = read(segment_path);
		for (const auto &path : { index_path, segment_path })
		{
			auto original = read(path), corrupt = original;
			corrupt.back() ^= 1;
			write(path, corrupt);
			assert(flatfile_accounting_lookup(root.string(), lock, first.command,
							  &decoded, &error) == status::invalid);
			write(path, original);
			fs::rename(path, path.string() + ".held");
			assert(flatfile_accounting_lookup(root.string(), lock, first.command,
							  &decoded, &error) != status::ok);
			fs::rename(path.string() + ".held", path);
		}
		// An orphaned next segment exposes a stale otherwise-valid index.
		write(root / "economic-evidence/bucket-01-1.eas", segment);
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::invalid);
		fs::remove(root / "economic-evidence/bucket-01-1.eas");
		// Restoring an older index cannot make later IDs absent and executable.
		auto unsupported = index;
		unsupported[8] = 2;
		write(index_path, unsupported);
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::invalid);
		auto excessive = index;
		for (size_t byte = 0; byte < 4; ++byte)
			excessive[128 + byte] = static_cast<uint8_t>(
				FLATFILE_ACCOUNTING_BUCKET_SEGMENTS >> (8 * byte));
		auto checksum = hash(std::span<const uint8_t>(excessive).subspan(48));
		std::copy(checksum.begin(), checksum.end(), excessive.begin() + 16);
		write(index_path, excessive);
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::invalid);
		write(index_path, index);
		write(root / "domains/.critical-authority-transaction", { 1, 2, 3 });
		flatfile_authority_lock missing;
		const auto saved_result = decoded.result;
		assert(flatfile_accounting_lookup(root.string(), missing, first.command, &decoded,
						  &error) == status::invalid);
		assert(flatfile_accounting_lookup((root / "wrong-root").string(), lock,
						  first.command, &decoded,
						  &error) == status::invalid);
		assert(decoded.result == saved_result &&
		       read(root / "domains/.critical-authority-transaction") ==
			       std::vector<uint8_t>({ 1, 2, 3 }));

		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::invalid);
		fs::remove(root / "domains/.critical-authority-transaction");
		auto later = record(900);
		std::vector<flatfile_authority_operation> operations;
		assert(flatfile_accounting_test_access::stage(root.string(), lock, later,
							      &operations, &error) == status::ok);
		assert(flatfile_accounting_test_access::commit(root.string(), lock, operations,
							       &error) ==
		       flatfile_authority_transaction_result::ok);
		auto current = read(index_path);
		write(index_path, index);
		assert(flatfile_accounting_lookup(root.string(), lock, later.command, &decoded,
						  &error) == status::invalid);
		write(index_path, current);
		assert(flatfile_accounting_lookup(root.string(), lock, later.command, &decoded,
						  &error) == status::ok);
	}
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), &error));
		const auto sealed = read(root / "economic-evidence/bucket-01-0.eas");
		for (uint32_t sequence = 2000; sequence < 2025; ++sequence)
		{
			auto value = large_record(sequence);
			std::vector<flatfile_authority_operation> operations;
			assert(flatfile_accounting_test_access::stage(root.string(), lock, value,
								      &operations,
								      &error) == status::ok);
			assert(flatfile_accounting_test_access::commit(root.string(), lock,
								       operations, &error) ==
			       flatfile_authority_transaction_result::ok);
		}
		assert(fs::exists(root / "economic-evidence/bucket-01-1.eas"));
		const auto sealed_after = read(root / "economic-evidence/bucket-01-0.eas");
		assert(sealed_after.size() > sealed.size());
		const auto value = large_record(2024);
		assert(flatfile_accounting_lookup(root.string(), lock, value.command, &decoded,
						  &error) == status::ok);
		assert(decoded.result == value.result && decoded.result.size() == 4096 &&
		       decoded.failure_stage == value.failure_stage);
		assert(flatfile_accounting_lookup(root.string(), lock, first.command, &decoded,
						  &error) == status::ok);
		auto next = large_record(2025);
		std::vector<flatfile_authority_operation> operations;
		assert(flatfile_accounting_test_access::stage(root.string(), lock, next,
							      &operations, &error) == status::ok);
		assert(flatfile_accounting_test_access::commit(root.string(), lock, operations,
							       &error) ==
		       flatfile_authority_transaction_result::ok);
		assert(read(root / "economic-evidence/bucket-01-0.eas") == sealed_after);
	}
	// Exact journal-byte headroom, including all framing, is checked before
	// copying the caller's domain bundle. The large image is never published.
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(root.string(), &error));
		auto value = record(1900);
		std::vector<flatfile_authority_operation> probe;
		assert(flatfile_accounting_test_access::stage(root.string(), lock, value, &probe,
							      &error) == status::ok);
		size_t accounting_bytes = 0;
		for (const auto &operation : probe)
			accounting_bytes += 8 + operation.filename.size() + operation.bytes.size();
		const std::string name = "byte-boundary";
		const auto budget = flatfile_authority_transaction_maximum_bytes - 50 - 8 -
				    name.size() - accounting_bytes;
		std::vector<flatfile_authority_operation> exact = {
			{ flatfile_authority_store::domains,
			  flatfile_authority_operation_kind::write, name,
			  std::vector<uint8_t>(budget, 1) }
		};
		assert(flatfile_accounting_test_access::stage(root.string(), lock, value, &exact,
							      &error) == status::ok &&
		       exact.size() == 3);
		exact.clear();
		exact.push_back({ flatfile_authority_store::domains,
				  flatfile_authority_operation_kind::write, name,
				  std::vector<uint8_t>(budget + 1, 1) });
		assert(flatfile_accounting_test_access::stage(root.string(), lock, value, &exact,
							      &error) == status::capacity &&
		       exact.size() == 1 && exact[0].bytes.size() == budget + 1);
	}
	// A full retained index refuses growth but never evicts old operation IDs.
	const auto capacity = root / "capacity";
	provision(capacity);
	{
		flatfile_authority_lock lock;
		assert(lock.acquire(capacity.string(), &error));
		std::vector<uint8_t> records, entries;
		for (uint32_t sequence = 1; sequence <= FLATFILE_ACCOUNTING_BUCKET_RECORDS;
		     ++sequence)
		{
			auto value = record(sequence, true);
			std::vector<uint8_t> bytes;
			assert(flatfile_accounting_record_encode(value, &bytes) == status::ok);
			raw(entries, value.command.operation_id.bytes);
			raw(entries, hash(bytes));
			number(entries, 0, 4);
			number(entries, records.size(), 4);
			number(entries, bytes.size(), 4);
			number(entries, 0, 4);
			raw(records, bytes);
		}
		std::vector<uint8_t> index, segment;
		raw(index, id(1).bytes);
		number(index, 1, 4);
		number(index, FLATFILE_ACCOUNTING_BUCKET_RECORDS, 4);
		number(index, records.size(), 8);
		raw(index, entries);
		raw(segment, id(1).bytes);
		number(segment, 1, 4);
		number(segment, 0, 4);
		number(segment, FLATFILE_ACCOUNTING_BUCKET_RECORDS, 4);
		number(segment, 0, 4);
		raw(segment, records);
		write(capacity / "economic-evidence/bucket-01.eai", envelope("DURECI1", index));
		write(capacity / "economic-evidence/bucket-01-0.eas", envelope("DURECS1", segment));
		auto old = record(1, true);
		std::vector<flatfile_authority_operation> operations;
		assert(flatfile_accounting_lookup(capacity.string(), lock, old.command, &decoded,
						  &error) == status::ok);
		assert(flatfile_accounting_test_access::stage(capacity.string(), lock, record(5000),
							      &operations,
							      &error) == status::capacity &&
		       operations.empty());
		assert(flatfile_accounting_lookup(capacity.string(), lock, old.command, &decoded,
						  &error) == status::ok);
	}
	qualify_io_faults(root / "io-faults");
	puts("flatfile accounting: canonical records, private append, retained replay, bounded bundle and crash recovery passed");
}
