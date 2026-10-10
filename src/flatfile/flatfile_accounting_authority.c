#include "flatfile/flatfile_accounting_authority.h"
#include "flatfile/flatfile_native_mobile_birth_ordinary_initial.h"
#include "economy/native_mobile_birth_cash_role_command.h"
#include "flatfile/flatfile_ordinary_native_birth_receipt.h"
#include "flatfile/flatfile_native_mobile_wallet.h"
#include "flatfile/quest_mobile_native_flatfile.h"
#include <type_traits>
#include <utility>
#include "flatfile/flatfile_accounting_staging_view.h"
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
bytes envelope(const char *magic, const bytes &body, uint32_t version = 1)
{
	need(body.size() + 48 <= FLATFILE_ECONOMIC_METADATA_MAX_BYTES, ENOSPC);
	bytes out;
	raw(out, { reinterpret_cast<const uint8_t *>(magic), 8 });
	number(out, version, 4);
	number(out, body.size(), 4);
	raw(out, hash(body));
	raw(out, body);
	return out;
}
reader unwrap(const bytes &encoded, const char *magic, uint32_t *catalog_version = nullptr)
{
	need(encoded.size() >= 48 && encoded.size() <= FLATFILE_ECONOMIC_METADATA_MAX_BYTES);
	reader in{ encoded };
	auto prefix = in.take(8);
	need(!memcmp(prefix.data(), magic, 8));
	const auto version = in.number(4);
	need((version == 1 || (catalog_version && (version == 2 || version == 3))) &&
	     in.number(4) == encoded.size() - 48);
	if (catalog_version)
		*catalog_version = version;
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
bytes read_file(const std::string &root, const std::string &name,
		const flatfile_accounting_staging_view *view = nullptr)
{
	bytes value;
	if (view)
	{
		bool found = false;
		const auto code =
			view->read(name, FLATFILE_ECONOMIC_METADATA_MAX_BYTES, &value, &found);
		need(!code, code);
		if (found)
			return value;
	}
	errno = 0;
	auto result = flatfile_read(directory(root), name, FLATFILE_ECONOMIC_METADATA_MAX_BYTES,
				    &value, nullptr);
	need(result == flatfile_read_result::ok,
	     result == flatfile_read_result::io_error ? (errno == ENOMEM ? ENOMEM : EIO) : EILSEQ);
	return value;
}
void require_absent(const std::string &root, const std::string &name,
		    const flatfile_accounting_staging_view *view = nullptr)
{
	if (view)
	{
		bytes ignored;
		bool found = false;
		const auto code =
			view->read(name, FLATFILE_ECONOMIC_METADATA_MAX_BYTES, &ignored, &found);
		need(!code, code);
		need(!found);
	}
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
void validate_epochs(const critical_operation_id &lineage, const epochs &values)
{
	need(values.size() <= FLATFILE_ECONOMIC_MAX_EPOCHS, ENOSPC);
	std::set<std::array<uint8_t, 16>> identities;
	std::set<std::array<uint8_t, 16>> lifecycle_operations;
	critical_operation_id previous = {};
	for (size_t i = 0; i < values.size(); ++i)
	{
		const auto &value = values[i];
		need(nonzero(value.epoch) && nonzero(value.creating_operation) &&
		     value.transition_kind && value.transition_digest != economic_digest{} &&
		     value.ordinal == i + 1 && value.predecessor.bytes == previous.bytes &&
		     identities.insert(value.epoch.bytes).second);
		const auto state = value.baseline_initialization;
		need(state == flatfile_baseline_initialization::legacy_unknown ||
		     state == flatfile_baseline_initialization::never_initialized ||
		     state == flatfile_baseline_initialization::initialized);
		const auto origin = value.initialization_origin;
		need(origin == flatfile_baseline_initialization_origin::legacy_unknown ||
		     origin == flatfile_baseline_initialization_origin::baseline_participant ||
		     origin == flatfile_baseline_initialization_origin::lifecycle_owner);
		need(origin == flatfile_baseline_initialization_origin::legacy_unknown ||
		     state == flatfile_baseline_initialization::initialized);
		if (origin == flatfile_baseline_initialization_origin::lifecycle_owner)
			need(value.baseline_initializing_operation.bytes ==
				     value.creating_operation.bytes &&
			     value.transition_kind == 1 &&
			     lifecycle_operations
				     .insert(value.baseline_initializing_operation.bytes)
				     .second);
		if (state == flatfile_baseline_initialization::initialized)
			need(nonzero(value.baseline_initializing_operation) &&
			     economic_account_key_valid(value.baseline_opening) &&
			     value.baseline_opening.kind == economic_account_kind::opening &&
			     value.baseline_opening.lineage.bytes == lineage.bytes);
		else
			need(!nonzero(value.baseline_initializing_operation) &&
			     !nonzero(value.baseline_opening.lineage) &&
			     value.baseline_opening.kind == economic_account_kind{} &&
			     !value.baseline_opening.authority_id &&
			     !value.baseline_opening.context_id);
		previous = value.epoch;
	}
}
bytes encode_epochs(const critical_operation_id &lineage, const epochs &values)
{
	validate_epochs(lineage, values);
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
		number(out, static_cast<uint8_t>(value.baseline_initialization), 1);
		number(out, static_cast<uint8_t>(value.initialization_origin), 1);
		number(out, 0, 6);
		raw(out, value.baseline_initializing_operation.bytes);
		std::array<uint8_t, ECONOMIC_ACCOUNT_KEY_BYTES> opening = {};
		if (value.baseline_initialization == flatfile_baseline_initialization::initialized)
			need(economic_account_key_encode(value.baseline_opening, &opening) ==
			     economic_accounting_error::ok);
		raw(out, opening);
	}
	return envelope("DURECE1", out, 3);
}
epochs load_epochs(const std::string &root, const flatfile_economic_control &control,
		   const flatfile_accounting_staging_view *view = nullptr)
{
	auto encoded = read_file(root, "epochs.eae", view);
	need(hash(encoded) == control.epochs_digest);
	uint32_t version = 0;
	auto in = unwrap(encoded, "DURECE1", &version);
	need(in.id().bytes == control.lineage.bytes);
	auto count = in.number(4);
	need(count == control.epoch_count && count <= FLATFILE_ECONOMIC_MAX_EPOCHS &&
	     in.number(4) == 0 && in.data.size() == 24 + count * (version == 1 ? 96 : 160));
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
		if (version >= 2)
		{
			value.baseline_initialization =
				static_cast<flatfile_baseline_initialization>(in.number(1));
			if (version == 3)
			{
				value.initialization_origin =
					static_cast<flatfile_baseline_initialization_origin>(
						in.number(1));
				need(in.number(6) == 0);
			}
			else
				need(in.number(7) == 0);
			value.baseline_initializing_operation = in.id();
			auto opening = in.take(ECONOMIC_ACCOUNT_KEY_BYTES);
			if (value.baseline_initialization ==
			    flatfile_baseline_initialization::initialized)
				need(economic_account_key_decode(opening,
								 &value.baseline_opening) ==
				     economic_accounting_error::ok);
			else
				need(std::all_of(opening.begin(), opening.end(),
						 [](uint8_t byte) { return byte == 0; }));
		}
		values.push_back(value);
	}
	in.done();
	validate_epochs(control.lineage, values);
	need(values.empty() || values.back().epoch.bytes == control.last_epoch.bytes);
	return values;
}
flatfile_economic_control load_control(const std::string &root,
				       const flatfile_accounting_staging_view *view = nullptr)
{
	auto value = decode_control(read_file(root, "authority.eal", view));
	(void)load_epochs(root, value, view);
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
	if (kind == economic_account_kind::wallet &&
	    context == ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT)
	{
		need(flatfile_native_mobile_wallet_locator_valid(kind, context, locator), EINVAL);
	}
	else if (kind == economic_account_kind::wallet ||
		 kind == economic_account_kind::auction_escrow ||
		 kind == economic_account_kind::pending_claim ||
		 kind == economic_account_kind::treasury)
	{
		const uint64_t limit = kind == economic_account_kind::treasury ?
					       uint64_t{ UINT32_MAX } + 1 :
				       kind == economic_account_kind::auction_escrow ? UINT32_MAX :
										       INT32_MAX;
		need(context == 0 && locator.kind == static_cast<uint16_t>(kind) &&
			     locator.native_id > 0 && locator.native_id <= limit &&
			     locator.name.empty(),
		     EINVAL);
	}
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
	if (kind != economic_account_kind::bank)
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
	if (kind != economic_account_kind::bank)
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
		       size_t bucket, const flatfile_accounting_staging_view *view = nullptr)
{
	if (control.mapping_digests[bucket] == economic_digest{})
	{
		need(mapping_count(control, bucket) == 0);
		require_absent(root, mapping_name(bucket), view);
		return {};
	}
	auto encoded = read_file(root, mapping_name(bucket), view);
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
			 size_t bucket, const flatfile_accounting_staging_view *view = nullptr)
{
	if (control.native_digests[bucket] == economic_digest{})
	{
		require_absent(root, native_name(bucket), view);
		throw failure{ ENODATA };
	}
	auto encoded = read_file(root, native_name(bucket), view);
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
					const flatfile_economic_control &control, uint64_t id,
					const flatfile_accounting_staging_view *view = nullptr)
{
	need(id && id < control.next_mapping_id, ESTALE);
	auto values = load_mappings(root, control, id % 256, view);
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
		      const flatfile_economic_mapping &mapping,
		      const flatfile_accounting_staging_view *view = nullptr)
{
	auto key = native_key(mapping.account.kind, mapping.account.context_id, mapping.locator);
	auto index = load_native(root, control, hash(key)[0], view);
	auto at = find_native(index, key);
	need(!nonzero(mapping.retiring_operation), ESTALE);
	need(at < index.size() && index[at].active == mapping.account.authority_id);
}
void validate_tombstone_identity(const native_entry &entry, const flatfile_economic_mapping &last)
{
	need(!entry.active);
	const auto current = native_key(last.account.kind, last.account.context_id, last.locator);
	need(current.size() >= 12 && entry.key.size() >= 12 &&
	     std::equal(current.begin(), current.begin() + 12, entry.key.begin()));
	if (last.account.kind != economic_account_kind::bank)
		need(current == entry.key);
	if (!nonzero(last.retiring_operation))
		need(last.account.kind == economic_account_kind::bank && current != entry.key);
}
void validate_tombstone(const std::string &root, const flatfile_economic_control &control,
			const native_entry &entry,
			const flatfile_accounting_staging_view *view = nullptr)
{
	auto last = mapping_by_id(root, control, entry.last, view);
	validate_tombstone_identity(entry, last);
	auto current = native_key(last.account.kind, last.account.context_id, last.locator);
	if (!nonzero(last.retiring_operation))
	{
		need(last.account.kind == economic_account_kind::bank && current != entry.key);
		active_crosslink(root, control, last, view);
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
void finish(const flatfile_economic_control &control, updates files, operations *out,
	    flatfile_accounting_staging_view *view = nullptr)
{
	need(out, EINVAL);
	need(files.emplace("authority.eal", encode_control(control)).second);
	if (view)
	{
		operations changes;
		changes.reserve(files.size());
		for (auto &[name, encoded] : files)
			changes.push_back({ flatfile_authority_store::economic_evidence,
					    flatfile_authority_operation_kind::write, name,
					    std::move(encoded) });
		const auto code = view->merge(changes, out);
		need(!code, code);
		return;
	}
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
bool evidence_directory_empty(const std::string &root)
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
	return empty;
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
unsigned int flatfile_economic_legacy_domain_gate(const std::string &root,
						  const flatfile_authority_lock &lock,
						  std::string *error)
{
	return guarded(
		[&]
		{
			need(!root.empty() && lock.matches(root), EINVAL);
			struct stat info = {};
			const auto path = directory(root);
			if (stat(path.c_str(), &info))
			{
				need(errno == ENOENT, EIO);
				return;
			}
			need(S_ISDIR(info.st_mode));
			recover(root, lock);
			// Boot creates this private directory before accounting is initialized.
			// Any retained evidence still requires a valid inactive control record.
			if (evidence_directory_empty(root))
				return;
			const auto control = load_control(root);
			need(!nonzero(control.active_epoch), EAGAIN);
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
			need(evidence_directory_empty(root), EEXIST);
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
	return create_mapping_staged(root, lock, expected, kind, context, locator, operation,
				     mapping, out, error, nullptr);
}
unsigned int flatfile_accounting_authority_storage::create_mapping_staged(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	economic_account_kind kind, uint64_t context, const flatfile_economic_locator &locator,
	const critical_operation_id &operation, flatfile_economic_mapping *mapping, operations *out,
	std::string *error, flatfile_accounting_staging_view *view)
{
	return guarded(
		[&]
		{
			need(mapping && out, EINVAL);
			if (view)
			{
				const auto code = view->begin(root, lock, out);
				need(!code, code);
			}
			locator_valid(kind, context, locator, true);
			auto key = native_key(kind, context, locator);
			recover(root, lock);
			auto control = load_control(root, view);
			changing(control, expected, operation);
			need(control.next_mapping_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS, ENOSPC);
			auto native_bucket = hash(key)[0];
			auto index = load_native(root, control, native_bucket, view);
			auto at = find_native(index, key);
			need(at == index.size() || !index[at].active, EEXIST);
			if (at < index.size())
				validate_tombstone(root, control, index[at], view);
			auto id = control.next_mapping_id;
			auto bucket = id % 256;
			auto values = load_mappings(root, control, bucket, view);
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
			finish(control, std::move(files), out, view);
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
	return append_epoch_staged(root, lock, expected, epoch, out, error, nullptr);
}
unsigned int flatfile_accounting_authority_storage::append_epoch_staged(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const flatfile_economic_epoch &epoch, operations *out, std::string *error,
	flatfile_accounting_staging_view *view)
{
	return guarded(
		[&]
		{
			if (view)
			{
				const auto code = view->begin(root, lock, out);
				need(!code, code);
			}
			recover(root, lock);
			auto control = load_control(root, view);
			changing(control, expected, epoch.creating_operation);
			auto values = load_epochs(root, control, view);
			need(values.size() < FLATFILE_ECONOMIC_MAX_EPOCHS, ENOSPC);
			// Callers cannot supply an initialized marker or its participant origin.
			need(epoch.initialization_origin ==
					     flatfile_baseline_initialization_origin::legacy_unknown &&
				     epoch.baseline_initialization ==
					     flatfile_baseline_initialization::legacy_unknown &&
				     !nonzero(epoch.baseline_initializing_operation) &&
				     !nonzero(epoch.baseline_opening.lineage) &&
				     epoch.baseline_opening.kind == economic_account_kind{} &&
				     !epoch.baseline_opening.authority_id &&
				     !epoch.baseline_opening.context_id,
			     EINVAL);
			auto appended = epoch;
			appended.baseline_initialization =
				flatfile_baseline_initialization::never_initialized;
			values.push_back(appended);
			auto encoded = encode_epochs(control.lineage, values);
			control.epoch_count = values.size();
			control.last_epoch = epoch.epoch;
			control.active_epoch = {};
			control.epochs_digest = hash(encoded);
			finish(control, { { "epochs.eae", std::move(encoded) } }, out, view);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::stage_baseline_initialization(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const economic_account_key &opening, const critical_operation_id &operation,
	operations *out, std::string *error)
{
	return stage_baseline_initialization_staged(root, lock, expected, lineage, epoch, opening,
						    operation, out, error, nullptr);
}
unsigned int flatfile_accounting_authority_storage::stage_baseline_initialization_staged(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const economic_account_key &opening, const critical_operation_id &operation,
	operations *out, std::string *error, flatfile_accounting_staging_view *view)
{
	return stage_baseline_initialization_with_origin_staged(
		root, lock, expected, lineage, epoch, opening, operation, out, error, view,
		flatfile_baseline_initialization_origin::baseline_participant);
}
unsigned int
flatfile_accounting_authority_storage::stage_baseline_initialization_with_origin_staged(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const economic_account_key &opening, const critical_operation_id &operation,
	operations *out, std::string *error, flatfile_accounting_staging_view *view,
	flatfile_baseline_initialization_origin origin)
{
	return guarded(
		[&]
		{
			if (view)
			{
				const auto code = view->begin(root, lock, out);
				need(!code, code);
			}
			recover(root, lock);
			need(origin == flatfile_baseline_initialization_origin::baseline_participant ||
				     origin ==
					     flatfile_baseline_initialization_origin::lifecycle_owner,
			     EINVAL);
			need(out && nonzero(lineage) && nonzero(epoch) &&
				     economic_account_key_valid(opening) &&
				     opening.kind == economic_account_kind::opening &&
				     opening.lineage.bytes == lineage.bytes,
			     EINVAL);
			auto control = load_control(root, view);
			need(control.lineage.bytes == lineage.bytes, ESTALE);
			changing(control, expected, operation);
			auto values = load_epochs(root, control, view);
			auto at = std::find_if(values.begin(), values.end(), [&](const auto &value)
					       { return value.epoch.bytes == epoch.bytes; });
			need(at != values.end(), ENODATA);
			need(at->baseline_initialization ==
				     flatfile_baseline_initialization::never_initialized,
			     EILSEQ);
			need(at->initialization_origin ==
				     flatfile_baseline_initialization_origin::legacy_unknown,
			     EILSEQ);
			at->initialization_origin = origin;
			at->baseline_initialization = flatfile_baseline_initialization::initialized;
			at->baseline_initializing_operation = operation;
			at->baseline_opening = opening;
			auto encoded = encode_epochs(control.lineage, values);
			control.epochs_digest = hash(encoded);
			finish(control, { { "epochs.eae", std::move(encoded) } }, out, view);
		},
		error);
}
unsigned int flatfile_accounting_authority_storage::select_epoch(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	bool active, const critical_operation_id &operation, operations *out, std::string *error)
{
	return select_epoch_staged(root, lock, expected, active, operation, out, error, nullptr);
}
unsigned int flatfile_accounting_authority_storage::select_epoch_staged(
	const std::string &root, const flatfile_authority_lock &lock, uint64_t expected,
	bool active, const critical_operation_id &operation, operations *out, std::string *error,
	flatfile_accounting_staging_view *view)
{
	return guarded(
		[&]
		{
			if (view)
			{
				const auto code = view->begin(root, lock, out);
				need(!code, code);
			}
			recover(root, lock);
			auto control = load_control(root, view);
			changing(control, expected, operation);
			need(control.epoch_count, ENODATA);
			need(nonzero(control.active_epoch) != active, EALREADY);
			control.active_epoch = active ? control.last_epoch :
							critical_operation_id{};
			finish(control, {}, out, view);
		},
		error);
}

unsigned int flatfile_accounting_authority_storage::read_control(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_staging_view *view, flatfile_economic_control *out,
	std::string *error)
{
	return guarded(
		[&]
		{
			need(view && out && view->matches(root, lock), EINVAL);
			recover(root, lock);
			auto value = load_control(root, view);
			*out = std::move(value);
		},
		error);
}

unsigned int flatfile_accounting_authority_storage::read_epoch(
	const std::string &root, const flatfile_authority_lock &lock,
	const flatfile_accounting_staging_view *view, const critical_operation_id &lineage,
	const critical_operation_id &epoch, flatfile_economic_epoch *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(view && out && view->matches(root, lock) && nonzero(lineage) &&
				     nonzero(epoch),
			     EINVAL);
			recover(root, lock);
			const auto control = load_control(root, view);
			need(control.lineage.bytes == lineage.bytes, ESTALE);
			const auto values = load_epochs(root, control, view);
			const auto at = std::find_if(values.begin(), values.end(),
						     [&](const auto &value)
						     { return value.epoch.bytes == epoch.bytes; });
			need(at != values.end(), ENODATA);
			*out = *at;
		},
		error);
}

unsigned int flatfile_accounting_staging_view::control(flatfile_economic_control *out,
						       std::string *error) const
{
	return flatfile_accounting_authority_storage::read_control(root_, lock_, this, out, error);
}

unsigned int flatfile_accounting_staging_view::epoch(const critical_operation_id &lineage,
						     const critical_operation_id &epoch,
						     flatfile_economic_epoch *out,
						     std::string *error) const
{
	return flatfile_accounting_authority_storage::read_epoch(root_, lock_, this, lineage, epoch,
								 out, error);
}

unsigned int flatfile_accounting_authority_storage::read_all_mappings_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	std::vector<flatfile_economic_mapping> *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out && !root.empty() && lock.matches(root), EINVAL);
			recover(root, lock);
			const auto control = load_control(root);
			mappings all;
			all.reserve(control.next_mapping_id - 1);
			std::array<native_index, FLATFILE_ECONOMIC_METADATA_BUCKETS> indexes;
			for (size_t slot = 0; slot < indexes.size(); ++slot)
			{
				auto values = load_mappings(root, control, slot);
				for (auto &value : values)
					all.push_back(std::move(value));
				indexes[slot] = load_native(root, control, slot);
			}
			need(all.size() == control.next_mapping_id - 1);
			std::sort(all.begin(), all.end(), [](const auto &a, const auto &b)
				  { return a.account.authority_id < b.account.authority_id; });
			std::map<bytes, const native_entry *> links;
			for (const auto &index : indexes)
				for (const auto &entry : index)
					need(links.emplace(entry.key, &entry).second);
			auto mapping = [&](uint64_t id) -> const flatfile_economic_mapping &
			{
				need(id && id <= all.size());
				const auto &value = all[id - 1];
				need(value.account.authority_id == id);
				return value;
			};
			auto current_link =
				[&](const flatfile_economic_mapping &value) -> const native_entry &
			{
				const auto key = native_key(value.account.kind,
							    value.account.context_id,
							    value.locator);
				const auto at = links.find(key);
				need(at != links.end());
				return *at->second;
			};
			for (size_t i = 0; i < all.size(); ++i)
			{
				const auto &value = mapping(i + 1);
				const auto &entry = current_link(value);
				if (!nonzero(value.retiring_operation))
					need(entry.active == value.account.authority_id &&
					     entry.last == entry.active);
				else
					// An existing bank may rename into a retired alias even when
					// its lifetime ID is older. last identifies the latest alias
					// assignment, not monotonically increasing bank lifetimes.
					need((value.account.kind == economic_account_kind::bank ||
					      entry.last >= value.account.authority_id) &&
					     entry.active != value.account.authority_id);
			}
			for (const auto &[key, pointer] : links)
			{
				const auto &entry = *pointer;
				const auto &last = mapping(entry.last);
				if (entry.active)
					need(!nonzero(last.retiring_operation) &&
					     native_key(last.account.kind, last.account.context_id,
							last.locator) == key);
				else
				{
					validate_tombstone_identity(entry, last);
					if (!nonzero(last.retiring_operation))
						need(current_link(last).active ==
						     last.account.authority_id);
				}
			}
			*out = std::move(all);
		},
		error);
}

// Original complete CURRENT authority proof; caller resolves journals first.
unsigned int economic_flatfile_read_current_authority_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	std::span<const flatfile_economic_mapping_request> requests,
	flatfile_economic_authority_snapshot *out, std::string *error)
{
	return guarded(
		[&]
		{
			need(out && nonzero(lineage) && nonzero(epoch), EINVAL);
			need(requests.size() <= ECONOMIC_ACCOUNTING_MAX_ACCOUNTS, E2BIG);
			need(!root.empty() && lock.matches(root), EINVAL);
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

namespace
{
#if defined(_GLIBCXX_RELEASE) && _GLIBCXX_RELEASE == 13 && defined(_GLIBCXX_USE_CXX11_ABI) && \
	_GLIBCXX_USE_CXX11_ABI
bool authority_bounded_add(size_t &total, size_t amount) noexcept
{
	if (amount > SIZE_MAX - total)
		return false;
	total += amount;
	return true;
}
void authority_bounded_admit(size_t outer, size_t request,
			     flatfile_scratch_reserve_fn reserve_scratch_peak, void *context)
{
	need(authority_bounded_add(outer, request) && reserve_scratch_peak(outer, context),
	     ENOBUFS);
}
struct authority_bounded_file_workspace
{
	authority_bounded_file_workspace(size_t directory_size, const char *filename)
		: directory(directory_size, '\0')
		, name(filename)
	{
	}
	std::string directory, name;
	bytes encoded;
};
bytes authority_read_file_bounded(const std::string &root, const char *filename,
				  flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
				  size_t outer)
{
	size_t directory_size = root.size(), request = sizeof(authority_bounded_file_workspace);
	need(authority_bounded_add(directory_size, sizeof("/economic-evidence") - 1), ENOBUFS);
	if (directory_size > 15)
		need(directory_size != SIZE_MAX &&
			     authority_bounded_add(request, directory_size + 1),
		     ENOBUFS);
	const size_t name_size = std::strlen(filename);
	if (name_size > 15)
		need(authority_bounded_add(request, name_size + 1), ENOBUFS);
	authority_bounded_admit(outer, request, reserve_scratch_peak, context);
	authority_bounded_file_workspace work(directory_size, filename);
	std::copy(root.begin(), root.end(), work.directory.begin());
	std::copy_n("/economic-evidence", sizeof("/economic-evidence") - 1,
		    work.directory.begin() + root.size());
	errno = 0;
	const auto result = flatfile_read_bounded(work.directory, work.name,
						  FLATFILE_ECONOMIC_METADATA_MAX_BYTES,
						  &work.encoded, reserve_scratch_peak, context,
						  outer + request);
	need(result == flatfile_read_result::ok, result == flatfile_read_result::io_error ?
							 (errno == ENOMEM  ? ENOMEM :
							  errno == ENOBUFS ? ENOBUFS :
									     EIO) :
							 EILSEQ);
	return std::move(work.encoded);
}

// unwrap's reader/prefix/body/digest/hash result coexist. reader::fixed's
// array/take span and the returned reader are actual subsequent call frames.
constexpr size_t authority_unwrap_working =
	sizeof(reader) * 2 + sizeof(std::span<const uint8_t>) * 3 + sizeof(economic_digest) * 2;
constexpr size_t authority_epoch_decode_working =
	sizeof(bytes) + sizeof(epochs) + sizeof(reader) + sizeof(flatfile_economic_epoch) +
	sizeof(uint32_t) + authority_unwrap_working + sizeof(economic_account_key) +
	sizeof(std::span<const uint8_t>) * 2;

epochs authority_load_epochs_bounded(const std::string &root,
				     const flatfile_economic_control &control,
				     flatfile_scratch_reserve_fn reserve_scratch_peak,
				     void *context, size_t outer)
{
	authority_bounded_admit(outer, authority_epoch_decode_working, reserve_scratch_peak,
				context);
	const size_t frame_live = outer + authority_epoch_decode_working;
	auto encoded = authority_read_file_bounded(root, "epochs.eae", reserve_scratch_peak,
						   context, frame_live);
	authority_bounded_admit(frame_live, encoded.capacity(), reserve_scratch_peak, context);
	need(hash(encoded) == control.epochs_digest);
	uint32_t version = 0;
	auto in = unwrap(encoded, "DURECE1", &version);
	need(in.id().bytes == control.lineage.bytes);
	auto count = in.number(4);
	need(count == control.epoch_count && count <= FLATFILE_ECONOMIC_MAX_EPOCHS &&
	     in.number(4) == 0 && in.data.size() == 24 + count * (version == 1 ? 96 : 160));
	epochs values;
	size_t retained_live = frame_live;
	need(authority_bounded_add(retained_live, encoded.capacity()) &&
		     count <= SIZE_MAX / sizeof(flatfile_economic_epoch) &&
		     authority_bounded_add(retained_live, count * sizeof(flatfile_economic_epoch)),
	     ENOBUFS);
	need(reserve_scratch_peak(retained_live, context), ENOBUFS);
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
		if (version >= 2)
		{
			value.baseline_initialization =
				static_cast<flatfile_baseline_initialization>(in.number(1));
			if (version == 3)
			{
				value.initialization_origin =
					static_cast<flatfile_baseline_initialization_origin>(
						in.number(1));
				need(in.number(6) == 0);
			}
			else
				need(in.number(7) == 0);
			value.baseline_initializing_operation = in.id();
			auto opening = in.take(ECONOMIC_ACCOUNT_KEY_BYTES);
			if (value.baseline_initialization ==
			    flatfile_baseline_initialization::initialized)
				need(economic_account_key_decode(opening,
								 &value.baseline_opening) ==
				     economic_accounting_error::ok);
			else
				need(std::all_of(opening.begin(), opening.end(),
						 [](uint8_t byte) { return byte == 0; }));
		}
		values.push_back(value);
	}
	in.done();
	size_t lifecycle_count = 0;
	for (const auto &value : values)
		if (value.initialization_origin ==
		    flatfile_baseline_initialization_origin::lifecycle_owner)
			++lifecycle_count;
	const size_t node_bytes = sizeof(std::_Rb_tree_node<std::array<uint8_t, 16>>);
	size_t validator_request = sizeof(std::set<std::array<uint8_t, 16>>) * 2 +
				   sizeof(critical_operation_id) * 2 + sizeof(economic_account_key);
	need(values.size() <= SIZE_MAX / node_bytes && lifecycle_count <= SIZE_MAX / node_bytes &&
		     authority_bounded_add(validator_request, values.size() * node_bytes) &&
		     authority_bounded_add(validator_request, lifecycle_count * node_bytes),
	     ENOBUFS);
	authority_bounded_admit(retained_live, validator_request, reserve_scratch_peak, context);
	validate_epochs(control.lineage, values);
	need(values.empty() || values.back().epoch.bytes == control.last_epoch.bytes);
	return values;
}

flatfile_economic_control
authority_load_control_bounded(const std::string &root,
			       flatfile_scratch_reserve_fn reserve_scratch_peak, void *context,
			       size_t outer)
{
	const size_t frame = sizeof(flatfile_economic_control) + sizeof(bytes);
	authority_bounded_admit(outer, frame, reserve_scratch_peak, context);
	flatfile_economic_control value;
	{
		auto encoded = authority_read_file_bounded(
			root, "authority.eal", reserve_scratch_peak, context, outer + frame);
		size_t decode_request = encoded.capacity();
		need(authority_bounded_add(decode_request, sizeof(flatfile_economic_control)) &&
			     authority_bounded_add(decode_request, sizeof(reader)) &&
			     authority_bounded_add(decode_request, authority_unwrap_working),
		     ENOBUFS);
		authority_bounded_admit(outer + frame, decode_request, reserve_scratch_peak,
					context);
		value = decode_control(encoded);
	}
	// The control file/path/decoder die before the complete epoch catalog pass.
	(void)authority_load_epochs_bounded(root, value, reserve_scratch_peak, context,
					    outer + frame);
	return value;
}

unsigned int authority_recover_bounded(const std::string &root, const flatfile_authority_lock &lock,
				       flatfile_scratch_reserve_fn reserve_scratch_peak,
				       void *context, size_t outer) noexcept
{
	if (!lock.matches(root))
		return EINVAL;
	const auto result = flatfile_authority_transaction_recover_bounded(
		root, lock, reserve_scratch_peak, context, outer);
	return result == flatfile_authority_transaction_result::ok	 ? 0 :
	       result == flatfile_authority_transaction_result::io_error ? EIO :
									   EILSEQ;
}
#endif
}

unsigned int
flatfile_economic_control_read_bounded(const std::string &root, const flatfile_authority_lock &lock,
				       flatfile_economic_control *out,
				       flatfile_scratch_reserve_fn reserve_scratch_peak,
				       void *context, size_t outer) noexcept
{
	if (!out || !reserve_scratch_peak)
		return EINVAL;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)context;
	(void)outer;
	return ENOTSUP;
#else
	const auto recovered =
		authority_recover_bounded(root, lock, reserve_scratch_peak, context, outer);
	if (recovered)
		return recovered;
	try
	{
		authority_bounded_admit(outer, sizeof(flatfile_economic_control),
					reserve_scratch_peak, context);
		auto value =
			authority_load_control_bounded(root, reserve_scratch_peak, context,
						       outer + sizeof(flatfile_economic_control));
		*out = std::move(value);
		return 0;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int flatfile_economic_epoch_read_bounded(const std::string &root,
						  const flatfile_authority_lock &lock,
						  const critical_operation_id &lineage,
						  const critical_operation_id &epoch,
						  flatfile_economic_epoch *out,
						  flatfile_scratch_reserve_fn reserve_scratch_peak,
						  void *context, size_t outer) noexcept
{
	if (!out || !nonzero(lineage) || !nonzero(epoch) || !reserve_scratch_peak)
		return EINVAL;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)context;
	(void)outer;
	return ENOTSUP;
#else
	const auto recovered =
		authority_recover_bounded(root, lock, reserve_scratch_peak, context, outer);
	if (recovered)
		return recovered;
	try
	{
		const size_t frame = sizeof(flatfile_economic_control) + sizeof(epochs);
		authority_bounded_admit(outer, frame, reserve_scratch_peak, context);
		const auto control = authority_load_control_bounded(root, reserve_scratch_peak,
								    context, outer + frame);
		need(control.lineage.bytes == lineage.bytes, ESTALE);
		// Preserve the original second complete epoch read after lineage checking.
		const auto values = authority_load_epochs_bounded(
			root, control, reserve_scratch_peak, context, outer + frame);
		const auto at = std::find_if(values.begin(), values.end(), [&](const auto &value)
					     { return value.epoch.bytes == epoch.bytes; });
		need(at != values.end(), ENODATA);
		*out = *at;
		return 0;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

unsigned int economic_flatfile_lock_authority_bounded(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &lineage, const critical_operation_id &epoch,
	const std::span<const flatfile_economic_mapping_request> &requests,
	flatfile_economic_authority_snapshot *out, flatfile_scratch_reserve_fn reserve_scratch_peak,
	void *context, size_t outer, size_t *retained_payload) noexcept
{
	if (!out || !nonzero(lineage) || !nonzero(epoch) || !reserve_scratch_peak)
		return EINVAL;
	if (requests.size() > ECONOMIC_ACCOUNTING_MAX_ACCOUNTS)
		return E2BIG;
#if !defined(_GLIBCXX_RELEASE) || _GLIBCXX_RELEASE != 13 || !defined(_GLIBCXX_USE_CXX11_ABI) || \
	!_GLIBCXX_USE_CXX11_ABI
	(void)root;
	(void)lock;
	(void)context;
	(void)outer;
	(void)retained_payload;
	return ENOTSUP;
#else
	// INITIAL requests no mappings. Do not expose a partially profiled general
	// mapping reader: that distinct unsupported scope refuses before recovery.
	if (!requests.empty())
		return ENOTSUP;
	const auto recovered =
		authority_recover_bounded(root, lock, reserve_scratch_peak, context, outer);
	if (recovered)
		return recovered;
	try
	{
		using groups_type =
			std::map<size_t, std::vector<const flatfile_economic_mapping_request *>>;
		using native_groups_type = std::map<size_t, std::vector<size_t>>;
		const size_t frame = sizeof(flatfile_economic_control) +
				     sizeof(flatfile_economic_authority_snapshot) +
				     sizeof(std::set<uint64_t>) + sizeof(groups_type) +
				     sizeof(native_groups_type);
		authority_bounded_admit(outer, frame, reserve_scratch_peak, context);
		auto control = authority_load_control_bounded(root, reserve_scratch_peak, context,
							      outer + frame);
		need(control.lineage.bytes == lineage.bytes, ESTALE);
		need(nonzero(control.active_epoch), ENODATA);
		need(control.active_epoch.bytes == epoch.bytes, ESTALE);
		std::set<uint64_t> ids;
		groups_type groups;
		flatfile_economic_authority_snapshot candidate;
		candidate.lineage = lineage;
		candidate.epoch = epoch;
		candidate.lineage_revision = control.revision;
		candidate.mappings.reserve(requests.size());
		native_groups_type native_groups;
		std::sort(candidate.mappings.begin(), candidate.mappings.end(),
			  [](const auto &a, const auto &b)
			  { return a.account.authority_id < b.account.authority_id; });
		*out = std::move(candidate);
		if (retained_payload)
			*retained_payload = 0;
		return 0;
	}
	catch (const failure &value)
	{
		return value.code;
	}
	catch (const std::bad_alloc &)
	{
		return ENOMEM;
	}
	catch (...)
	{
		return EILSEQ;
	}
#endif
}

// Pure exact native-wallet namespace, sharing the original native-key framing.
bool flatfile_native_mobile_wallet_locator_valid(economic_account_kind kind, uint64_t context,
						 const flatfile_economic_locator &locator) noexcept
{
	return kind == economic_account_kind::wallet &&
	       context == ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT &&
	       locator.kind == FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR && locator.native_id &&
	       locator.native_id != UINT64_MAX && locator.name.empty();
}

bool flatfile_native_mobile_wallet_key_encode(
	economic_account_kind kind, uint64_t context, const flatfile_economic_locator &locator,
	std::array<uint8_t, FLATFILE_NATIVE_MOBILE_WALLET_KEY_BYTES> *output) noexcept
{
	if (!output || !flatfile_native_mobile_wallet_locator_valid(kind, context, locator))
		return false;
	std::array<uint8_t, FLATFILE_NATIVE_MOBILE_WALLET_KEY_BYTES> candidate{};
	size_t offset = 0;
	const auto put = [&](uint64_t value, size_t width)
	{
		for (size_t index = 0; index < width; ++index)
			candidate[offset++] = static_cast<uint8_t>(value >> (index * 8));
	};
	put(static_cast<uint16_t>(kind), 2);
	put(context, 8);
	put(locator.kind, 2);
	put(locator.native_id, 8);
	*output = candidate;
	return true;
}

bool flatfile_native_mobile_wallet_key_decode(std::span<const uint8_t> input,
					      flatfile_native_mobile_wallet_key *output) noexcept
{
	if (!output || input.size() != FLATFILE_NATIVE_MOBILE_WALLET_KEY_BYTES)
		return false;
	size_t offset = 0;
	const auto get = [&](size_t width)
	{
		uint64_t value = 0;
		for (size_t index = 0; index < width; ++index)
			value |= uint64_t(input[offset++]) << (index * 8);
		return value;
	};
	// Namespace values only: no mapping account/lineage/authority is fabricated.
	flatfile_native_mobile_wallet_key candidate;
	candidate.kind = static_cast<economic_account_kind>(get(2));
	candidate.context = get(8);
	candidate.locator_kind = static_cast<uint16_t>(get(2));
	candidate.native_id = get(8);
	if (candidate.kind != economic_account_kind::wallet ||
	    candidate.context != ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT ||
	    candidate.locator_kind != FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR ||
	    !candidate.native_id || candidate.native_id == UINT64_MAX)
		return false;
	*output = candidate;
	return true;
}

unsigned int flatfile_native_mobile_wallet_storage::observe_current_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_operation_id &active_epoch, const economic_account_key &wallet,
	const quest_mobile_native_reference &reference,
	flatfile_native_mobile_wallet_current *output) noexcept
{
	return guarded(
		[&]
		{
			need(output && !root.empty() && lock.matches(root) &&
				     !critical_operation_id_is_zero(active_epoch) &&
				     economic_account_key_valid(wallet) &&
				     wallet.kind == economic_account_kind::wallet &&
				     wallet.context_id == ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT &&
				     wallet.authority_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS &&
				     quest_mobile_native_reference_valid(reference),
			     EINVAL);
			std::array<uint8_t, QUEST_MOBILE_NATIVE_REFERENCE_BYTES> expected{},
				current{};
			need(quest_mobile_native_reference_encode(reference, &expected) ==
				     player_snapshot_codec_result::ok,
			     EINVAL);
			flatfile_economic_mapping_request request;
			request.account = wallet;
			request.locator.kind = FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR;
			request.locator.native_id = reference.mobile_instance_id;
			flatfile_economic_authority_snapshot authority;
			const auto authority_error =
				economic_flatfile_read_current_authority_locked(
					root, lock, wallet.lineage, active_epoch, { &request, 1 },
					&authority, nullptr);
			need(!authority_error, authority_error);
			need(authority.mappings.size() == 1 &&
				     economic_account_key_equal(authority.mappings.front().account,
								wallet) &&
				     flatfile_native_mobile_wallet_locator_valid(
					     authority.mappings.front().account.kind,
					     authority.mappings.front().account.context_id,
					     authority.mappings.front().locator) &&
				     authority.mappings.front().locator.native_id ==
					     reference.mobile_instance_id &&
				     authority.mappings.front().creating_operation.bytes ==
					     reference.birth_operation.bytes &&
				     critical_operation_id_is_zero(
					     authority.mappings.front().retiring_operation),
			     ESTALE);
			quest_mobile_native_flatfile_row row;
			const auto native_error = quest_mobile_native_flatfile_read_locked(
				root, lock, reference.mobile_instance_id, &row);
			need(!native_error, static_cast<unsigned int>(native_error));
			need(row.present &&
				     row.mobile_instance_id == reference.mobile_instance_id &&
				     row.image.state == quest_mobile_lifetime_state::live &&
				     row.image.cash && row.image.cash->revision,
			     ESTALE);
			need(quest_mobile_native_reference_encode(row.image.reference, &current) ==
					     player_snapshot_codec_result::ok &&
				     current == expected,
			     ESTALE);
			int64_t copper = 0;
			need(economic_coin_value(row.image.cash->denominations.amount, &copper) ==
				     economic_accounting_error::ok,
			     EILSEQ);
			flatfile_native_mobile_wallet_current candidate;
			candidate.active_epoch = authority.epoch;
			candidate.lineage_revision = authority.lineage_revision;
			candidate.mapping = std::move(authority.mappings.front());
			candidate.native = std::move(row.image);
			need(lock.matches(root), EINVAL);
			static_assert(std::is_nothrow_move_assignable_v<
				      flatfile_native_mobile_wallet_current>);
			*output = std::move(candidate);
		},
		nullptr);
}

// Private historical proof for the authentic ordinary NMB4 financial reader.
// Receives original compiled wallet/epoch/creator/native identity, not CURRENT
// DTO authority. Retired lifetimes remain historical; no active epoch required.
unsigned int flatfile_ordinary_native_birth_history_storage::verify_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const economic_account_key &wallet, const critical_operation_id &birth_epoch,
	const critical_operation_id &birth_operation, uint64_t native_id, std::string *error)
{
	return guarded(
		[&]
		{
			flatfile_economic_locator locator;
			locator.kind = FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR;
			locator.native_id = native_id;
			need(!root.empty() && lock.matches(root) &&
				     economic_account_key_valid(wallet) &&
				     wallet.authority_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS &&
				     nonzero(birth_epoch) && nonzero(birth_operation) &&
				     flatfile_native_mobile_wallet_locator_valid(
					     wallet.kind, wallet.context_id, locator),
			     EINVAL);
			// Complete original authenticated control/epochs loading. No recover()
			// or active/current selection gate is introduced into historical proof.
			const auto control = load_control(root);
			need(control.lineage.bytes == wallet.lineage.bytes, ESTALE);
			const size_t evidence_bucket = birth_operation.bytes[0];
			need(control.evidence_initialized[evidence_bucket / 8] &
				     (1U << (evidence_bucket % 8)),
			     ENODATA);
			const auto historical_epochs = load_epochs(root, control);
			need(std::any_of(historical_epochs.begin(), historical_epochs.end(),
					 [&](const auto &epoch)
					 { return epoch.epoch.bytes == birth_epoch.bytes; }),
			     ENODATA);

			const auto original = mapping_for_key(root, control, wallet);
			need(original.locator.kind == FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR &&
				     original.locator.native_id == native_id &&
				     original.locator.name.empty() &&
				     original.creating_operation.bytes == birth_operation.bytes,
			     EILSEQ);
			// Original SQL retained proof requires ONE locator/native lifetime and
			// ONE mapping created by this operation. Authenticate all real buckets,
			// including retired rows; absence is never an empty damaged bucket.
			size_t native_lifetimes = 0, created_lifetimes = 0;
			for (size_t bucket = 0; bucket < FLATFILE_ECONOMIC_METADATA_BUCKETS;
			     ++bucket)
			{
				const auto values = load_mappings(root, control, bucket);
				for (const auto &value : values)
				{
					if (value.locator.kind ==
						    FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR &&
					    value.locator.native_id == native_id)
					{
						++native_lifetimes;
						need(economic_account_key_equal(value.account,
										wallet),
						     EILSEQ);
					}
					if (value.creating_operation.bytes == birth_operation.bytes)
					{
						++created_lifetimes;
						need(economic_account_key_equal(value.account,
										wallet),
						     EILSEQ);
					}
				}
			}
			need(native_lifetimes == 1 && created_lifetimes == 1, EILSEQ);
			need(lock.matches(root), EINVAL);
		},
		error);
}

// CLOSED ordinary INITIAL mapping participant. Original generic mapping creation
// deliberately permits retired locator reuse; ordinary native birth never does.
unsigned int flatfile_native_mobile_birth_ordinary_initial_storage::prepare_locked(
	const std::string &root, const flatfile_authority_lock &lock,
	const critical_native_recovery_envelope &original, uint64_t expected_control_revision,
	flatfile_native_mobile_birth_ordinary_initial_stage *output, std::string *error) noexcept
{
	try
	{
		return guarded(
			[&]
			{
				need(output && !root.empty() && lock.matches(root), EINVAL);
				need(native_mobile_birth_cash_role_recovery_initial(original),
				     EINVAL);
				const auto checked = [](economic_accounting_error code)
				{
					need(code == economic_accounting_error::ok,
					     code == economic_accounting_error::capacity ? ENOMEM :
											   EINVAL);
				};
				quest_mobile_native_image image;
				std::vector<native_mobile_birth_item_recipe> recipes;
				native_mobile_birth_cash_role_recipe role;
				checked(native_mobile_birth_cash_role_command_decode(
					original.command, &image, &recipes, &role));
				need(role.role == native_mobile_birth_cash_role::ordinary_wallet,
				     ENOTSUP);
				economic_frozen_intent intent;
				checked(economic_intent_decode(original.command.accounting_intent,
							       &intent));
				checked(economic_intent_verify_binding(original.command, intent));
				const auto &metadata = intent.admission.metadata;
				need(metadata.source_event.has_value() && image.cash &&
					     image.cash->revision == 1,
				     EINVAL);

				// Caller recovered before entering this SAME root lock. None of the
				// original no-recovery internal readers below rewrite authority.
				auto control = load_control(root);
				need(control.lineage.bytes == metadata.lineage.bytes, ESTALE);
				need(nonzero(control.active_epoch), ENODATA);
				need(control.active_epoch.bytes == metadata.epoch.bytes &&
					     control.revision == expected_control_revision,
				     ESTALE);
				const size_t evidence_bucket =
					original.command.operation_id.bytes[0];
				need(control.evidence_initialized[evidence_bucket / 8] &
					     (1U << (evidence_bucket % 8)),
				     ENODATA);
				// load_control authenticated the complete retained epoch catalog,
				// its digest/count/last epoch; active_epoch is that actual last epoch.
				need(control.next_mapping_id <= FLATFILE_ECONOMIC_MAX_MAPPINGS,
				     ENOSPC);
				flatfile_economic_locator locator;
				locator.kind = FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR;
				locator.native_id = image.reference.mobile_instance_id;
				locator_valid(economic_account_kind::wallet,
					      ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT, locator, true);
				auto key = native_key(economic_account_kind::wallet,
						      ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT,
						      locator);

				// SQL original_absence excludes every old locator/native lifetime
				// and every mapping created by the root, including retired rows.
				// Authenticate ALL real buckets: missing required storage fails;
				// a truly empty bucket must have no unexpected retained file.
				for (size_t bucket = 0; bucket < FLATFILE_ECONOMIC_METADATA_BUCKETS;
				     ++bucket)
				{
					const auto values = load_mappings(root, control, bucket);
					for (const auto &value : values)
						need(!(value.locator.kind ==
							       FLATFILE_NATIVE_MOBILE_WALLET_LOCATOR &&
						       value.locator.native_id ==
							       locator.native_id) &&
							     value.creating_operation.bytes !=
								     original.command.operation_id
									     .bytes,
						     EEXIST);
				}
				const size_t native_bucket = hash(key)[0];
				auto index = load_native(root, control, native_bucket);
				// Even a structurally valid retired selected-key tombstone cannot
				// supply a second lifetime for this genuinely first native birth.
				need(find_native(index, key) == index.size(), EEXIST);
				need(index.size() < FLATFILE_ECONOMIC_BUCKET_MAPPINGS, ENOSPC);
				const uint64_t mapping_id = control.next_mapping_id;
				const size_t mapping_bucket =
					mapping_id % FLATFILE_ECONOMIC_METADATA_BUCKETS;
				auto values = load_mappings(root, control, mapping_bucket);
				need(values.size() < FLATFILE_ECONOMIC_BUCKET_MAPPINGS, ENOSPC);

				flatfile_native_mobile_birth_ordinary_initial_stage stage;
				stage.lineage_revision_before = control.revision;
				changing(control, expected_control_revision,
					 original.command.operation_id);
				stage.lineage_revision_after = control.revision;
				auto &mapping = stage.mapping;
				mapping.account = { control.lineage, economic_account_kind::wallet,
						    mapping_id,
						    ECONOMIC_NATIVE_MOBILE_WALLET_CONTEXT };
				mapping.locator = locator;
				mapping.creating_operation = mapping.last_operation =
					original.command.operation_id;
				// Mapping revision0, empty retirement and native index active=last
				// match original freshly created lifetime semantics. This ID comes
				// from real next_mapping_id, NEVER from native UID or a current DTO.
				values.push_back(mapping);
				++control.next_mapping_id;
				index.push_back({ std::move(key), mapping_id, mapping_id });
				std::sort(index.begin(), index.end(),
					  [](const auto &a, const auto &b)
					  { return a.key < b.key; });
				updates files;
				put_mapping(control, mapping_bucket, values, files);
				put_native(control, native_bucket, index, files);
				finish(control, std::move(files), &stage.operations);
				need(lock.matches(root), EINVAL);
				static_assert(std::is_nothrow_move_assignable_v<
					      flatfile_native_mobile_birth_ordinary_initial_stage>);
				*output = std::move(stage);
			},
			error);
	}
	catch (...)
	{
		return EFAULT;
	}
}
