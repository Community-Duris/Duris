#include "flatfile/flatfile_accounting_baseline.h"
#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_store.h"
#include <algorithm>
#include <cerrno>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <new>
#include <openssl/sha.h>
#include <set>
#include <string_view>
#include <unistd.h>

namespace
{
using status = flatfile_accounting_status;
using bytes = std::vector<uint8_t>;
using operations = std::vector<flatfile_authority_operation>;
struct failure
{
	status code;
};
void need(bool value, status code = status::invalid)
{
	if (!value)
		throw failure{ code };
}
void checked(status code)
{
	if (code != status::ok)
		throw failure{ code };
}
void checked(economic_accounting_error code)
{
	need(code == economic_accounting_error::ok,
	     code == economic_accounting_error::capacity ? status::capacity : status::invalid);
}
void authority(unsigned int code)
{
	need(!code, code == ENOMEM || code == ENOSPC ? status::capacity :
		    code == EIO			     ? status::io_error :
						       status::invalid);
}
economic_digest hash(std::span<const uint8_t> value)
{
	economic_digest result;
	SHA256(value.data(), value.size(), result.data());
	return result;
}
void number(bytes &out, uint64_t value, size_t width = 8)
{
	for (size_t i = 0; i < width; ++i)
		out.push_back(static_cast<uint8_t>(value >> (i * 8)));
}
void raw(bytes &out, std::span<const uint8_t> value)
{
	out.insert(out.end(), value.begin(), value.end());
}
struct reader
{
	std::span<const uint8_t> value;
	size_t offset = 0;
	std::span<const uint8_t> take(size_t count)
	{
		need(offset <= value.size() && count <= value.size() - offset);
		auto result = value.subspan(offset, count);
		offset += count;
		return result;
	}
	uint64_t number(size_t count = 8)
	{
		auto part = take(count);
		uint64_t result = 0;
		for (size_t i = 0; i < count; ++i)
			result |= uint64_t(part[i]) << (8 * i);
		return result;
	}
	template <size_t N> std::array<uint8_t, N> fixed()
	{
		auto part = take(N);
		std::array<uint8_t, N> result;
		std::copy(part.begin(), part.end(), result.begin());
		return result;
	}
	critical_operation_id id() { return { fixed<16>() }; }
	void done() { need(offset == value.size()); }
};
bytes envelope(const char *magic, const bytes &body)
{
	bytes result;
	raw(result, { reinterpret_cast<const uint8_t *>(magic), 8 });
	number(result, 1, 4);
	number(result, body.size(), 4);
	raw(result, hash(body));
	raw(result, body);
	return result;
}
reader unwrap(const bytes &value, const char *magic)
{
	reader in{ value };
	auto tag = in.take(8);
	need(!memcmp(tag.data(), magic, 8));
	need(in.number(4) == 1 && in.number(4) == value.size() - 48);
	auto digest = in.fixed<32>();
	auto body = in.take(value.size() - 48);
	need(hash(body) == digest);
	return { body };
}
std::string hex(const critical_operation_id &id)
{
	char result[CRITICAL_COMMAND_ID_HEX_SIZE];
	need(critical_operation_id_to_hex(id, result, sizeof(result)) &&
	     !critical_operation_id_is_zero(id));
	return result;
}
std::string prefix(const critical_operation_id &lineage, const critical_operation_id &epoch)
{
	return "baseline-" + hex(lineage) + "-" + hex(epoch) + "-";
}
std::string index_name(const std::string &base, size_t bucket)
{
	need(bucket < FLATFILE_BASELINE_BUCKETS);
	return base + "0123456789abcdef"[bucket] + ".ebi";
}
bytes read(const std::string &root, const std::string &name, size_t maximum)
{
	bytes result;
	errno = 0;
	auto code = flatfile_read(root + "/economic-evidence", name, maximum, &result, nullptr);
	need(code == flatfile_read_result::ok,
	     code == flatfile_read_result::io_error ?
		     (errno == ENOMEM ? status::capacity : status::io_error) :
		     status::invalid);
	return result;
}
void absent(const std::string &root, const std::string &name)
{
	bytes ignored;
	errno = 0;
	auto code = flatfile_read(root + "/economic-evidence", name, ECONOMIC_BASELINE_MAX_BYTES,
				  &ignored, nullptr);
	need(code == flatfile_read_result::not_found,
	     code == flatfile_read_result::io_error ?
		     (errno == ENOMEM ? status::capacity : status::io_error) :
		     status::conflict);
}
struct head
{
	critical_operation_id lineage, epoch, last_operation;
	economic_account_key opening;
	uint64_t revision = 0;
	std::array<economic_digest, FLATFILE_BASELINE_BUCKETS> indexes;
};
bytes encode(const head &value)
{
	bytes body;
	raw(body, value.lineage.bytes);
	raw(body, value.epoch.bytes);
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> key;
	checked(economic_account_key_encode(value.opening, &key));
	raw(body, key);
	number(body, value.revision);
	raw(body, value.last_operation.bytes);
	for (const auto &digest : value.indexes)
		raw(body, digest);
	return envelope("DUREBC1", body);
}
void membership(const std::string &root, const flatfile_authority_lock &lock,
		const critical_operation_id &lineage, const critical_operation_id &epoch)
{
	flatfile_economic_control control;
	authority(flatfile_economic_control_read(root, lock, &control, nullptr));
	need(control.lineage.bytes == lineage.bytes);
	flatfile_economic_epoch retained;
	authority(flatfile_economic_epoch_read(root, lock, lineage, epoch, &retained, nullptr));
}
head load(const std::string &root, const flatfile_authority_lock &lock,
	  const critical_operation_id &lineage, const critical_operation_id &epoch)
{
	membership(root, lock, lineage, epoch);
	auto encoded = read(root, prefix(lineage, epoch) + "head.ebc", 656);
	auto in = unwrap(encoded, "DUREBC1");
	head value;
	value.lineage = in.id();
	value.epoch = in.id();
	checked(economic_account_key_decode(in.take(ECONOMIC_ACCOUNT_KEY_BYTES), &value.opening));
	value.revision = in.number();
	value.last_operation = in.id();
	for (auto &digest : value.indexes)
		digest = in.fixed<32>();
	in.done();
	need(value.lineage.bytes == lineage.bytes && value.epoch.bytes == epoch.bytes &&
	     value.opening.kind == economic_account_kind::opening &&
	     value.opening.lineage.bytes == lineage.bytes &&
	     !critical_operation_id_is_zero(value.last_operation));
	return value;
}
struct reservation
{
	uint64_t kind, id;
	critical_operation_id operation;
};
bool less(const reservation &a, const reservation &b)
{
	return a.kind < b.kind || (a.kind == b.kind && a.id < b.id);
}
using bucket = std::vector<reservation>;
bytes encode(const head &value, size_t slot, const bucket &entries)
{
	need(entries.size() <= FLATFILE_BASELINE_BUCKET_RECORDS, status::capacity);
	bytes body;
	raw(body, value.lineage.bytes);
	raw(body, value.epoch.bytes);
	number(body, slot, 4);
	number(body, entries.size(), 4);
	for (const auto &entry : entries)
	{
		number(body, entry.kind);
		number(body, entry.id);
		raw(body, entry.operation.bytes);
	}
	return envelope("DUREBI1", body);
}
bucket load(const std::string &root, const head &value, size_t slot)
{
	auto encoded = read(root, index_name(prefix(value.lineage, value.epoch), slot),
			    FLATFILE_BASELINE_INDEX_MAX_BYTES);
	need(hash(encoded) == value.indexes[slot]);
	auto in = unwrap(encoded, "DUREBI1");
	need(in.id().bytes == value.lineage.bytes && in.id().bytes == value.epoch.bytes &&
	     in.number(4) == slot);
	auto count = in.number(4);
	need(count <= FLATFILE_BASELINE_BUCKET_RECORDS && in.value.size() == 40 + count * 32);
	bucket result;
	result.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		reservation entry{ in.number(), in.number(), in.id() };
		need((entry.kind == 1 || entry.kind == 2) && entry.id &&
		     entry.id % FLATFILE_BASELINE_BUCKETS == slot &&
		     !critical_operation_id_is_zero(entry.operation) &&
		     (result.empty() || less(result.back(), entry)));
		result.push_back(entry);
	}
	in.done();
	return result;
}
bucket reservations(const economic_prepared_baseline &prepared)
{
	bucket result;
	for (const auto &holding : prepared.witness().holdings)
		result.push_back(
			{ 1, holding.account.authority_id, prepared.plan().metadata.operation_id });
	for (const auto &item : prepared.witness().items)
		result.push_back({ 2, item.snapshot.uid, prepared.plan().metadata.operation_id });
	std::sort(result.begin(), result.end(), less);
	return result;
}
void room(const operations &ops)
{
	need(ops.size() <= flatfile_authority_transaction_maximum_operations, status::capacity);
	// Current v2 journal: 48-byte envelope and 2-byte operation count.
	size_t total = 50;
	std::set<std::pair<uint8_t, std::string>> names;
	for (const auto &op : ops)
	{
		need(names.emplace(static_cast<uint8_t>(op.store), op.filename).second,
		     status::conflict);
		need(op.filename.size() <= 192 &&
			     op.bytes.size() <= flatfile_authority_transaction_maximum_bytes,
		     status::capacity);
		const auto size = 8 + op.filename.size() + op.bytes.size();
		need(size <= flatfile_authority_transaction_maximum_bytes - total,
		     status::capacity);
		total += size;
	}
}
void append(operations &ops, const std::string &name, bytes value)
{
	ops.push_back({ flatfile_authority_store::economic_evidence,
			flatfile_authority_operation_kind::write, name, std::move(value) });
}
void empty_namespace(const std::string &root, const std::string &base)
{
	int fd = open((root + "/economic-evidence").c_str(),
		      O_RDONLY | O_CLOEXEC | O_DIRECTORY | O_NOFOLLOW);
	need(fd >= 0, status::io_error);
	DIR *dir = fdopendir(fd);
	if (!dir)
	{
		close(fd);
		throw failure{ status::io_error };
	}
	struct cleanup
	{
		DIR *dir;
		~cleanup() { closedir(dir); }
	} owner{ dir };
	size_t count = 0;
	errno = 0;
	while (auto *entry = readdir(dir))
	{
		need(++count <= 1024 * 1024, status::capacity);
		need(!std::string_view(entry->d_name).starts_with(base), status::conflict);
		errno = 0;
	}
	need(!errno, status::io_error);
}
template <class Work> status guarded(Work work, std::string *error)
{
	try
	{
		work();
		return status::ok;
	}
	catch (const failure &value)
	{
		if (error)
			try
			{
				*error = "baseline evidence refused";
			}
			catch (const std::bad_alloc &)
			{
			}
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return status::capacity;
	}
}
}

flatfile_accounting_status flatfile_accounting_baseline_lookup(const std::string &root,
							       const flatfile_authority_lock &lock,
							       const critical_command &command,
							       flatfile_accounting_record *record,
							       bytes *witness, std::string *error)
{
	return guarded(
		[&]
		{
			need(record && witness &&
			     command.type == critical_command_type::economic_baseline &&
			     command.schema_version == CRITICAL_COMMAND_ACCOUNTING_SCHEMA_VERSION &&
			     critical_command_envelope_valid(command));
			flatfile_accounting_record retained;
			checked(flatfile_accounting_lookup(root, lock, command, &retained, error));
			economic_frozen_intent intent;
			checked(economic_intent_decode(command.accounting_intent, &intent));
			const auto &meta = intent.admission.metadata;
			const auto book = load(root, lock, meta.lineage, meta.epoch);
			need(!retained.result_code &&
			     retained.failure_stage == critical_failure_stage::none &&
			     retained.result.empty() && retained.durable_revision &&
			     retained.durable_revision <= book.revision);
			auto source = read(root,
					   prefix(meta.lineage, meta.epoch) +
						   hex(command.operation_id) + ".eab",
					   ECONOMIC_BASELINE_MAX_BYTES);
			std::optional<economic_prepared_baseline> prepared;
			checked(economic_baseline_decode(source, &prepared));
			need(economic_account_key_equal(book.opening,
							prepared->witness().opening_account));
			economic_accounting_plan plan;
			checked(economic_baseline_command_plan(command, *prepared, &plan));
			bytes encoded;
			checked(economic_plan_encode(plan, &encoded));
			need(encoded == retained.plan);
			std::array<bucket, FLATFILE_BASELINE_BUCKETS> indexes;
			std::array<bool, FLATFILE_BASELINE_BUCKETS> loaded = {};
			for (const auto &entry : reservations(*prepared))
			{
				const auto slot = entry.id % FLATFILE_BASELINE_BUCKETS;
				if (!loaded[slot])
				{
					indexes[slot] = load(root, book, slot);
					loaded[slot] = true;
				}
				const auto found = std::lower_bound(
					indexes[slot].begin(), indexes[slot].end(), entry, less);
				need(found != indexes[slot].end() && found->kind == entry.kind &&
				     found->id == entry.id &&
				     found->operation.bytes == command.operation_id.bytes);
			}
			*record = std::move(retained);
			*witness = std::move(source);
		},
		error);
}

flatfile_accounting_status flatfile_accounting_baseline_storage::initialize(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const economic_account_key &opening, const critical_operation_id &creating_operation,
	operations *ops, std::string *error)
{
	return guarded(
		[&]
		{
			need(ops && economic_account_key_valid(opening) &&
			     opening.kind == economic_account_kind::opening &&
			     opening.lineage.bytes == lineage.bytes &&
			     !critical_operation_id_is_zero(creating_operation));
			room(*ops);
			need(ops->size() + FLATFILE_BASELINE_BUCKETS + 1 <=
				     flatfile_authority_transaction_maximum_operations,
			     status::capacity);
			membership(root, lock, lineage, epoch);
			const auto base = prefix(lineage, epoch);
			empty_namespace(root, base);
			head book{ lineage, epoch, creating_operation, opening, 0, {} };
			auto result = *ops;
			for (size_t slot = 0; slot < FLATFILE_BASELINE_BUCKETS; ++slot)
			{
				auto encoded = encode(book, slot, {});
				book.indexes[slot] = hash(encoded);
				append(result, index_name(base, slot), std::move(encoded));
			}
			append(result, base + "head.ebc", encode(book));
			room(result);
			*ops = std::move(result);
		},
		error);
}

flatfile_accounting_status flatfile_accounting_baseline_storage::stage(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_command &command, const economic_prepared_baseline &prepared,
	operations *ops, std::string *error)
{
	return guarded(
		[&]
		{
			need(ops);
			room(*ops);
			economic_accounting_plan plan;
			checked(economic_baseline_command_plan(command, prepared, &plan));
			flatfile_accounting_record retained;
			bytes source;
			auto existing = flatfile_accounting_baseline_lookup(
				root, lock, command, &retained, &source, error);
			if (existing == status::ok)
				throw failure{ status::already_exists };
			need(existing == status::not_found, existing);
			auto book = load(root, lock, plan.metadata.lineage, plan.metadata.epoch);
			need(economic_account_key_equal(book.opening,
							prepared.witness().opening_account));
			need(book.revision != UINT64_MAX, status::capacity);
			const auto base = prefix(book.lineage, book.epoch);
			const auto name = base + hex(command.operation_id) + ".eab";
			absent(root, name);
			std::array<bucket, FLATFILE_BASELINE_BUCKETS> indexes;
			for (size_t slot = 0; slot < FLATFILE_BASELINE_BUCKETS; ++slot)
				indexes[slot] = load(root, book, slot);
			std::array<bucket, FLATFILE_BASELINE_BUCKETS> added;
			for (const auto &entry : reservations(prepared))
			{
				const auto slot = entry.id % FLATFILE_BASELINE_BUCKETS;
				const auto &entries = indexes[slot];
				auto where = std::lower_bound(entries.begin(), entries.end(), entry,
							      less);
				need(where == entries.end() || where->kind != entry.kind ||
					     where->id != entry.id,
				     status::conflict);
				added[slot].push_back(entry);
				need(entries.size() + added[slot].size() <=
					     FLATFILE_BASELINE_BUCKET_RECORDS,
				     status::capacity);
			}
			const size_t changed = std::count_if(added.begin(), added.end(),
							     [](const auto &value)
							     { return !value.empty(); });
			need(ops->size() + changed + 4 <=
				     flatfile_authority_transaction_maximum_operations,
			     status::capacity);
			auto result = *ops;
			for (size_t slot = 0; slot < FLATFILE_BASELINE_BUCKETS; ++slot)
				if (!added[slot].empty())
				{
					// Merge once per bucket; inserting each reservation separately
					// would repeatedly shift a full retained index.
					bucket merged;
					merged.reserve(indexes[slot].size() + added[slot].size());
					std::merge(indexes[slot].begin(), indexes[slot].end(),
						   added[slot].begin(), added[slot].end(),
						   std::back_inserter(merged), less);
					auto encoded = encode(book, slot, merged);
					book.indexes[slot] = hash(encoded);
					append(result, index_name(base, slot), std::move(encoded));
				}
			checked(economic_baseline_encode(prepared, &source));
			append(result, name, std::move(source));
			++book.revision;
			book.last_operation = command.operation_id;
			append(result, base + "head.ebc", encode(book));
			flatfile_accounting_record record;
			record.command = command;
			record.durable_revision = book.revision;
			record.result_code = 0;
			record.failure_stage = critical_failure_stage::none;
			checked(economic_plan_encode(plan, &record.plan));
			// Preserve the common store's refusal of caller-supplied economic
			// after-images. Combine independently staged private bundles only
			// under this same lock, then recheck total capacity and targets.
			operations receipt;
			checked(flatfile_accounting_storage::stage(root, lock, record, &receipt,
								   error));
			for (auto &operation : receipt)
				result.push_back(std::move(operation));
			room(result);
			*ops = std::move(result);
		},
		error);
}
