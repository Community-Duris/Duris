#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_store.h"
#include "economy/currency_command.h"
#include <algorithm>
#include <cerrno>
#include <climits>
#include <cstring>
#include <dirent.h>
#include <fcntl.h>
#include <map>
#include <new>
#include <openssl/sha.h>
#include <set>
#include <sys/stat.h>
#include <unistd.h>
namespace
{
using bytes = std::vector<uint8_t>;
using operations = std::vector<flatfile_authority_operation>;
using mappings = std::vector<flatfile_economic_mapping>;
using epochs = std::vector<flatfile_economic_epoch>;
struct failure
{
	unsigned int code;
};
void need(bool value, unsigned int error = EILSEQ)
{
	if (!value)
		throw failure{ error };
}
bool nonzero(const critical_operation_id &id)
{
	return !critical_operation_id_is_zero(id);
}
economic_digest hash(std::span<const uint8_t> data)
{
	economic_digest value = {};
	SHA256(data.data(), data.size(), value.data());
	return value;
}
void number(bytes &out, uint64_t value, size_t width)
{
	for (size_t i = 0; i < width; ++i)
		out.push_back(static_cast<uint8_t>(value >> (8 * i)));
}
void raw(bytes &out, std::span<const uint8_t> data)
{
	out.insert(out.end(), data.begin(), data.end());
}
struct reader
{
	std::span<const uint8_t> data;
	size_t offset = 0;
	std::span<const uint8_t> take(size_t count)
	{
		need(offset <= data.size() && count <= data.size() - offset);
		auto part = data.subspan(offset, count);
		offset += count;
		return part;
	}
	uint64_t number(size_t count)
	{
		auto part = take(count);
		uint64_t value = 0;
		for (size_t i = 0; i < count; ++i)
			value |= uint64_t(part[i]) << (8 * i);
		return value;
	}
	template <size_t N> std::array<uint8_t, N> fixed()
	{
		auto part = take(N);
		std::array<uint8_t, N> value;
		std::copy(part.begin(), part.end(), value.begin());
		return value;
	}
	critical_operation_id id() { return { fixed<16>() }; }
	void done() { need(offset == data.size()); }
};
bytes envelope(const char *magic, const bytes &body)
{
	need(body.size() + 48 <= FLATFILE_ECONOMIC_METADATA_MAX_BYTES, ENOSPC);
	bytes out;
	raw(out, { reinterpret_cast<const uint8_t *>(magic), 8 });
	number(out, 1, 4);
	number(out, body.size(), 4);
	raw(out, hash(body));
	raw(out, body);
	return out;
}
reader unwrap(const bytes &encoded, const char *magic)
{
	need(encoded.size() >= 48 && encoded.size() <= FLATFILE_ECONOMIC_METADATA_MAX_BYTES);
	reader in{ encoded };
	auto prefix = in.take(8);
	need(!memcmp(prefix.data(), magic, 8));
	need(in.number(4) == 1 && in.number(4) == encoded.size() - 48);
	auto digest = in.fixed<32>();
	auto body = in.take(encoded.size() - 48);
	need(digest == hash(body));
	return { body };
}
std::string directory(const std::string &root)
{
	return root + "/economic-evidence";
}
std::string bucket_name(const char *prefix, size_t bucket, const char *suffix)
{
	need(bucket < 256, EINVAL);
	constexpr char hex[] = "0123456789abcdef";
	return std::string(prefix) + hex[bucket >> 4] + hex[bucket & 15] + suffix;
}
std::string mapping_name(size_t bucket)
{
	return bucket_name("mapping-", bucket, ".eam");
}
std::string native_name(size_t bucket)
{
	return bucket_name("native-", bucket, ".ean");
}
bytes read_file(const std::string &root, const std::string &name)
{
	bytes value;
	errno = 0;
	auto result = flatfile_read(directory(root), name, FLATFILE_ECONOMIC_METADATA_MAX_BYTES,
				    &value, nullptr);
	need(result == flatfile_read_result::ok,
	     result == flatfile_read_result::io_error ? (errno == ENOMEM ? ENOMEM : EIO) : EILSEQ);
	return value;
}
void require_absent(const std::string &root, const std::string &name)
{
	bytes ignored;
	const auto result = flatfile_read(directory(root), name,
					  FLATFILE_ECONOMIC_METADATA_MAX_BYTES, &ignored, nullptr);
	need(result == flatfile_read_result::not_found,
	     result == flatfile_read_result::io_error ? EIO : EILSEQ);
}
void recover(const std::string &root, const flatfile_authority_lock &lock)
{
	need(lock.matches(root), EINVAL);
	auto result = flatfile_authority_transaction_recover(root, lock, nullptr);
	need(result == flatfile_authority_transaction_result::ok,
	     result == flatfile_authority_transaction_result::io_error ? EIO : EILSEQ);
}
template <typename Work> unsigned int guarded(Work work, std::string *error)
{
	try
	{
		work();
		return 0;
	}
	catch (const failure &value)
	{
		try
		{
			if (error)
				*error = "flatfile economic authority refused (errno " +
					 std::to_string(value.code) + ")";
		}
		catch (const std::bad_alloc &)
		{
		}
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
}
size_t mapping_count(const flatfile_economic_control &control, size_t bucket)
{
	const uint64_t first = bucket ? bucket : 256;
	if (control.next_mapping_id <= first)
		return 0;
	return 1 + (control.next_mapping_id - 1 - first) / 256;
}
void validate_control(const flatfile_economic_control &value)
{
	need(nonzero(value.lineage) && nonzero(value.creating_operation) &&
	     nonzero(value.last_operation));
	need(value.next_mapping_id && value.next_mapping_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS + 1);
	need(value.epoch_count <= FLATFILE_ECONOMIC_MAX_EPOCHS &&
	     value.epochs_digest != economic_digest{});
	need(nonzero(value.last_epoch) == (value.epoch_count != 0));
	need(!nonzero(value.active_epoch) || value.active_epoch.bytes == value.last_epoch.bytes);
	for (size_t bucket = 0; bucket < 256; ++bucket)
		need((value.mapping_digests[bucket] != economic_digest{}) ==
		     (mapping_count(value, bucket) != 0));
}
bytes encode_control(const flatfile_economic_control &value)
{
	validate_control(value);
	bytes out;
	for (const auto &id : { value.lineage, value.creating_operation, value.last_operation,
				value.last_epoch, value.active_epoch })
		raw(out, id.bytes);
	number(out, value.revision, 8);
	number(out, value.next_mapping_id, 8);
	number(out, value.epoch_count, 4);
	number(out, 0, 4);
	raw(out, value.epochs_digest);
	for (const auto &digest : value.native_digests)
		raw(out, digest);
	for (const auto &digest : value.mapping_digests)
		raw(out, digest);
	raw(out, value.evidence_initialized);
	return envelope("DURECA1", out);
}
flatfile_economic_control decode_control(const bytes &encoded)
{
	auto in = unwrap(encoded, "DURECA1");
	flatfile_economic_control value;
	value.lineage = in.id();
	value.creating_operation = in.id();
	value.last_operation = in.id();
	value.last_epoch = in.id();
	value.active_epoch = in.id();
	value.revision = in.number(8);
	value.next_mapping_id = in.number(8);
	value.epoch_count = in.number(4);
	need(in.number(4) == 0);
	value.epochs_digest = in.fixed<32>();
	for (auto &digest : value.native_digests)
		digest = in.fixed<32>();
	for (auto &digest : value.mapping_digests)
		digest = in.fixed<32>();
	value.evidence_initialized = in.fixed<32>();
	in.done();
	validate_control(value);
	return value;
}
void validate_epochs(const epochs &values)
{
	need(values.size() <= FLATFILE_ECONOMIC_MAX_EPOCHS, ENOSPC);
	std::set<std::array<uint8_t, 16>> identities;
	critical_operation_id previous = {};
	for (size_t i = 0; i < values.size(); ++i)
	{
		const auto &value = values[i];
		need(nonzero(value.epoch) && nonzero(value.creating_operation) &&
		     value.transition_kind && value.transition_digest != economic_digest{} &&
		     value.ordinal == i + 1 && value.predecessor.bytes == previous.bytes &&
		     identities.insert(value.epoch.bytes).second);
		previous = value.epoch;
	}
}
bytes encode_epochs(const critical_operation_id &lineage, const epochs &values)
{
	validate_epochs(values);
	bytes out;
	raw(out, lineage.bytes);
	number(out, values.size(), 4);
	number(out, 0, 4);
	for (const auto &value : values)
	{
		raw(out, value.epoch.bytes);
		number(out, value.ordinal, 8);
		raw(out, value.predecessor.bytes);
		number(out, value.transition_kind, 2);
		number(out, 0, 6);
		raw(out, value.transition_digest);
		raw(out, value.creating_operation.bytes);
	}
	return envelope("DURECE1", out);
}
epochs load_epochs(const std::string &root, const flatfile_economic_control &control)
{
	auto encoded = read_file(root, "epochs.eae");
	need(hash(encoded) == control.epochs_digest);
	auto in = unwrap(encoded, "DURECE1");
	need(in.id().bytes == control.lineage.bytes);
	auto count = in.number(4);
	need(count == control.epoch_count && in.number(4) == 0);
	epochs values;
	values.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		flatfile_economic_epoch value;
		value.epoch = in.id();
		value.ordinal = in.number(8);
		value.predecessor = in.id();
		value.transition_kind = in.number(2);
		need(in.number(6) == 0);
		value.transition_digest = in.fixed<32>();
		value.creating_operation = in.id();
		values.push_back(value);
	}
	in.done();
	validate_epochs(values);
	need(values.empty() || values.back().epoch.bytes == control.last_epoch.bytes);
	return values;
}
flatfile_economic_control load_control(const std::string &root)
{
	auto value = decode_control(read_file(root, "authority.eal"));
	(void)load_epochs(root, value);
	return value;
}
void name_valid(const std::string &name)
{
	need(!name.empty() && name.size() <= CURRENCY_ACCOUNT_NAME_MAX_BYTES, EINVAL);
	for (auto c : name)
		need((c >= 'a' && c <= 'z') || (c >= '0' && c <= '9') || c == '_' || c == '-',
		     EINVAL);
}
void locator_valid(economic_account_kind kind, uint64_t context,
		   const flatfile_economic_locator &locator, bool creating = false)
{
	if (kind == economic_account_kind::wallet)
		need(context == 0 && locator.kind == 1 && locator.native_id > 0 &&
			     locator.native_id <= INT32_MAX && locator.name.empty(),
		     EINVAL);
	else
	{
		need(kind == economic_account_kind::bank && context <= INT8_MAX &&
			     locator.kind == 2 &&
			     (creating ? locator.native_id == 0 : locator.native_id > 0),
		     EINVAL);
		name_valid(locator.name);
	}
}
bytes native_key(economic_account_kind kind, uint64_t context,
		 const flatfile_economic_locator &locator)
{
	locator_valid(kind, context, locator,
		      kind == economic_account_kind::bank && locator.native_id == 0);
	bytes key;
	number(key, static_cast<uint16_t>(kind), 2);
	number(key, context, 8);
	number(key, locator.kind, 2);
	if (kind == economic_account_kind::wallet)
		number(key, locator.native_id, 8);
	else
		raw(key, { reinterpret_cast<const uint8_t *>(locator.name.data()),
			   locator.name.size() });
	return key;
}
void validate_native_key(const bytes &key)
{
	reader in{ key };
	const auto kind = static_cast<economic_account_kind>(in.number(2));
	auto context = in.number(8);
	flatfile_economic_locator locator;
	locator.kind = in.number(2);
	if (kind == economic_account_kind::wallet)
		locator.native_id = in.number(8);
	else
	{
		auto name = in.take(key.size() - in.offset);
		locator.name.assign(name.begin(), name.end());
	}
	in.done();
	try
	{
		need(native_key(kind, context, locator) == key);
	}
	catch (const failure &)
	{
		throw failure{ EILSEQ };
	}
}
void validate_mapping(const flatfile_economic_mapping &value)
{
	need(economic_account_key_valid(value.account) &&
	     value.account.authority_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS);
	locator_valid(value.account.kind, value.account.context_id, value.locator);
	need(nonzero(value.creating_operation) && nonzero(value.last_operation));
	need(value.revision || (value.last_operation.bytes == value.creating_operation.bytes &&
				!nonzero(value.retiring_operation)));
	if (nonzero(value.retiring_operation))
		need(value.last_operation.bytes == value.retiring_operation.bytes &&
		     value.revision);
	if (value.account.kind == economic_account_kind::bank)
		need(value.locator.native_id == value.account.authority_id);
}
bytes encode_mapping(const flatfile_economic_mapping &value)
{
	validate_mapping(value);
	bytes out;
	std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> account;
	need(economic_account_key_encode(value.account, &account) == economic_accounting_error::ok);
	raw(out, account);
	number(out, value.locator.kind, 2);
	number(out, value.locator.name.size(), 2);
	number(out, value.locator.native_id, 8);
	raw(out, { reinterpret_cast<const uint8_t *>(value.locator.name.data()),
		   value.locator.name.size() });
	raw(out, value.creating_operation.bytes);
	raw(out, value.retiring_operation.bytes);
	raw(out, value.last_operation.bytes);
	number(out, value.revision, 8);
	return out;
}
flatfile_economic_mapping decode_mapping(std::span<const uint8_t> encoded)
{
	reader in{ encoded };
	flatfile_economic_mapping value;
	need(economic_account_key_decode(in.take(40), &value.account) ==
	     economic_accounting_error::ok);
	value.locator.kind = in.number(2);
	auto name_size = in.number(2);
	need(name_size <= CURRENCY_ACCOUNT_NAME_MAX_BYTES);
	value.locator.native_id = in.number(8);
	auto name = in.take(name_size);
	value.locator.name.assign(name.begin(), name.end());
	value.creating_operation = in.id();
	value.retiring_operation = in.id();
	value.last_operation = in.id();
	value.revision = in.number(8);
	in.done();
	try
	{
		validate_mapping(value);
	}
	catch (const failure &)
	{
		throw failure{ EILSEQ };
	}
	return value;
}
void validate_mappings(const flatfile_economic_control &control, size_t bucket,
		       const mappings &values)
{
	need(values.size() == mapping_count(control, bucket) &&
	     values.size() <= FLATFILE_ECONOMIC_BUCKET_MAPPINGS);
	uint64_t expected = bucket ? bucket : 256;
	for (const auto &value : values)
	{
		validate_mapping(value);
		need(value.account.lineage.bytes == control.lineage.bytes &&
		     value.account.authority_id == expected);
		expected += 256;
	}
}
bytes encode_mappings(const flatfile_economic_control &control, size_t bucket,
		      const mappings &values)
{
	validate_mappings(control, bucket, values);
	bytes out;
	raw(out, control.lineage.bytes);
	number(out, bucket, 4);
	number(out, values.size(), 4);
	for (const auto &value : values)
	{
		auto entry = encode_mapping(value);
		number(out, entry.size(), 4);
		raw(out, entry);
	}
	return envelope("DURECM1", out);
}
mappings load_mappings(const std::string &root, const flatfile_economic_control &control,
		       size_t bucket)
{
	if (control.mapping_digests[bucket] == economic_digest{})
	{
		need(mapping_count(control, bucket) == 0);
		require_absent(root, mapping_name(bucket));
		return {};
	}
	auto encoded = read_file(root, mapping_name(bucket));
	need(hash(encoded) == control.mapping_digests[bucket]);
	auto in = unwrap(encoded, "DURECM1");
	need(in.id().bytes == control.lineage.bytes && in.number(4) == bucket);
	auto count = in.number(4);
	need(count == mapping_count(control, bucket) && count <= FLATFILE_ECONOMIC_BUCKET_MAPPINGS);
	mappings values;
	values.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		const auto size = in.number(4);
		values.push_back(decode_mapping(in.take(size)));
	}
	in.done();
	validate_mappings(control, bucket, values);
	return values;
}
struct native_entry
{
	bytes key;
	uint64_t active = 0, last = 0;
};
using native_index = std::vector<native_entry>;
void validate_native(const flatfile_economic_control &control, size_t bucket,
		     const native_index &values)
{
	need(values.size() <= FLATFILE_ECONOMIC_BUCKET_MAPPINGS, ENOSPC);
	for (size_t i = 0; i < values.size(); ++i)
	{
		const auto &value = values[i];
		validate_native_key(value.key);
		need(hash(value.key)[0] == bucket && value.last &&
		     value.last < control.next_mapping_id &&
		     (!value.active || value.active == value.last) &&
		     (!i || values[i - 1].key < value.key));
	}
}
bytes encode_native(const flatfile_economic_control &control, size_t bucket,
		    const native_index &values)
{
	validate_native(control, bucket, values);
	bytes out;
	raw(out, control.lineage.bytes);
	number(out, bucket, 4);
	number(out, values.size(), 4);
	for (const auto &value : values)
	{
		number(out, value.key.size(), 2);
		number(out, 0, 2);
		number(out, value.active, 8);
		number(out, value.last, 8);
		raw(out, value.key);
	}
	return envelope("DURECN1", out);
}
native_index load_native(const std::string &root, const flatfile_economic_control &control,
			 size_t bucket)
{
	if (control.native_digests[bucket] == economic_digest{})
	{
		require_absent(root, native_name(bucket));
		throw failure{ ENODATA };
	}
	auto encoded = read_file(root, native_name(bucket));
	need(hash(encoded) == control.native_digests[bucket]);
	auto in = unwrap(encoded, "DURECN1");
	need(in.id().bytes == control.lineage.bytes && in.number(4) == bucket);
	auto count = in.number(4);
	need(count <= FLATFILE_ECONOMIC_BUCKET_MAPPINGS);
	native_index values;
	values.reserve(count);
	for (size_t i = 0; i < count; ++i)
	{
		native_entry entry;
		auto length = in.number(2);
		need(length <= 12 + CURRENCY_ACCOUNT_NAME_MAX_BYTES && in.number(2) == 0);
		entry.active = in.number(8);
		entry.last = in.number(8);
		auto key = in.take(length);
		entry.key.assign(key.begin(), key.end());
		values.push_back(std::move(entry));
	}
	in.done();
	validate_native(control, bucket, values);
	return values;
}
size_t find_native(const native_index &values, const bytes &key)
{
	auto found = std::lower_bound(values.begin(), values.end(), key,
				      [](const auto &value, const auto &expected)
				      { return value.key < expected; });
	return found == values.end() || found->key != key ?
		       values.size() :
		       static_cast<size_t>(found - values.begin());
}
flatfile_economic_mapping mapping_by_id(const std::string &root,
					const flatfile_economic_control &control, uint64_t id)
{
	need(id && id < control.next_mapping_id, ESTALE);
	auto values = load_mappings(root, control, id % 256);
	auto position = (id - 1) / 256;
	need(position < values.size());
	return values[position];
}
flatfile_economic_mapping mapping_for_key(const std::string &root,
					  const flatfile_economic_control &control,
					  const economic_account_key &key)
{
	need(economic_account_key_valid(key), EINVAL);
	need(key.lineage.bytes == control.lineage.bytes, ESTALE);
	auto value = mapping_by_id(root, control, key.authority_id);
	need(economic_account_key_equal(value.account, key), ESTALE);
	return value;
}
void active_crosslink(const std::string &root, const flatfile_economic_control &control,
		      const flatfile_economic_mapping &mapping)
{
	auto key = native_key(mapping.account.kind, mapping.account.context_id, mapping.locator);
	auto index = load_native(root, control, hash(key)[0]);
	auto at = find_native(index, key);
	need(!nonzero(mapping.retiring_operation), ESTALE);
	need(at < index.size() && index[at].active == mapping.account.authority_id);
}
void validate_tombstone(const std::string &root, const flatfile_economic_control &control,
			const native_entry &entry)
{
	need(!entry.active);
	auto last = mapping_by_id(root, control, entry.last);
	auto current = native_key(last.account.kind, last.account.context_id, last.locator);
	need(current.size() >= 12 && entry.key.size() >= 12 &&
	     std::equal(current.begin(), current.begin() + 12, entry.key.begin()));
	if (last.account.kind == economic_account_kind::wallet)
		need(current == entry.key);
	if (!nonzero(last.retiring_operation))
	{
		need(last.account.kind == economic_account_kind::bank && current != entry.key);
		active_crosslink(root, control, last);
	}
}
void changing(flatfile_economic_control &control, uint64_t expected,
	      const critical_operation_id &operation)
{
	need(control.revision == expected, ESTALE);
	need(nonzero(operation), EINVAL);
	need(control.revision != UINT64_MAX, EOVERFLOW);
	++control.revision;
	control.last_operation = operation;
}
using updates = std::map<std::string, bytes>;
void put_mapping(flatfile_economic_control &control, size_t bucket, const mappings &values,
		 updates &files)
{
	auto encoded = encode_mappings(control, bucket, values);
	control.mapping_digests[bucket] = hash(encoded);
	need(files.emplace(mapping_name(bucket), std::move(encoded)).second);
}
void put_native(flatfile_economic_control &control, size_t bucket, const native_index &values,
		updates &files)
{
	auto encoded = encode_native(control, bucket, values);
	control.native_digests[bucket] = hash(encoded);
	need(files.emplace(native_name(bucket), std::move(encoded)).second);
}
void finish(const flatfile_economic_control &control, updates files, operations *out)
{
	need(out, EINVAL);
	need(files.emplace("authority.eal", encode_control(control)).second);
	need(files.size() + out->size() <= flatfile_authority_transaction_maximum_operations,
	     ENOSPC);
	size_t total = 50;
	std::set<std::pair<flatfile_authority_store, std::string>> destinations;
	auto count =
		[&](flatfile_authority_store store, const std::string &name, const bytes &encoded)
	{
		need(destinations.emplace(store, name).second, EINVAL);
		need(name.size() <= 192 &&
			     encoded.size() <= flatfile_authority_transaction_maximum_bytes,
		     ENOSPC);
		size_t more = 8 + name.size() + encoded.size();
		need(more <= flatfile_authority_transaction_maximum_bytes - total, ENOSPC);
		total += more;
	};
	for (const auto &op : *out)
		count(op.store, op.filename, op.bytes);
	for (const auto &[name, encoded] : files)
		count(flatfile_authority_store::economic_evidence, name, encoded);
	auto candidate = *out;
	for (auto &[name, encoded] : files)
		candidate.push_back({ flatfile_authority_store::economic_evidence,
				      flatfile_authority_operation_kind::write, name,
				      std::move(encoded) });
	*out = std::move(candidate);
}
void require_empty(const std::string &root)
{
	const int fd =
		open(directory(root).c_str(), O_RDONLY | O_DIRECTORY | O_CLOEXEC | O_NOFOLLOW);
	need(fd >= 0, EIO);
	struct stat info = {};
	const bool safe = fstat(fd, &info) == 0 && S_ISDIR(info.st_mode) &&
			  info.st_uid == geteuid() && !(info.st_mode & 0077);
	if (!safe)
	{
		close(fd);
		throw failure{ EILSEQ };
	}
	DIR *dir = fdopendir(fd);
	if (!dir)
	{
		close(fd);
		throw failure{ EIO };
	}
	bool empty = true;
	errno = 0;
	while (auto *entry = readdir(dir))
		if (strcmp(entry->d_name, ".") && strcmp(entry->d_name, ".."))
		{
			empty = false;
			break;
		}
	const auto error = errno;
	closedir(dir);
	need(!error, EIO);
	need(empty, EEXIST);
}
}
unsigned int flatfile_economic_control_read(const std::string &root,
					    const flatfile_authority_lock &lock,
					    flatfile_economic_control *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out, EINVAL);
			recover(root, lock);
			auto value = load_control(root);
			*out = std::move(value);
		},
		error);
}
unsigned int flatfile_economic_epoch_read(const std::string &root,
					  const flatfile_authority_lock &lock,
					  const critical_operation_id &lineage,
					  const critical_operation_id &epoch,
					  flatfile_economic_epoch *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out && nonzero(lineage) && nonzero(epoch), EINVAL);
			recover(root, lock);
			const auto control = load_control(root);
			need(control.lineage.bytes == lineage.bytes, ESTALE);
			const auto values = load_epochs(root, control);
			const auto at = std::find_if(values.begin(), values.end(),
						     [&](const auto &value)
						     { return value.epoch.bytes == epoch.bytes; });
			need(at != values.end(), ENODATA);
			*out = *at;
		},
		error);
}
unsigned int flatfile_economic_mapping_read(const std::string &root,
					    const flatfile_authority_lock &lock,
					    const economic_account_key &key,
					    flatfile_economic_mapping *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out, EINVAL);
			recover(root, lock);
			auto control = load_control(root);
			auto value = mapping_for_key(root, control, key);
			*out = std::move(value);
		},
		error);
}
unsigned int flatfile_economic_native_lookup(const std::string &root,
					     const flatfile_authority_lock &lock,
					     economic_account_kind kind, uint64_t context,
					     const flatfile_economic_locator &locator,
					     flatfile_economic_mapping *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out, EINVAL);
			auto key = native_key(kind, context, locator);
			recover(root, lock);
			auto control = load_control(root);
			auto index = load_native(root, control, hash(key)[0]);
			auto at = find_native(index, key);
			need(at < index.size(), ENODATA);
			if (!index[at].active)
			{
				validate_tombstone(root, control, index[at]);
				throw failure{ ENODATA };
			}
			auto value = mapping_by_id(root, control, index[at].active);
			need(!nonzero(value.retiring_operation) &&
			     native_key(value.account.kind, value.account.context_id,
					value.locator) == key);
			*out = std::move(value);
		},
		error);
}
unsigned int
economic_flatfile_lock_authority(const std::string &root, const flatfile_authority_lock &lock,
				 const critical_operation_id &lineage,
				 const critical_operation_id &epoch,
				 std::span<const flatfile_economic_mapping_request> requests,
				 flatfile_economic_authority_snapshot *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out && nonzero(lineage) && nonzero(epoch), EINVAL);
			need(requests.size() <= ECONOMIC_ACCOUNTING_MAX_ACCOUNTS, E2BIG);
			recover(root, lock);
			auto control = load_control(root);
			need(control.lineage.bytes == lineage.bytes, ESTALE);
			need(nonzero(control.active_epoch), ENODATA);
			need(control.active_epoch.bytes == epoch.bytes, ESTALE);
			std::set<uint64_t> ids;
			std::map<size_t, std::vector<const flatfile_economic_mapping_request *>>
				groups;
			for (const auto &request : requests)
			{
				need(economic_account_key_valid(request.account) &&
					     ids.insert(request.account.authority_id).second,
				     EINVAL);
				need(request.account.lineage.bytes == lineage.bytes &&
					     request.account.authority_id < control.next_mapping_id,
				     ESTALE);
				groups[request.account.authority_id % 256].push_back(&request);
			}
			flatfile_economic_authority_snapshot candidate;
			candidate.lineage = lineage;
			candidate.epoch = epoch;
			candidate.lineage_revision = control.revision;
			candidate.mappings.reserve(requests.size());
			// One decoded bucket at a time, retaining only the requested mappings.
			for (const auto &[bucket, group] : groups)
			{
				auto values = load_mappings(root, control, bucket);
				for (const auto *request : group)
				{
					const auto &value = values.at(
						(request->account.authority_id - 1) / 256);
					need(economic_account_key_equal(value.account,
									request->account) &&
						     value.locator.kind == request->locator.kind &&
						     value.locator.native_id ==
							     request->locator.native_id &&
						     value.locator.name == request->locator.name &&
						     !nonzero(value.retiring_operation),
					     ESTALE);
					candidate.mappings.push_back(value);
				}
			}
			std::map<size_t, std::vector<size_t>> native_groups;
			for (size_t i = 0; i < candidate.mappings.size(); ++i)
			{
				const auto &value = candidate.mappings[i];
				native_groups[hash(native_key(value.account.kind,
							      value.account.context_id,
							      value.locator))[0]]
					.push_back(i);
			}
			for (const auto &[bucket, group] : native_groups)
			{
				auto index = load_native(root, control, bucket);
				for (auto i : group)
				{
					const auto &value = candidate.mappings[i];
					auto key = native_key(value.account.kind,
							      value.account.context_id,
							      value.locator);
					auto at = find_native(index, key);
					need(at < index.size() &&
					     index[at].active == value.account.authority_id);
				}
			}
			std::sort(candidate.mappings.begin(), candidate.mappings.end(),
				  [](const auto &a, const auto &b)
				  { return a.account.authority_id < b.account.authority_id; });
			*out = std::move(candidate);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::bootstrap(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, const critical_operation_id &operation,
	operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(nonzero(lineage) && nonzero(operation), EINVAL);
			recover(root, lock);
			require_empty(root);
			flatfile_economic_control control;
			control.lineage = lineage;
			control.creating_operation = control.last_operation = operation;
			auto catalog = encode_epochs(lineage, {});
			control.epochs_digest = hash(catalog);
			finish(control, { { "epochs.eae", std::move(catalog) } }, out);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::initialize_native_bucket(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	size_t bucket, const critical_operation_id &operation, operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(bucket < 256, EINVAL);
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, operation);
			if (control.native_digests[bucket] != economic_digest{})
			{
				(void)load_native(root, control, bucket);
				throw failure{ EALREADY };
			}
			require_absent(root, native_name(bucket));
			updates files;
			put_native(control, bucket, {}, files);
			finish(control, std::move(files), out);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::initialize_evidence_bucket(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	size_t bucket, const critical_operation_id &operation, operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out && bucket < 256, EINVAL);
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, operation);
			if (control.evidence_initialized[bucket / 8] & (1u << (bucket % 8)))
			{
				const auto checked = flatfile_accounting_check_bucket(
					root, lock, control.lineage, bucket, error);
				need(checked == flatfile_accounting_status::ok,
				     checked == flatfile_accounting_status::capacity ? ENOMEM :
				     checked == flatfile_accounting_status::io_error ? EIO :
										       EILSEQ);
				throw failure{ EALREADY };
			}
			auto candidate = *out;
			auto result = flatfile_accounting_storage::initialize_bucket(
				root, lock, control.lineage, bucket, &candidate, error);
			need(result == flatfile_accounting_status::ok,
			     result == flatfile_accounting_status::io_error ? EIO :
			     result == flatfile_accounting_status::capacity ? ENOSPC :
									      EILSEQ);
			control.evidence_initialized[bucket / 8] |=
				static_cast<uint8_t>(1u << (bucket % 8));
			finish(control, {}, &candidate);
			*out = std::move(candidate);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::create_mapping(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	economic_account_kind kind, uint64_t context, const flatfile_economic_locator &locator,
	const critical_operation_id &operation, flatfile_economic_mapping *mapping, operations *out,
	std::string *error)
{
	return guarded(
		[&]
		{
			need(mapping && out, EINVAL);
			locator_valid(kind, context, locator, true);
			auto key = native_key(kind, context, locator);
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, operation);
			need(control.next_mapping_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS, ENOSPC);
			auto native_bucket = hash(key)[0];
			auto index = load_native(root, control, native_bucket);
			auto at = find_native(index, key);
			need(at == index.size() || !index[at].active, EEXIST);
			if (at < index.size())
				validate_tombstone(root, control, index[at]);
			auto id = control.next_mapping_id;
			auto bucket = id % 256;
			auto values = load_mappings(root, control, bucket);
			flatfile_economic_mapping value;
			value.account = { control.lineage, kind, id, context };
			value.locator = locator;
			if (kind == economic_account_kind::bank)
				value.locator.native_id = id;
			value.creating_operation = value.last_operation = operation;
			values.push_back(value);
			++control.next_mapping_id;
			if (at == index.size())
			{
				need(index.size() < FLATFILE_ECONOMIC_BUCKET_MAPPINGS, ENOSPC);
				index.push_back({ key, id, id });
			}
			else
				index[at].active = index[at].last = id;
			std::sort(index.begin(), index.end(),
				  [](const auto &a, const auto &b) { return a.key < b.key; });
			updates files;
			put_mapping(control, bucket, values, files);
			put_native(control, native_bucket, index, files);
			finish(control, std::move(files), out);
			*mapping = std::move(value);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::retire_mapping(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const economic_account_key &key, uint64_t revision, const critical_operation_id &operation,
	operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, operation);
			auto value = mapping_for_key(root, control, key);
			need(value.revision == revision && !nonzero(value.retiring_operation),
			     ESTALE);
			need(value.revision != UINT64_MAX, EOVERFLOW);
			active_crosslink(root, control, value);
			auto native = native_key(value.account.kind, value.account.context_id,
						 value.locator);
			auto bucket = hash(native)[0];
			auto index = load_native(root, control, bucket);
			index[find_native(index, native)].active = 0;
			value.retiring_operation = value.last_operation = operation;
			++value.revision;
			auto values = load_mappings(root, control, key.authority_id % 256);
			values[(key.authority_id - 1) / 256] = value;
			updates files;
			put_mapping(control, key.authority_id % 256, values, files);
			put_native(control, bucket, index, files);
			finish(control, std::move(files), out);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::rename_bank(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const economic_account_key &key, uint64_t revision, const std::string &name,
	const critical_operation_id &operation, operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			name_valid(name);
			need(key.kind == economic_account_kind::bank, EINVAL);
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, operation);
			auto value = mapping_for_key(root, control, key);
			need(value.revision == revision && !nonzero(value.retiring_operation),
			     ESTALE);
			active_crosslink(root, control, value);
			need(name != value.locator.name, EALREADY);
			need(value.revision != UINT64_MAX, EOVERFLOW);
			auto old_key = native_key(key.kind, key.context_id, value.locator);
			auto old_bucket = hash(old_key)[0];
			value.locator.name = name;
			auto new_key = native_key(key.kind, key.context_id, value.locator);
			auto new_bucket = hash(new_key)[0];
			std::map<size_t, native_index> indexes;
			indexes.emplace(old_bucket, load_native(root, control, old_bucket));
			if (new_bucket != old_bucket)
				indexes.emplace(new_bucket, load_native(root, control, new_bucket));
			auto &old_index = indexes.at(old_bucket);
			auto &new_index = indexes.at(new_bucket);
			auto at = find_native(new_index, new_key);
			need(at == new_index.size() || !new_index[at].active, EEXIST);
			if (at < new_index.size())
				validate_tombstone(root, control, new_index[at]);
			old_index[find_native(old_index, old_key)].active = 0;
			if (at == new_index.size())
			{
				need(new_index.size() < FLATFILE_ECONOMIC_BUCKET_MAPPINGS, ENOSPC);
				new_index.push_back(
					{ new_key, key.authority_id, key.authority_id });
			}
			else
				new_index[at].active = new_index[at].last = key.authority_id;
			std::sort(new_index.begin(), new_index.end(),
				  [](const auto &a, const auto &b) { return a.key < b.key; });
			value.last_operation = operation;
			++value.revision;
			auto values = load_mappings(root, control, key.authority_id % 256);
			values[(key.authority_id - 1) / 256] = value;
			updates files;
			put_mapping(control, key.authority_id % 256, values, files);
			for (const auto &[bucket, index] : indexes)
				put_native(control, bucket, index, files);
			finish(control, std::move(files), out);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::append_epoch(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const flatfile_economic_epoch &epoch, operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, epoch.creating_operation);
			auto values = load_epochs(root, control);
			need(values.size() < FLATFILE_ECONOMIC_MAX_EPOCHS, ENOSPC);
			values.push_back(epoch);
			auto encoded = encode_epochs(control.lineage, values);
			control.epoch_count = values.size();
			control.last_epoch = epoch.epoch;
			control.active_epoch = {};
			control.epochs_digest = hash(encoded);
			finish(control, { { "epochs.eae", std::move(encoded) } }, out);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::select_epoch(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	bool active, const critical_operation_id &operation, operations *out, std::string *error)
{
	return guarded(
		[&]
		{
			recover(root, lock);
			auto control = load_control(root);
			changing(control, expected, operation);
			need(control.epoch_count, ENODATA);
			need(nonzero(control.active_epoch) != active, EALREADY);
			control.active_epoch = active ? control.last_epoch :
							critical_operation_id{};
			finish(control, {}, out);
		},
		error);
}
