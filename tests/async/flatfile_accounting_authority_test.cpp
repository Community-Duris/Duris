#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_store.h"
#include <cassert>
#include <algorithm>
#include <new>
#include <fcntl.h>
#include <cstdarg>
#include <cerrno>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <openssl/sha.h>
#include <sys/wait.h>
#include <unistd.h>

// Fail individual C++ allocations in the linked repository objects. The shared
// library/allocator itself remains real; only calls made while armed are counted.
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
bool count_bucket_reads = false;
bool fail_control_allocation = false, fail_control_io = false, control_fault_seen = false;
std::array<size_t, 256> mapping_reads = {}, native_reads = {};
extern "C" int __real_openat(int, const char *, int, ...);
extern "C" int __wrap_openat(int fd, const char *name, int flags, ...)
{
	mode_t mode = 0;
	if (flags & O_CREAT)
	{
		va_list args;
		va_start(args, flags);
		mode = va_arg(args, int);
		va_end(args);
	}
	if (count_bucket_reads && !(flags & O_CREAT))
	{
		unsigned bucket = 0;
		char extra = 0;
		if (sscanf(name, "mapping-%2x.eam%c", &bucket, &extra) == 1 && bucket < 256)
			++mapping_reads[bucket];
		else if (sscanf(name, "native-%2x.ean%c", &bucket, &extra) == 1 && bucket < 256)
			++native_reads[bucket];
	}
	if (!(flags & O_CREAT) && std::string_view(name) == "authority.eal")
	{
		if (fail_control_io)
		{
			fail_control_io = false;
			control_fault_seen = true;
			errno = EIO;
			return -1;
		}
		if (fail_control_allocation)
		{
			fail_control_allocation = false;
			control_fault_seen = true;
			allocation_target = 1;
			allocation_seen = 0;
		}
	}
	return __real_openat(fd, name, flags, mode);
}
class flatfile_accounting_test_access
{
    public:
	static constexpr auto bootstrap = &flatfile_accounting_authority_storage::bootstrap;
	static constexpr auto initialize_native_bucket =
		&flatfile_accounting_authority_storage::initialize_native_bucket;
	static constexpr auto initialize_evidence_bucket =
		&flatfile_accounting_authority_storage::initialize_evidence_bucket;
	static constexpr auto create_mapping =
		&flatfile_accounting_authority_storage::create_mapping;
	static constexpr auto retire_mapping =
		&flatfile_accounting_authority_storage::retire_mapping;
	static constexpr auto rename_bank = &flatfile_accounting_authority_storage::rename_bank;
	static constexpr auto append_epoch = &flatfile_accounting_authority_storage::append_epoch;
	static constexpr auto select_epoch = &flatfile_accounting_authority_storage::select_epoch;

	static auto commit(const std::string &root, const flatfile_authority_lock &lock,
			   const std::vector<flatfile_authority_operation> &ops, std::string *error)
	{
		return flatfile_accounting_storage::commit(root, lock, ops, error);
	}
};
using metadata_access = flatfile_accounting_test_access;
using bytes = std::vector<uint8_t>;
using ops = std::vector<flatfile_authority_operation>;
namespace fs = std::filesystem;
critical_operation_id id(uint64_t number)
{
	critical_operation_id value = {};
	for (size_t i = 0; i < 8; ++i)
		value.bytes[i] = static_cast<uint8_t>(number >> (8 * i));
	return value;
}
bytes read(const fs::path &path)
{
	std::ifstream in(path, std::ios::binary);
	return { std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>() };
}
void write(const fs::path &path, const bytes &value)
{
	assert(flatfile_atomic_write(path.parent_path().string(), path.filename().string(), value,
				     nullptr));
}
economic_digest hash(std::span<const uint8_t> value)
{
	economic_digest result;
	SHA256(value.data(), value.size(), result.data());
	return result;
}
void number(bytes &out, uint64_t value, size_t width)
{
	for (size_t i = 0; i < width; ++i)
		out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
size_t bank_bucket(const std::string &name)
{
	bytes key;
	number(key, 2, 2);
	number(key, 1, 8);
	number(key, 2, 2);
	key.insert(key.end(), name.begin(), name.end());
	return hash(key)[0];
}
std::string native_name(size_t bucket)
{
	char name[32];
	snprintf(name, sizeof(name), "native-%02zx.ean", bucket);
	return name;
}
void rehash(bytes &encoded)
{
	auto digest = hash(std::span<const uint8_t>(encoded).subspan(48));
	std::copy(digest.begin(), digest.end(), encoded.begin() + 16);
}
void bind_native(bytes &control, size_t bucket, const bytes &index)
{
	auto digest = hash(index);
	std::copy(digest.begin(), digest.end(), control.begin() + 184 + bucket * 32);
	rehash(control);
}
void provision(const fs::path &root)
{
	for (const auto &path : { root, root / "domains", root / "economic-evidence" })
	{
		fs::create_directories(path);
		fs::permissions(path, fs::perms::owner_all);
	}
}
struct fixture
{
	std::string root;
	flatfile_authority_lock lock;
	std::string error;
	explicit fixture(const fs::path &path)
		: root(path.string())
	{
		provision(path);
		assert(lock.acquire(root, &error));
	}
	flatfile_economic_control control()
	{
		flatfile_economic_control value;
		assert(flatfile_economic_control_read(root, lock, &value, &error) == 0);
		return value;
	}
	fs::path file(const std::string &name)
	{
		return fs::path(root) / "economic-evidence" / name;
	}
	void commit(ops &changes)
	{
		assert(flatfile_authority_transaction_commit_operations(root, lock, changes,
									&error) ==
		       flatfile_authority_transaction_result::invalid);
		assert(metadata_access::commit(root, lock, changes, &error) ==
		       flatfile_authority_transaction_result::ok);
		changes.clear();
	}
	void initialize()
	{
		ops changes;
		assert(metadata_access::bootstrap(root, lock, id(1), id(2), &changes, &error) == 0);
		commit(changes);
		assert(metadata_access::bootstrap(root, lock, id(1), id(2), &changes, &error) ==
			       EEXIST &&
		       changes.empty());
		for (size_t bucket = 0; bucket < 256; ++bucket)
		{
			assert(metadata_access::initialize_native_bucket(
				       root, lock, control().revision, bucket, id(3), &changes,
				       &error) == 0);
			commit(changes);
		}
	}
	flatfile_economic_mapping create(economic_account_kind kind, uint64_t context,
					 flatfile_economic_locator locator)
	{
		flatfile_economic_mapping result;
		ops changes;
		assert(metadata_access::create_mapping(root, lock, control().revision, kind,
						       context, locator, id(4), &result, &changes,
						       &error) == 0);
		assert(changes.size() == 3);
		commit(changes);
		return result;
	}
	flatfile_economic_mapping retained(const economic_account_key &key)
	{
		flatfile_economic_mapping result;
		assert(flatfile_economic_mapping_read(root, lock, key, &result, &error) == 0);
		return result;
	}
	flatfile_economic_mapping bank(const std::string &name)
	{
		flatfile_economic_mapping result;
		assert(flatfile_economic_native_lookup(root, lock, economic_account_kind::bank, 1,
						       { 2, 0, name }, &result, &error) == 0);
		return result;
	}
	void rename(const economic_account_key &key, const std::string &name,
		    size_t expected_images)
	{
		ops changes;
		auto before = retained(key);
		assert(metadata_access::rename_bank(root, lock, control().revision, key,
						    before.revision, name, id(5), &changes,
						    &error) == 0);
		assert(changes.size() == expected_images);
		commit(changes);
		auto after = retained(key);
		assert(after.account.authority_id == before.account.authority_id &&
		       after.locator.native_id == before.locator.native_id &&
		       after.creating_operation.bytes == before.creating_operation.bytes &&
		       after.locator.name == name && after.revision == before.revision + 1);
	}
	void retire(const economic_account_key &key)
	{
		ops changes;
		assert(metadata_access::retire_mapping(root, lock, control().revision, key,
						       retained(key).revision, id(6), &changes,
						       &error) == 0);
		commit(changes);
	}
};
flatfile_economic_epoch epoch(uint64_t ordinal, uint64_t identity, uint64_t predecessor = 0)
{
	flatfile_economic_epoch value;
	value.epoch = id(identity);
	value.ordinal = ordinal;
	value.predecessor = id(predecessor);
	value.creating_operation = id(7);
	value.transition_kind = 1;
	value.transition_digest[0] = 42;
	return value;
}
void basic(const fs::path &root)
{
	fixture f(root);
	flatfile_economic_control absent;
	absent.revision = 99;
	assert(flatfile_economic_control_read(f.root, f.lock, &absent, &f.error) == EILSEQ &&
	       absent.revision == 99);
	f.initialize();
	auto initial = f.control();
	assert(initial.next_mapping_id == 1 && initial.epoch_count == 0 && initial.revision == 256);
	ops changes;
	assert(metadata_access::initialize_native_bucket(f.root, f.lock, initial.revision, 0, id(3),
							 &changes, &f.error) == EALREADY);
	assert(metadata_access::initialize_native_bucket(f.root, f.lock, initial.revision - 1, 0,
							 id(3), &changes, &f.error) == ESTALE);
	auto wallet = f.create(economic_account_kind::wallet, 0, { 1, 11, {} });
	auto bank = f.create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	assert(wallet.account.authority_id == 1 && bank.account.authority_id == 2 &&
	       bank.locator.native_id == 2);
	flatfile_economic_mapping preserved = bank;
	assert(metadata_access::create_mapping(
		       f.root, f.lock, f.control().revision, economic_account_kind::bank, 1,
		       { 2, 0, "synthetic" }, id(4), &preserved, &changes, &f.error) == EEXIST &&
	       changes.empty() && preserved.account.authority_id == 2);
	assert(metadata_access::create_mapping(f.root, f.lock, f.control().revision,
					       economic_account_kind::bank, 1, { 2, 55, "forged" },
					       id(4), &preserved, &changes, &f.error) == EINVAL);
	std::array<flatfile_economic_mapping_request, 2> requests = {
		{ { bank.account, bank.locator }, { wallet.account, wallet.locator } }
	};
	flatfile_economic_authority_snapshot snapshot;
	snapshot.lineage_revision = 99;
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(1), id(50), requests, &snapshot,
						&f.error) == ENODATA &&
	       snapshot.lineage_revision == 99);
	assert(metadata_access::append_epoch(f.root, f.lock, f.control().revision, epoch(1, 50),
					     &changes, &f.error) == 0);
	f.commit(changes);
	assert(metadata_access::select_epoch(f.root, f.lock, f.control().revision, true, id(8),
					     &changes, &f.error) == 0);
	f.commit(changes);
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(1), id(50), requests, &snapshot,
						&f.error) == 0 &&
	       snapshot.mappings.size() == 2 && snapshot.mappings[0].account.authority_id == 1);
	auto revision = snapshot.lineage_revision;
	auto wrong = requests;
	wrong[0].locator.native_id = 999;
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(1), id(50), wrong, &snapshot,
						&f.error) == ESTALE &&
	       snapshot.lineage_revision == revision);
	wrong[0] = wrong[1];
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(1), id(50), wrong, &snapshot,
						&f.error) == EINVAL);
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(9), id(50), requests, &snapshot,
						&f.error) == ESTALE);
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(1), id(51), requests, &snapshot,
						&f.error) == ESTALE);
	std::string same, other;
	for (size_t i = 0; same.empty() || other.empty(); ++i)
	{
		auto name = "alias" + std::to_string(i);
		if (bank_bucket(name) == bank_bucket("synthetic"))
			same = name;
		else
			other = name;
	}
	const auto original_mapping = read(f.file("mapping-02.eam"));
	f.rename(bank.account, same, 3);
	assert(f.bank(same).account.authority_id == 2);
	assert(flatfile_economic_native_lookup(f.root, f.lock, economic_account_kind::bank, 1,
					       { 2, 0, "synthetic" }, &preserved,
					       &f.error) == ENODATA);
	const auto current_mapping = read(f.file("mapping-02.eam"));
	write(f.file("mapping-02.eam"), original_mapping);
	assert(flatfile_economic_mapping_read(f.root, f.lock, bank.account, &preserved, &f.error) ==
		       EILSEQ &&
	       preserved.account.authority_id == 2);
	write(f.file("mapping-02.eam"), current_mapping);
	f.rename(bank.account, other, 4);
	f.rename(bank.account, "synthetic", 4); // rename-back retains lifetime.
	f.rename(bank.account, other, 4);
	auto replacement = f.create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	assert(replacement.account.authority_id != bank.account.authority_id &&
	       f.retained(bank.account).locator.name == other);
	auto before_collision = read(f.file("authority.eal"));
	assert(metadata_access::rename_bank(f.root, f.lock, f.control().revision, bank.account,
					    f.retained(bank.account).revision, "synthetic", id(5),
					    &changes, &f.error) == EEXIST &&
	       changes.empty());
	assert(read(f.file("authority.eal")) == before_collision);
	f.retire(replacement.account);
	auto recreated = f.create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	assert(recreated.account.authority_id > replacement.account.authority_id);
	assert(!critical_operation_id_is_zero(f.retained(replacement.account).retiring_operation));
	assert(flatfile_economic_mapping_read(f.root, f.lock, replacement.account, &preserved,
					      &f.error) ==
	       0); // Retained lookup ignores activity and current alias.
	f.retire(recreated.account);
	f.rename(bank.account, "synthetic", 4);
	bank = f.retained(bank.account);
	requests[0] = { bank.account, bank.locator };
	assert(metadata_access::select_epoch(f.root, f.lock, f.control().revision, false, id(8),
					     &changes, &f.error) == 0);
	f.commit(changes);
	assert(economic_flatfile_lock_authority(f.root, f.lock, id(1), id(50), requests, &snapshot,
						&f.error) == ENODATA);
	assert(flatfile_economic_mapping_read(f.root, f.lock, bank.account, &preserved, &f.error) ==
	       0);
	auto old_epochs = read(f.file("epochs.eae"));
	assert(metadata_access::append_epoch(f.root, f.lock, f.control().revision, epoch(2, 51, 50),
					     &changes, &f.error) == 0);
	f.commit(changes);
	auto new_epochs = read(f.file("epochs.eae"));
	// Membership is retained independently of current epoch selection.
	flatfile_economic_epoch retained_epoch;
	assert(flatfile_economic_epoch_read(f.root, f.lock, id(1), id(50), &retained_epoch,
					    &f.error) == 0);
	const auto original_epoch = epoch(1, 50);
	assert(retained_epoch.epoch.bytes == original_epoch.epoch.bytes &&
	       retained_epoch.predecessor.bytes == original_epoch.predecessor.bytes &&
	       retained_epoch.creating_operation.bytes == original_epoch.creating_operation.bytes &&
	       retained_epoch.ordinal == original_epoch.ordinal &&
	       retained_epoch.transition_kind == original_epoch.transition_kind &&
	       retained_epoch.transition_digest == original_epoch.transition_digest);
	for (const auto &query : { std::pair{ id(9), id(50) }, std::pair{ id(1), id(99) },
				   std::pair{ critical_operation_id{}, id(50) } })
	{
		const auto expected = critical_operation_id_is_zero(query.first) ?
					      EINVAL :
					      (query.first.bytes == id(1).bytes ? ENODATA : ESTALE);
		assert(flatfile_economic_epoch_read(f.root, f.lock, query.first, query.second,
						    &retained_epoch,
						    &f.error) == static_cast<unsigned>(expected));
		assert(retained_epoch.epoch.bytes == original_epoch.epoch.bytes &&
		       retained_epoch.predecessor.bytes == original_epoch.predecessor.bytes &&
		       retained_epoch.creating_operation.bytes ==
			       original_epoch.creating_operation.bytes &&
		       retained_epoch.ordinal == original_epoch.ordinal &&
		       retained_epoch.transition_kind == original_epoch.transition_kind &&
		       retained_epoch.transition_digest == original_epoch.transition_digest);
	}

	write(f.file("epochs.eae"), old_epochs);
	assert(flatfile_economic_control_read(f.root, f.lock, &absent, &f.error) == EILSEQ &&
	       absent.revision == 99);
	write(f.file("epochs.eae"), new_epochs);
	assert(metadata_access::append_epoch(f.root, f.lock, f.control().revision, epoch(3, 50, 51),
					     &changes, &f.error) == EILSEQ &&
	       changes.empty());
	assert(metadata_access::append_epoch(f.root, f.lock, f.control().revision, epoch(4, 52, 51),
					     &changes, &f.error) == EILSEQ);
	assert(metadata_access::initialize_evidence_bucket(f.root, f.lock, f.control().revision, 1,
							   id(9), &changes, &f.error) == 0);
	f.commit(changes);
	assert(f.control().evidence_initialized[0] == 2);
	assert(metadata_access::initialize_evidence_bucket(f.root, f.lock, f.control().revision, 1,
							   id(9), &changes, &f.error) == EALREADY);
	auto evidence = read(f.file("bucket-01.eai"));
	fs::remove(f.file("bucket-01.eai"));
	assert(metadata_access::initialize_evidence_bucket(f.root, f.lock, f.control().revision, 1,
							   id(9), &changes, &f.error) == EILSEQ);
	write(f.file("bucket-01.eai"), evidence);
	// Fill the first ID cycle and exercise the second retained entry in a bucket.
	for (uint64_t pid = 100; f.control().next_mapping_id <= 260; ++pid)
		f.create(economic_account_kind::wallet, 0, { 1, pid, {} });
	assert(f.retained(wallet.account).locator.native_id == 11 &&
	       f.control().next_mapping_id == 261);
	assert(metadata_access::select_epoch(f.root, f.lock, f.control().revision, true, id(8),
					     &changes, &f.error) == 0);
	f.commit(changes);
	std::vector<flatfile_economic_mapping_request> many;
	for (uint64_t identifier = 1; identifier <= 260; ++identifier)
	{
		auto key = wallet.account;
		key.authority_id = identifier;
		if (identifier >= 2 && identifier <= 4)
		{
			key.kind = economic_account_kind::bank;
			key.context_id = 1;
		}
		auto value = f.retained(key);
		if (critical_operation_id_is_zero(value.retiring_operation))
			many.push_back({ value.account, value.locator });
	}
	std::reverse(many.begin(), many.end());
	mapping_reads = {};
	native_reads = {};
	count_bucket_reads = true;
	const auto grouped = economic_flatfile_lock_authority(f.root, f.lock, id(1), id(51), many,
							      &snapshot, &f.error);
	count_bucket_reads = false;
	assert(grouped == 0 && snapshot.mappings.size() == many.size());
	assert(mapping_reads[1] == 1);
	for (size_t bucket = 0; bucket < 256; ++bucket)
		assert(mapping_reads[bucket] <= 1 && native_reads[bucket] <= 1);
	for (size_t i = 1; i < snapshot.mappings.size(); ++i)
		assert(snapshot.mappings[i - 1].account.authority_id <
		       snapshot.mappings[i].account.authority_id);

	auto original = read(f.file("mapping-01.eam"));
	auto control = read(f.file("authority.eal"));
	auto damaged =
		original; // Drop an allocated record while retaining valid envelope/control hashes.
	uint32_t length = 0;
	for (size_t i = 0; i < 4; ++i)
		length |= uint32_t(damaged[72 + i]) << (8 * i);
	damaged.erase(damaged.begin() + 72, damaged.begin() + 76 + length);
	damaged[68] = 1;
	const uint32_t payload = damaged.size() - 48;
	for (size_t i = 0; i < 4; ++i)
		damaged[12 + i] = static_cast<uint8_t>(payload >> (8 * i));
	rehash(damaged);
	auto rebound = control;
	auto digest = hash(damaged);
	std::copy(digest.begin(), digest.end(), rebound.begin() + 184 + 8192 + 32);
	rehash(rebound);
	write(f.file("mapping-01.eam"), damaged);
	write(f.file("authority.eal"), rebound);
	assert(flatfile_economic_mapping_read(f.root, f.lock, wallet.account, &preserved,
					      &f.error) == EILSEQ);
	write(f.file("authority.eal"), control);
	write(f.file("mapping-01.eam"), original);
	// Unacquired/wrong-root calls must not touch a pending corrupt journal.
	auto pending = fs::path(f.root) / "domains/.critical-authority-transaction";
	write(pending, { 1, 2, 3 });
	flatfile_authority_lock missing;
	assert(flatfile_economic_control_read(f.root, missing, &absent, &f.error) == EINVAL);
	assert(flatfile_economic_control_read(f.root + "-wrong", f.lock, &absent, &f.error) ==
		       EINVAL &&
	       read(pending) == bytes({ 1, 2, 3 }));
	assert(flatfile_economic_control_read(f.root, f.lock, &absent, &f.error) == EILSEQ);
	fs::remove(pending);
}

void corrupt_native(const fs::path &root)
{
	fixture f(root);
	ops changes;
	assert(metadata_access::bootstrap(f.root, f.lock, id(1), id(2), &changes, &f.error) == 0);
	f.commit(changes);
	const auto bucket = bank_bucket("synthetic");
	auto path = f.file(native_name(bucket));
	flatfile_economic_mapping out;
	out.revision = 999;
	write(path, {}); // Existing empty file is not a never-initialized bucket.
	assert(flatfile_economic_native_lookup(f.root, f.lock, economic_account_kind::bank, 1,
					       { 2, 0, "synthetic" }, &out, &f.error) == EILSEQ &&
	       out.revision == 999);
	assert(metadata_access::initialize_native_bucket(f.root, f.lock, f.control().revision,
							 bucket, id(3), &changes,
							 &f.error) == EILSEQ);
	fs::remove(path);
	assert(metadata_access::initialize_native_bucket(f.root, f.lock, f.control().revision,
							 bucket, id(3), &changes, &f.error) == 0);
	f.commit(changes);
	const auto empty_index = read(path);
	auto mapping = f.create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	const auto index = read(path), control = read(f.file("authority.eal"));
	write(path, empty_index); // Valid stale bytes cannot pass the current control digest.
	assert(flatfile_economic_native_lookup(f.root, f.lock, economic_account_kind::bank, 1,
					       { 2, 0, "synthetic" }, &out, &f.error) == EILSEQ);
	assert(metadata_access::create_mapping(
		       f.root, f.lock, f.control().revision, economic_account_kind::bank, 1,
		       { 2, 0, "synthetic" }, id(4), &out, &changes, &f.error) == EILSEQ &&
	       changes.empty());
	write(path, index);
	auto tombstone = index;
	std::fill(tombstone.begin() + 76, tombstone.begin() + 84, 0);
	rehash(tombstone);
	auto rebound = control;
	bind_native(rebound, bucket, tombstone);
	write(path, tombstone);
	write(f.file("authority.eal"), rebound);
	// Even with valid linked hashes, an active lifetime at this exact alias
	// contradicts a tombstone. Neither lookup nor name reuse can accept it.
	assert(flatfile_economic_native_lookup(f.root, f.lock, economic_account_kind::bank, 1,
					       { 2, 0, "synthetic" }, &out, &f.error) == EILSEQ);
	assert(metadata_access::create_mapping(
		       f.root, f.lock, f.control().revision, economic_account_kind::bank, 1,
		       { 2, 0, "synthetic" }, id(4), &out, &changes, &f.error) == EILSEQ &&
	       changes.empty());
	write(f.file("authority.eal"), control);
	write(path, index);
	auto mapping_file = f.file("mapping-01.eam");
	auto saved = read(mapping_file);
	fs::remove(mapping_file);
	assert(flatfile_economic_mapping_read(f.root, f.lock, mapping.account, &out, &f.error) ==
	       EILSEQ);
	write(mapping_file, saved);
	for (const auto &name : { std::string("authority.eal"), std::string("epochs.eae"),
				  std::string("mapping-01.eam"), native_name(bucket) })
	{
		auto file = f.file(name);
		auto good = read(file), bad = good;
		bad[8] = 2;
		write(file, bad);
		assert((name.ends_with(".ean") ?
				flatfile_economic_native_lookup(
					f.root, f.lock, economic_account_kind::bank, 1,
					{ 2, 0, "synthetic" }, &out, &f.error) :
				flatfile_economic_mapping_read(f.root, f.lock, mapping.account,
							       &out, &f.error)) != 0);
		write(file, good);
	}
	// Duplicate destination or exhausted transaction budget leaves all inputs unchanged.
	changes = { { flatfile_authority_store::economic_evidence,
		      flatfile_authority_operation_kind::write,
		      "authority.eal",
		      { 9 } } };
	assert(metadata_access::retire_mapping(f.root, f.lock, f.control().revision,
					       mapping.account, 0, id(6), &changes,
					       &f.error) == EINVAL &&
	       changes.size() == 1 && changes[0].bytes == bytes{ 9 });
	changes.clear();
	for (size_t i = 0; i < 30; ++i)
		changes.push_back({ flatfile_authority_store::domains,
				    flatfile_authority_operation_kind::write,
				    "domain-" + std::to_string(i),
				    { 1 } });
	assert(metadata_access::retire_mapping(f.root, f.lock, f.control().revision,
					       mapping.account, 0, id(6), &changes,
					       &f.error) == ENOSPC &&
	       changes.size() == 30);
	assert(f.bank("synthetic").account.authority_id == mapping.account.authority_id);
}
void crash_boundaries(const fs::path &parent)
{
	size_t cases = 0;
	for (int action = 0; action < 3; ++action)
	{
		const size_t count = action == 0 ? 3 : action == 1 ? 4 : 3;
		for (size_t boundary = 1; boundary <= count; ++boundary)
		{
			const auto root = parent / std::to_string(++cases);
			ops changes;
			economic_account_key key;
			std::string target = "renamed";
			while (bank_bucket(target) == bank_bucket("synthetic"))
				target += "x";
			{
				fixture f(root);
				f.initialize();
				auto mapping = f.create(economic_account_kind::bank, 1,
							{ 2, 0, "synthetic" });
				key = mapping.account;
				if (action == 0)
				{
					flatfile_economic_mapping next;
					assert(metadata_access::create_mapping(
						       f.root, f.lock, f.control().revision,
						       economic_account_kind::wallet, 0,
						       { 1, 77, {} }, id(10), &next, &changes,
						       &f.error) == 0);
					key = next.account;
				}
				else if (action == 1)
					assert(metadata_access::rename_bank(
						       f.root, f.lock, f.control().revision, key, 0,
						       target, id(11), &changes, &f.error) == 0);
				else
					assert(metadata_access::retire_mapping(
						       f.root, f.lock, f.control().revision, key, 0,
						       id(12), &changes, &f.error) == 0);
				assert(changes.size() == count);
			}
			auto child = fork();
			assert(child >= 0);
			if (!child)
			{
				flatfile_authority_lock lock;
				std::string error;
				assert(lock.acquire(root.string(), &error));
				setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_OPERATION",
				       std::to_string(boundary).c_str(), 1);
				assert(metadata_access::commit(root.string(), lock, changes,
							       &error) ==
				       flatfile_authority_transaction_result::io_error);
				_exit(77);
			}
			int outcome = 0;
			assert(waitpid(child, &outcome, 0) == child && WIFEXITED(outcome) &&
			       WEXITSTATUS(outcome) == 77);
			fixture f(root);
			auto result = f.retained(key);
			if (action == 0)
				assert(result.locator.native_id == 77 &&
				       f.control().next_mapping_id == 3);
			else if (action == 1)
				assert(result.locator.name == target &&
				       f.bank(target).account.authority_id == key.authority_id);
			else
				assert(!critical_operation_id_is_zero(result.retiring_operation));
			const auto control = read(f.file("authority.eal"));
			assert(f.retained(key).revision == result.revision);
			assert(read(f.file("authority.eal")) == control &&
			       !fs::exists(root / "domains/.critical-authority-transaction"));
		}
	}
	assert(cases == 10);
}
void capacity_and_allocation(const fs::path &root)
{
	fixture f(root);
	f.initialize();
	auto bank = f.create(economic_account_kind::bank, 1, { 2, 0, "synthetic" });
	ops changes;
	assert(metadata_access::append_epoch(f.root, f.lock, f.control().revision, epoch(1, 50),
					     &changes, &f.error) == 0);
	f.commit(changes);
	assert(metadata_access::select_epoch(f.root, f.lock, f.control().revision, true, id(8),
					     &changes, &f.error) == 0);
	f.commit(changes);
	assert(metadata_access::initialize_evidence_bucket(f.root, f.lock, f.control().revision, 1,
							   id(9), &changes, &f.error) == 0);
	f.commit(changes);
	const auto revision = f.control().revision;
	const auto control = read(f.file("authority.eal"));
	// Fail exactly the buffer allocation after opening authority.eal, ensuring
	// read_file preserves ENOMEM instead of flattening it to generic EIO.
	const auto original = f.control();
	for (bool allocation : { true, false })
	{
		auto out = original;
		fail_control_allocation = allocation;
		fail_control_io = !allocation;
		control_fault_seen = false;
		errno = ENOMEM; // An unrelated I/O failure must not inherit stale errno.
		const auto result = flatfile_economic_control_read(f.root, f.lock, &out, &f.error);
		allocation_target = 0;
		assert(control_fault_seen &&
		       result == static_cast<unsigned>(allocation ? ENOMEM : EIO));
		assert(out.lineage.bytes == original.lineage.bytes &&
		       out.creating_operation.bytes == original.creating_operation.bytes &&
		       out.last_operation.bytes == original.last_operation.bytes &&
		       out.last_epoch.bytes == original.last_epoch.bytes &&
		       out.active_epoch.bytes == original.active_epoch.bytes &&
		       out.revision == original.revision &&
		       out.next_mapping_id == original.next_mapping_id &&
		       out.epoch_count == original.epoch_count &&
		       out.epochs_digest == original.epochs_digest &&
		       out.native_digests == original.native_digests &&
		       out.mapping_digests == original.mapping_digests &&
		       out.evidence_initialized == original.evidence_initialized);
		assert(read(f.file("authority.eal")) == control);
	}
	puts("flatfile authority: metadata read ENOMEM and I/O classification preserve full output");
	size_t create_failures = 0, snapshot_failures = 0;
	for (size_t fail = 1; fail < 1000; ++fail)
	{
		flatfile_economic_mapping out;
		out.revision = 1234;
		changes.clear();
		allocation_target = fail;
		allocation_seen = 0;
		const auto result = metadata_access::create_mapping(f.root, f.lock, revision,
								    economic_account_kind::wallet,
								    0, { 1, 66, {} }, id(10), &out,
								    &changes, &f.error);
		const auto observed = allocation_seen;
		allocation_target = 0;
		if (!result)
		{
			assert(observed < fail && !changes.empty());
			break;
		}
		assert((result == ENOMEM || result == EIO) && changes.empty() &&
		       out.revision == 1234);
		++create_failures;
	}
	std::array<flatfile_economic_mapping_request, 1> requests = { { { bank.account,
									  bank.locator } } };
	for (size_t fail = 1; fail < 1000; ++fail)
	{
		flatfile_economic_authority_snapshot out;
		out.lineage_revision = 1234;
		allocation_target = fail;
		allocation_seen = 0;
		const auto result = economic_flatfile_lock_authority(f.root, f.lock, id(1), id(50),
								     requests, &out, &f.error);
		const auto observed = allocation_seen;
		allocation_target = 0;
		if (!result)
		{
			assert(observed < fail && out.mappings.size() == 1);
			break;
		}
		assert((result == ENOMEM || result == EIO) && out.lineage_revision == 1234 &&
		       out.mappings.empty());
		++snapshot_failures;
	}
	assert(create_failures > 20 && create_failures < 999 && snapshot_failures > 20 &&
	       snapshot_failures < 999);
	size_t initialize_failures = 0;
	changes.clear();
	for (size_t fail = 1; fail < 1000; ++fail)
	{
		allocation_target = fail;
		allocation_seen = 0;
		const auto result = metadata_access::initialize_evidence_bucket(
			f.root, f.lock, revision, 1, id(9), &changes, &f.error);
		const auto observed = allocation_seen;
		allocation_target = 0;
		assert(changes.empty() &&
		       (result == EALREADY || result == ENOMEM || result == EIO));
		if (observed < fail)
		{
			assert(result == EALREADY);
			break;
		}
		++initialize_failures;
	}
	assert(initialize_failures > 10 && initialize_failures < 999);
	printf("flatfile authority: %zu repeated-initialization allocation cases preserve metadata/error classification\n",
	       initialize_failures);
	assert(read(f.file("authority.eal")) == control);
	std::string target = "full-target";
	while (bank_bucket(target) == bank_bucket("synthetic"))
		target += "x";
	const auto bucket = bank_bucket(target);
	std::vector<std::string> names;
	for (size_t n = 0; names.size() < FLATFILE_ECONOMIC_BUCKET_MAPPINGS; ++n)
	{
		auto name = "historical-alias-" + std::to_string(n);
		if (bank_bucket(name) == bucket)
			names.push_back(name);
	}
	std::sort(names.begin(), names.end());
	bytes body;
	const auto lineage = id(1);
	body.insert(body.end(), lineage.bytes.begin(), lineage.bytes.end());
	number(body, bucket, 4);
	number(body, names.size(), 4);
	for (const auto &name : names)
	{
		bytes key;
		number(key, 2, 2);
		number(key, 1, 8);
		number(key, 2, 2);
		key.insert(key.end(), name.begin(), name.end());
		number(body, key.size(), 2);
		number(body, 0, 2);
		number(body, 0, 8);
		number(body, bank.account.authority_id, 8);
		body.insert(body.end(), key.begin(), key.end());
	}
	auto path = f.file(native_name(bucket));
	auto empty = read(path);
	bytes full(empty.begin(), empty.begin() + 48);
	full.insert(full.end(), body.begin(), body.end());
	for (size_t i = 0; i < 4; ++i)
		full[12 + i] = static_cast<uint8_t>(body.size() >> (8 * i));
	rehash(full);
	auto rebound = control;
	bind_native(rebound, bucket, full);
	write(path, full);
	write(f.file("authority.eal"), rebound);
	changes.clear();
	flatfile_economic_mapping out;
	out.revision = 1234;
	assert(metadata_access::create_mapping(f.root, f.lock, revision,
					       economic_account_kind::bank, 1, { 2, 0, target },
					       id(10), &out, &changes, &f.error) == ENOSPC);
	assert(changes.empty() && out.revision == 1234);
	assert(metadata_access::rename_bank(f.root, f.lock, revision, bank.account, 0, target,
					    id(11), &changes, &f.error) == ENOSPC &&
	       changes.empty());
	assert(f.bank("synthetic").account.authority_id == bank.account.authority_id &&
	       read(path) == full && read(f.file("authority.eal")) == rebound);
	write(f.file("authority.eal"), control);
	write(path, empty);
	auto overflow = control;
	std::fill(overflow.begin() + 128, overflow.begin() + 136, 255);
	rehash(overflow);
	write(f.file("authority.eal"), overflow);
	assert(metadata_access::retire_mapping(f.root, f.lock, UINT64_MAX, bank.account, 0, id(12),
					       &changes, &f.error) == EOVERFLOW &&
	       changes.empty());
	write(f.file("authority.eal"), control);
	printf("flatfile authority: %zu create and %zu snapshot allocation failures preserve outputs; full native bucket and revision overflow refuse safely\n",
	       create_failures, snapshot_failures);
}

int main(int argc, char **argv)
{
	assert(argc == 2);
	const fs::path root = argv[1];
	basic(root / "basic");
	capacity_and_allocation(root / "capacity-allocation");
	corrupt_native(root / "corrupt");
	crash_boundaries(root / "crashes");
	puts("flatfile accounting authority: lifetimes, rename/recreation, bounded snapshots, corruption and 10 crash boundaries passed");
}
