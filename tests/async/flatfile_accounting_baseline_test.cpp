#include "flatfile/flatfile_accounting_baseline.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_store.h"
#include <cassert>
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <new>
#include <openssl/sha.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;
using status = flatfile_accounting_status;
using tx = flatfile_authority_transaction_result;
using bytes = std::vector<uint8_t>;
using ops = std::vector<flatfile_authority_operation>;
size_t allocation_target = 0, allocation_seen = 0;
extern "C" void *__real__Znwm(size_t);
extern "C" void *__real__Znam(size_t);
extern "C" void *__wrap__Znwm(size_t count)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znwm(count);
}
extern "C" void *__wrap__Znam(size_t count)
{
	if (allocation_target && ++allocation_seen == allocation_target)
		throw std::bad_alloc();
	return __real__Znam(count);
}
class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto bucket =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto initialize = &flatfile_accounting_baseline_storage::initialize;
	static constexpr auto stage = &flatfile_accounting_baseline_storage::stage;
	static constexpr auto commit = &flatfile_accounting_storage::commit;
};
using access_store = flatfile_accounting_test_access;
critical_operation_id id(uint64_t n)
{
	critical_operation_id result = {};
	for (size_t i = 0; i < 8; ++i)
		result.bytes[i] = static_cast<uint8_t>(n >> (8 * i));
	return result;
}
economic_digest digest(uint8_t n)
{
	economic_digest result = {};
	result[0] = n;
	return result;
}
economic_account_key opening()
{
	return { id(1), economic_account_kind::opening, 9001, 0 };
}
economic_baseline_batch batch(uint64_t sequence = 0)
{
	economic_baseline_batch result;
	result.lineage = id(1);
	result.epoch = id(2);
	result.preparation_id = id(3);
	result.actor_id = 7;
	result.batch_index = sequence;
	result.opening_account = opening();
	result.boundary_digest = digest(11);
	result.coverage_digest = digest(12);
	for (uint64_t n = 1; n <= 16; ++n)
	{
		result.holdings.push_back({ { id(1), economic_account_kind::wallet, n, 0 },
					    { 1, 2, 3, 4 },
					    n,
					    digest(1) });
		result.items.push_back({ { 100 + n,
					   { { item_owner_type::player, 7, 0 },
					     100 + n,
					     0,
					     1,
					     item_custody_state::active } },
					 digest(2) });
	}
	return result;
}
std::optional<economic_prepared_baseline> prepare(const economic_baseline_batch &value)
{
	std::optional<economic_prepared_baseline> result;
	assert(economic_baseline_prepare(value, &result) == economic_accounting_error::ok);
	return result;
}
critical_command command(const economic_prepared_baseline &value)
{
	critical_command result;
	assert(economic_baseline_command_build(value, 123456, &result) ==
	       economic_accounting_error::ok);
	return result;
}
bytes read(const fs::path &file)
{
	std::ifstream stream(file, std::ios::binary);
	assert(stream);
	return { std::istreambuf_iterator<char>(stream), {} };
}
void write(const fs::path &file, const bytes &value)
{
	assert(flatfile_atomic_write(file.parent_path().string(), file.filename().string(), value,
				     nullptr));
}
struct fixture
{
	std::string root;
	flatfile_authority_lock lock;
	explicit fixture(const fs::path &path)
		: root(path.string())
	{
		for (auto p : { path, path / "domains", path / "economic-evidence" })
		{
			fs::create_directories(p);
			fs::permissions(p, fs::perms::owner_all);
		}
		assert(lock.acquire(root, nullptr));
	}
	flatfile_economic_control control()
	{
		flatfile_economic_control value;
		assert(!flatfile_economic_control_read(root, lock, &value, nullptr));
		return value;
	}
	void commit(ops &changes)
	{
		assert(flatfile_authority_transaction_commit_operations(root, lock, changes,
									nullptr) == tx::invalid);
		assert(access_store::commit(root, lock, changes, nullptr) == tx::ok);
		changes.clear();
	}
	void epoch(uint64_t value = 2, uint64_t ordinal = 1, uint64_t predecessor = 0)
	{
		flatfile_economic_epoch next{ id(value), id(predecessor), id(99), ordinal,
					      1,	 digest(9) };
		ops changes;
		assert(!access_store::epoch(root, lock, control().revision, next, &changes,
					    nullptr));
		commit(changes);
	}
	void ensure(const critical_command &cmd)
	{
		auto c = control();
		size_t slot = cmd.operation_id.bytes[0];
		if (c.evidence_initialized[slot / 8] & (1U << (slot % 8)))
			return;
		ops changes;
		assert(!access_store::bucket(root, lock, c.revision, slot, id(98), &changes,
					     nullptr));
		commit(changes);
	}
	void initialize(uint64_t value = 2)
	{
		ops changes;
		assert(access_store::initialize(root, lock, id(1), id(value), opening(), id(97),
						&changes, nullptr) == status::ok);
		assert(changes.size() == 17);
		commit(changes);
	}
	void setup(bool book = true)
	{
		ops changes;
		assert(!access_store::bootstrap(root, lock, id(1), id(99), &changes, nullptr));
		commit(changes);
		epoch();
		ensure(command(*prepare(batch())));
		if (book)
			initialize();
		write(fs::path(root) / "domains" / "native-sentinel", { 1, 2, 3, 4 });
	}
	void stored(const critical_command &cmd, uint64_t revision)
	{
		flatfile_accounting_record record;
		bytes witness;
		assert(flatfile_accounting_baseline_lookup(root, lock, cmd, &record, &witness,
							   nullptr) == status::ok);
		assert(record.failure_stage == critical_failure_stage::none &&
		       record.durable_revision == revision &&
		       critical_command_equal(record.command, cmd));
		std::optional<economic_prepared_baseline> value;
		assert(economic_baseline_decode(witness, &value) == economic_accounting_error::ok);
		economic_accounting_plan plan;
		assert(economic_baseline_command_plan(cmd, *value, &plan) ==
		       economic_accounting_error::ok);
		bytes encoded;
		assert(economic_plan_encode(plan, &encoded) == economic_accounting_error::ok &&
		       encoded == record.plan);
		assert(read(fs::path(root) / "domains" / "native-sentinel") ==
		       bytes({ 1, 2, 3, 4 }));
	}
};
void basic(const fs::path &root)
{
	fixture f(root);
	f.setup();
	auto prepared = prepare(batch());
	auto cmd = command(*prepared);
	ops changes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
		       status::ok &&
	       changes.size() == 20);
	f.commit(changes);
	f.stored(cmd, 1);
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
		       status::already_exists &&
	       changes.empty());
	for (int variant = 0; variant < 3; ++variant)
	{
		auto input = batch(variant + 1);
		if (variant == 0)
			input.items.clear();
		if (variant == 1)
			input.holdings.clear();
		if (variant == 2)
		{
			input.items.clear();
			input.holdings.resize(1);
			input.holdings[0].account.kind = economic_account_kind::treasury;
		}
		auto other = prepare(input);
		auto attempt = command(*other);
		f.ensure(attempt);
		assert(access_store::stage(f.root, f.lock, attempt, *other, &changes, nullptr) ==
			       status::conflict &&
		       changes.empty());
	}
	auto changed = cmd;
	changed.accepted_at_usec++;
	assert(access_store::stage(f.root, f.lock, changed, *prepared, &changes, nullptr) ==
	       status::conflict);
	auto empty = batch(9);
	empty.holdings.clear();
	empty.items.clear();
	auto zero = prepare(empty);
	auto zcmd = command(*zero);
	f.ensure(zcmd);
	assert(access_store::stage(f.root, f.lock, zcmd, *zero, &changes, nullptr) == status::ok &&
	       changes.size() == 4);
	f.commit(changes);
	f.stored(zcmd, 2);
	f.stored(cmd, 1);
	auto different = empty;
	different.opening_account.authority_id++;
	different.batch_index++;
	auto bad = prepare(different);
	auto bcmd = command(*bad);
	f.ensure(bcmd);
	assert(access_store::stage(f.root, f.lock, bcmd, *bad, &changes, nullptr) ==
	       status::invalid);
	// Choose a fresh ID in the same common receipt bucket, then merge new
	// identities into all existing reservation buckets without losing history.
	auto separate = batch();
	critical_operation_id candidate;
	for (uint64_t sequence = 32;; ++sequence)
	{
		assert(critical_operation_id_derive(id(3), ECONOMIC_BASELINE_OPERATION_DOMAIN,
						    sequence, &candidate));
		if (candidate.bytes[0] == cmd.operation_id.bytes[0])
		{
			separate.batch_index = sequence;
			break;
		}
		assert(sequence < 10000);
	}
	for (auto &holding : separate.holdings)
		holding.account.authority_id += 1000;
	for (auto &item : separate.items)
	{
		item.snapshot.uid += 1000;
		item.snapshot.position.root_uid += 1000;
	}
	auto distinct = prepare(separate);
	auto dcmd = command(*distinct);
	assert(access_store::stage(f.root, f.lock, dcmd, *distinct, &changes, nullptr) ==
		       status::ok &&
	       changes.size() == 20);
	f.commit(changes);
	f.stored(dcmd, 3);
	f.stored(cmd, 1);
	f.stored(zcmd, 2);
	f.epoch(22, 2, 2);
	f.initialize(22);
	auto next = batch();
	next.epoch = id(22);
	next.preparation_id = id(33);
	auto n = prepare(next);
	auto ncmd = command(*n);
	f.ensure(ncmd);
	assert(access_store::stage(f.root, f.lock, ncmd, *n, &changes, nullptr) == status::ok);
	f.commit(changes);
	f.stored(ncmd, 1);
	f.stored(cmd, 1);
}
void corruption(const fs::path &root)
{
	fixture f(root);
	f.setup();
	auto prepared = prepare(batch());
	auto cmd = command(*prepared);
	ops changes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
	       status::ok);
	f.commit(changes);
	flatfile_accounting_record record;
	record.durable_revision = 999;
	bytes source{ 77 };
	auto rejected = [&]
	{
		assert(flatfile_accounting_baseline_lookup(f.root, f.lock, cmd, &record, &source,
							   nullptr) == status::invalid);
		assert(record.durable_revision == 999 && source == bytes{ 77 });
	};
	for (const auto &file : fs::directory_iterator(root / "economic-evidence"))
	{
		auto ext = file.path().extension();
		if (ext != ".eab" && ext != ".ebi" && ext != ".ebc")
			continue;
		auto original = read(file.path());
		auto bad = original;
		bad.back() ^= 1;
		write(file.path(), bad);
		rejected();
		fs::remove(file.path());
		rejected();
		write(file.path(), original);
		const auto alias = root / "baseline-hardlink";
		fs::create_hard_link(file.path(), alias);
		rejected();
		fs::remove(alias);
	}
	// Even coherent checksums cannot substitute a different reservation owner.
	fs::path index, head;
	for (const auto &file : fs::directory_iterator(root / "economic-evidence"))
	{
		if (file.path().filename().string().ends_with("0.ebi"))
			index = file.path();
		if (file.path().extension() == ".ebc")
			head = file.path();
	}
	auto index_before = read(index), head_before = read(head), edited = index_before,
	     edited_head = head_before;
	edited[88 + 16] ^= 1;
	auto rehash = [](bytes &value)
	{ SHA256(value.data() + 48, value.size() - 48, value.data() + 16); };
	rehash(edited);
	SHA256(edited.data(), edited.size(), edited_head.data() + 144);
	rehash(edited_head);
	write(index, edited);
	write(head, edited_head);
	rejected();
	write(index, index_before);
	write(head, head_before);
	f.stored(cmd, 1);
}
void failure_stage_tamper(const fs::path &root)
{
	fixture f(root);
	f.setup();
	auto prepared = prepare(batch());
	const auto cmd = command(*prepared);
	ops changes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
	       status::ok);
	auto segment = std::find_if(changes.begin(), changes.end(),
				    [](const auto &op) { return op.filename.ends_with(".eas"); });
	auto index = std::find_if(changes.begin(), changes.end(),
				  [](const auto &op) { return op.filename.ends_with(".eai"); });
	assert(segment != changes.end() && index != changes.end());
	// The fresh receipt bucket has exactly one DURECR2 record after its
	// 48-byte envelope and 32-byte segment header. Alter only its stage,
	// then rebuild every checksum so integrity checks cannot mask this case.
	constexpr size_t record_offset = 80, stage_offset = record_offset + 48 + 24;
	assert(segment->bytes.size() > stage_offset + 2 && index->bytes.size() == 80 + 64);
	assert(std::equal(segment->bytes.begin() + record_offset,
			  segment->bytes.begin() + record_offset + 7, "DURECR2"));
	const auto stage = critical_failure_stage::coin_source_wallet_revision;
	assert(critical_failure_stage_valid(stage));
	segment->bytes[stage_offset] = static_cast<uint16_t>(stage) & 255;
	segment->bytes[stage_offset + 1] = static_cast<uint16_t>(stage) >> 8;
	SHA256(segment->bytes.data() + record_offset + 48,
	       segment->bytes.size() - record_offset - 48,
	       segment->bytes.data() + record_offset + 16);
	SHA256(segment->bytes.data() + record_offset, segment->bytes.size() - record_offset,
	       index->bytes.data() + 80 + 16);
	SHA256(segment->bytes.data() + 48, segment->bytes.size() - 48, segment->bytes.data() + 16);
	SHA256(index->bytes.data() + 48, index->bytes.size() - 48, index->bytes.data() + 16);
	f.commit(changes);
	flatfile_accounting_record output;
	output.durable_revision = 999;
	output.result = { 42 };
	bytes witness{ 77 };
	assert(flatfile_accounting_baseline_lookup(f.root, f.lock, cmd, &output, &witness,
						   nullptr) == status::invalid);
	assert(output.durable_revision == 999 && output.result == bytes{ 42 } &&
	       witness == bytes{ 77 });
	assert(read(root / "domains/native-sentinel") == bytes({ 1, 2, 3, 4 }));
	std::cout
		<< "baseline receipt: coherently checksummed valid non-none failure stage refused\n";
}
void capacity_and_orphans(const fs::path &root)
{
	fixture f(root);
	f.setup();
	auto prepared = prepare(batch());
	auto cmd = command(*prepared);
	ops changes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
	       status::ok);
	// A caller bundle that would exceed the shared limit cannot be partially
	// changed. Duplicate target files also refuse before any commit.
	ops full;
	for (size_t n = 0; n < 13; ++n)
		full.push_back({ flatfile_authority_store::domains,
				 flatfile_authority_operation_kind::write,
				 "extra-" + std::to_string(n),
				 { 1 } });
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &full, nullptr) ==
		       status::capacity &&
	       full.size() == 13);
	ops duplicate{ changes.front() };
	auto original = duplicate.front().bytes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &duplicate, nullptr) ==
		       status::conflict &&
	       duplicate.size() == 1 && duplicate.front().bytes == original);
	const auto witness = std::find_if(changes.begin(), changes.end(), [](const auto &op)
					  { return op.filename.ends_with(".eab"); });
	assert(witness != changes.end());
	auto path = root / "economic-evidence" / witness->filename;
	write(path, witness->bytes);
	ops rejected;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &rejected, nullptr) ==
		       status::conflict &&
	       rejected.empty());
	fs::remove(path);
	// Construct a checksum-correct saturated bucket to exercise the persisted
	// capacity guard. Entries are synthetic reservations, not native holdings.
	fs::path index, head;
	for (const auto &file : fs::directory_iterator(root / "economic-evidence"))
	{
		if (file.path().filename().string().ends_with("0.ebi"))
			index = file.path();
		if (file.path().extension() == ".ebc")
			head = file.path();
	}
	auto empty_index = read(index), empty_head = read(head), saturated = empty_index,
	     book = empty_head;
	saturated.resize(88 + 32 * FLATFILE_BASELINE_BUCKET_RECORDS);
	auto put = [](bytes &value, size_t offset, uint64_t number, size_t width)
	{
		for (size_t byte = 0; byte < width; ++byte)
			value[offset + byte] = static_cast<uint8_t>(number >> (8 * byte));
	};
	put(saturated, 12, saturated.size() - 48, 4);
	put(saturated, 84, FLATFILE_BASELINE_BUCKET_RECORDS, 4);
	for (size_t row = 0; row < FLATFILE_BASELINE_BUCKET_RECORDS; ++row)
	{
		put(saturated, 88 + 32 * row, 1, 8);
		put(saturated, 96 + 32 * row, (row + 100000) * 16, 8);
		saturated[104 + 32 * row] = 99;
	}
	SHA256(saturated.data() + 48, saturated.size() - 48, saturated.data() + 16);
	SHA256(saturated.data(), saturated.size(), book.data() + 144);
	SHA256(book.data() + 48, book.size() - 48, book.data() + 16);
	write(index, saturated);
	write(head, book);
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &rejected, nullptr) ==
		       status::capacity &&
	       rejected.empty());
	write(index, empty_index);
	write(head, empty_head);
	changes.clear();
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
	       status::ok);
	f.commit(changes);
	f.stored(cmd, 1);
}
void allocations(const fs::path &root)
{
	fixture f(root);
	f.setup();
	auto prepared = prepare(batch());
	auto cmd = command(*prepared);
	size_t stages = 0, lookups = 0;
	for (size_t target = 1; target < 20000; ++target)
	{
		ops changes{ { flatfile_authority_store::domains,
			       flatfile_authority_operation_kind::write,
			       "extra",
			       { 99 } } };
		allocation_target = target;
		allocation_seen = 0;
		auto result =
			access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr);
		allocation_target = 0;
		if (result == status::ok)
			break;
		assert(result == status::capacity || result == status::io_error);
		assert(changes.size() == 1 && changes[0].filename == "extra" &&
		       changes[0].bytes == bytes{ 99 });
		++stages;
	}
	ops changes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
	       status::ok);
	f.commit(changes);
	for (size_t target = 1; target < 20000; ++target)
	{
		flatfile_accounting_record record;
		record.durable_revision = 999;
		bytes source{ 77 };
		allocation_target = target;
		allocation_seen = 0;
		auto result = flatfile_accounting_baseline_lookup(f.root, f.lock, cmd, &record,
								  &source, nullptr);
		allocation_target = 0;
		if (result == status::ok)
			break;
		assert(result == status::capacity || result == status::io_error);
		assert(record.durable_revision == 999 && source == bytes{ 77 });
		++lookups;
	}
	assert(stages > 100 && stages < 19999 && lookups > 100 && lookups < 19999);
	std::cout << "baseline storage allocation failures: " << stages << " stage, " << lookups
		  << " lookup passed\n";
}
void maximum(const fs::path &root)
{
	fixture f(root);
	f.setup();
	auto input = batch();
	input.holdings.clear();
	input.items.clear();
	for (uint64_t n = 1; n <= ECONOMIC_BASELINE_MAX_HOLDINGS; ++n)
		input.holdings.push_back({ { id(1), economic_account_kind::wallet, n, 0 },
					   { 1, 2, 3, 4 },
					   UINT64_MAX,
					   digest(1) });
	for (uint64_t n = 1; n <= ECONOMIC_ACCOUNTING_MAX_ITEM_WITNESSES; ++n)
		input.items.push_back({ { n,
					  { { item_owner_type::player, 7, 0 },
					    1,
					    n - 1,
					    UINT64_MAX,
					    item_custody_state::active } },
					digest(1) });
	auto prepared = prepare(input);
	auto cmd = command(*prepared);
	ops changes;
	assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
		       status::ok &&
	       changes.size() == 20);
	f.commit(changes);
	f.stored(cmd, 1);
}
void crashes(const fs::path &root, bool initialization)
{
	auto seed = root / "seed";
	{
		fixture f(seed);
		f.setup(!initialization);
	}
	auto prepared = prepare(batch());
	auto cmd = command(*prepared);
	const int count = initialization ? 17 : 20;
	for (int boundary = -1; boundary <= count; ++boundary)
	{
		auto target = root / ("boundary-" + std::to_string(boundary));
		fs::copy(seed, target, fs::copy_options::recursive);
		auto child = fork();
		assert(child >= 0);
		if (!child)
		{
			fixture f(target);
			ops changes;
			auto result = initialization ?
					      access_store::initialize(f.root, f.lock, id(1), id(2),
								       opening(), id(97), &changes,
								       nullptr) :
					      access_store::stage(f.root, f.lock, cmd, *prepared,
								  &changes, nullptr);
			assert(result == status::ok);
			if (boundary < 0)
				setenv("DURIS_FLATFILE_TEST_FAIL_BEFORE_AUTHORITY_COMMIT", "1", 1);
			else if (!boundary)
				setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_JOURNAL", "1",
				       1);
			else
				setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION",
				       std::to_string(boundary).c_str(), 1);
			assert(access_store::commit(f.root, f.lock, changes, nullptr) ==
			       tx::io_error);
			_exit(77);
		}
		int result = 0;
		assert(waitpid(child, &result, 0) == child && WIFEXITED(result) &&
		       WEXITSTATUS(result) == 77);
		fixture f(target);
		if (boundary == 0)
		{
			const auto journal = target / "domains/.critical-authority-transaction";
			const auto pending = read(journal);
			assert(!pending.empty());
			const ops replacement{ { flatfile_authority_store::domains,
						 flatfile_authority_operation_kind::write,
						 "native-sentinel",
						 { 99 } } };
			assert(access_store::commit(f.root, f.lock, replacement, nullptr) ==
			       tx::invalid);
			assert(read(journal) == pending &&
			       read(target / "domains/native-sentinel") == bytes({ 1, 2, 3, 4 }));
		}
		assert(flatfile_authority_transaction_recover(f.root, f.lock, nullptr) == tx::ok);
		ops changes;
		if (initialization)
		{
			if (boundary < 0)
				f.initialize();
			else
				assert(access_store::initialize(f.root, f.lock, id(1), id(2),
								opening(), id(97), &changes,
								nullptr) == status::conflict);
			assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes,
						   nullptr) == status::ok);
			f.commit(changes);
		}
		else if (boundary < 0)
		{
			flatfile_accounting_record record;
			bytes witness;
			assert(flatfile_accounting_baseline_lookup(f.root, f.lock, cmd, &record,
								   &witness,
								   nullptr) == status::not_found);
			assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes,
						   nullptr) == status::ok);
			f.commit(changes);
		}
		f.stored(cmd, 1);
		assert(access_store::stage(f.root, f.lock, cmd, *prepared, &changes, nullptr) ==
			       status::already_exists &&
		       changes.empty());
	}
	std::cout << "baseline " << (initialization ? "initialization" : "batch")
		  << ": before-journal, after-journal and " << count
		  << " after-image restart boundaries passed\n";
}
int main(int argc, char **argv)
{
	assert(argc == 2);
	fs::path root(argv[1]);
	fs::create_directories(root);
	basic(root / "basic");
	capacity_and_orphans(root / "capacity");
	corruption(root / "corrupt");
	failure_stage_tamper(root / "failure-stage");
	maximum(root / "maximum");
	allocations(root / "allocations");
	crashes(root / "init-crashes", true);
	crashes(root / "batch-crashes", false);
	std::cout
		<< "native baseline storage: exact replay, cross-batch duplicates, epoch isolation, coherent corruption, maximum witness and native sentinel preservation passed\n";
}
