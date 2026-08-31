#include "flatfile/flatfile_identity_repository.h"

#include "flatfile/flatfile_store.h"

#include <algorithm>
#include <array>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>
#include <utility>
#include <vector>

namespace
{
constexpr uint32_t identity_format_version = 2;
constexpr size_t identity_maximum_bytes = 64 * 1024 * 1024;
constexpr size_t identity_maximum_entries = 1000000;
constexpr size_t identity_maximum_name = 64;
constexpr std::array<uint8_t, 8> identity_magic = { 'D', 'U', 'R', 'I', 'D', 'E', 'N', 0 };
constexpr const char *identity_filename = "catalog.identity";
constexpr const char *identity_lock_filename = ".identity.lock";
std::mutex identity_mutex;

struct identity_catalog
{
	uint64_t revision = 0;
	int64_t next_pid = 1;
	std::vector<flatfile_identity_record> entries;
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = static_cast<unsigned_type>(value);
		for (size_t index = 0; index < sizeof(T); ++index)
		{
			bytes.push_back(static_cast<uint8_t>(bits & 0xff));
			bits >>= 8;
		}
	}

	void string(const std::string &value)
	{
		if (value.size() > identity_maximum_name || value.find('\0') != std::string::npos)
		{
			valid = false;
			return;
		}
		number<uint32_t>(value.size());
		bytes.insert(bytes.end(), value.begin(), value.end());
	}
};

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;
	bool valid = true;

	template <typename T> bool number(T *value)
	{
		if (!valid || !value || size - offset < sizeof(T))
		{
			valid = false;
			return false;
		}
		using unsigned_type = std::make_unsigned_t<T>;
		unsigned_type bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<unsigned_type>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}

	bool string(std::string *value)
	{
		uint32_t length = 0;
		if (!number(&length) || !length || length > identity_maximum_name ||
		    size - offset < length)
		{
			valid = false;
			return false;
		}
		value->assign(reinterpret_cast<const char *>(data + offset), length);
		offset += length;
		if (value->find('\0') != std::string::npos)
		{
			valid = false;
			return false;
		}
		return true;
	}
};

std::string identity_directory(const std::string &root)
{
	return root + "/identities/names";
}

bool canonical_name(const std::string &input, std::string *canonical)
{
	if (!canonical || input.empty() || input.size() > identity_maximum_name)
		return false;
	canonical->clear();
	canonical->reserve(input.size());
	for (unsigned char character : input)
	{
		if (character >= 'A' && character <= 'Z')
			canonical->push_back(static_cast<char>(character - 'A' + 'a'));
		else if ((character >= 'a' && character <= 'z') ||
			 (character >= '0' && character <= '9') || character == '_' ||
			 character == '-')
			canonical->push_back(static_cast<char>(character));
		else
			return false;
	}
	return true;
}

bool validate_catalog(const identity_catalog &catalog)
{
	if (catalog.next_pid < 1 ||
	    catalog.next_pid > static_cast<int64_t>(std::numeric_limits<int32_t>::max()) + 1 ||
	    catalog.entries.size() > identity_maximum_entries)
		return false;
	std::unordered_set<int32_t> pids;
	std::unordered_set<std::string> active_names;
	for (const flatfile_identity_record &entry : catalog.entries)
	{
		std::string name_key, account_key;
		if (entry.pid <= 0 || entry.pid >= catalog.next_pid ||
		    !canonical_name(entry.name, &name_key) ||
		    !canonical_name(entry.account, &account_key) || !pids.insert(entry.pid).second)
			return false;
		if (entry.active && !active_names.insert(name_key).second)
			return false;
	}
	return true;
}

bool encode_payload(const identity_catalog &catalog, std::vector<uint8_t> *payload)
{
	if (!payload || !validate_catalog(catalog))
		return false;
	std::vector<flatfile_identity_record> entries = catalog.entries;
	std::sort(entries.begin(), entries.end(),
		  [](const auto &left, const auto &right) { return left.pid < right.pid; });
	encoder out;
	out.number<int64_t>(catalog.next_pid);
	out.number<uint32_t>(entries.size());
	for (const flatfile_identity_record &entry : entries)
	{
		out.number<int32_t>(entry.pid);
		out.number<uint8_t>(entry.active ? 1 : 0);
		out.number<uint8_t>(entry.blocked ? 1 : 0);
		out.string(entry.name);
		out.string(entry.account);
		out.number<uint64_t>(entry.login_count);
		out.number<int64_t>(entry.last_login);
		out.number<int8_t>(entry.racewar);
		out.number<int32_t>(entry.level);
		out.number<int32_t>(entry.race);
		out.number<uint32_t>(entry.primary_class);
		out.number<uint32_t>(entry.secondary_class);
		out.number<int32_t>(entry.last_room);
		out.number<int64_t>(entry.last_save);
	}
	if (!out.valid || out.bytes.size() > identity_maximum_bytes)
		return false;
	*payload = std::move(out.bytes);
	return true;
}

bool decode_payload(const uint8_t *data, size_t size, uint32_t version, identity_catalog *catalog)
{
	decoder in{ data, size };
	identity_catalog decoded;
	uint32_t count = 0;
	if (!in.number(&decoded.next_pid) || !in.number(&count) || count > identity_maximum_entries)
		return false;
	decoded.entries.resize(count);
	for (flatfile_identity_record &entry : decoded.entries)
	{
		uint8_t active = 0, blocked = 0;
		if (!in.number(&entry.pid) || !in.number(&active) || !in.number(&blocked) ||
		    active > 1 || blocked > 1 || !in.string(&entry.name) ||
		    !in.string(&entry.account))
			return false;
		entry.active = active;
		entry.blocked = blocked;
		if (version >= 2 &&
		    (!in.number(&entry.login_count) || !in.number(&entry.last_login) ||
		     !in.number(&entry.racewar) || !in.number(&entry.level) ||
		     !in.number(&entry.race) || !in.number(&entry.primary_class) ||
		     !in.number(&entry.secondary_class) || !in.number(&entry.last_room) ||
		     !in.number(&entry.last_save)))
			return false;
	}
	if (!in.valid || in.offset != in.size || !validate_catalog(decoded))
		return false;
	*catalog = std::move(decoded);
	return true;
}

bool encode_file(const identity_catalog &catalog, uint64_t revision, std::vector<uint8_t> *bytes)
{
	std::vector<uint8_t> payload;
	if (!bytes || !revision || !encode_payload(catalog, &payload))
		return false;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.data(), payload.size(), digest);
	encoder out;
	out.bytes.insert(out.bytes.end(), identity_magic.begin(), identity_magic.end());
	out.number<uint32_t>(identity_format_version);
	out.number<uint32_t>(payload.size());
	out.number<uint64_t>(revision);
	out.bytes.insert(out.bytes.end(), digest, digest + sizeof(digest));
	out.bytes.insert(out.bytes.end(), payload.begin(), payload.end());
	if (!out.valid || out.bytes.size() > identity_maximum_bytes)
		return false;
	*bytes = std::move(out.bytes);
	return true;
}

flatfile_identity_result load_catalog(const std::string &root, identity_catalog *catalog,
				      std::string *error)
{
	if (!catalog)
		return flatfile_identity_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result result = flatfile_read(
		identity_directory(root), identity_filename, identity_maximum_bytes, &bytes, error);
	if (result == flatfile_read_result::not_found)
		return flatfile_identity_result::not_found;
	if (result == flatfile_read_result::invalid)
		return flatfile_identity_result::invalid;
	if (result != flatfile_read_result::ok)
		return flatfile_identity_result::io_error;
	constexpr size_t header_size = identity_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(uint64_t) + SHA256_DIGEST_LENGTH;
	if (bytes.size() < header_size ||
	    memcmp(bytes.data(), identity_magic.data(), identity_magic.size()))
		return flatfile_identity_result::invalid;
	decoder header{ bytes.data() + identity_magic.size(),
			bytes.size() - identity_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || (version != 1 && version != identity_format_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return flatfile_identity_result::invalid;
	const uint8_t *stored_digest =
		bytes.data() + identity_magic.size() + sizeof(uint32_t) * 2 + sizeof(uint64_t);
	const uint8_t *payload = bytes.data() + header_size;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_identity_result::invalid;
	identity_catalog decoded;
	if (!decode_payload(payload, payload_size, version, &decoded))
		return flatfile_identity_result::invalid;
	decoded.revision = revision;
	for (flatfile_identity_record &entry : decoded.entries)
		entry.catalog_revision = revision;
	*catalog = std::move(decoded);
	return flatfile_identity_result::ok;
}

flatfile_identity_result publish_catalog(const std::string &root, identity_catalog *catalog,
					 std::string *error)
{
	if (!catalog || catalog->revision == std::numeric_limits<uint64_t>::max())
		return flatfile_identity_result::exhausted;
	std::vector<uint8_t> bytes;
	const uint64_t revision = catalog->revision + 1;
	if (!encode_file(*catalog, revision, &bytes))
		return flatfile_identity_result::invalid;
	if (!flatfile_atomic_write(identity_directory(root), identity_filename, bytes, error))
		return flatfile_identity_result::io_error;
	catalog->revision = revision;
	return flatfile_identity_result::ok;
}

template <typename Mutation> flatfile_identity_result
mutate_catalog(const std::string &root, Mutation mutation, std::string *error)
{
	flatfile_identity_lock authority;
	if (!authority.acquire(root, error))
		return flatfile_identity_result::io_error;
	identity_catalog catalog;
	const flatfile_identity_result loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_identity_result::ok && loaded != flatfile_identity_result::not_found)
		return loaded;
	const flatfile_identity_result changed = mutation(&catalog);
	if (changed != flatfile_identity_result::ok)
		return changed;
	return publish_catalog(root, &catalog, error);
}

flatfile_identity_record *find_pid(identity_catalog *catalog, int32_t pid)
{
	auto found = std::find_if(catalog->entries.begin(), catalog->entries.end(),
				  [pid](const auto &entry) { return entry.pid == pid; });
	return found == catalog->entries.end() ? nullptr : &*found;
}

flatfile_identity_record *find_active_name(identity_catalog *catalog, const std::string &name)
{
	std::string sought;
	if (!canonical_name(name, &sought))
		return nullptr;
	for (flatfile_identity_record &entry : catalog->entries)
	{
		std::string candidate;
		if (entry.active && canonical_name(entry.name, &candidate) && candidate == sought)
			return &entry;
	}
	return nullptr;
}

flatfile_identity_result apply_account_sync(identity_catalog *catalog,
					    const std::string &account_key,
					    const std::vector<flatfile_identity_record> &records)
{
	std::unordered_set<int32_t> desired_pids;
	std::unordered_set<std::string> desired_names;
	for (const flatfile_identity_record &desired : records)
	{
		std::string desired_account, desired_name;
		if (desired.pid <= 0 || desired.pid >= catalog->next_pid ||
		    !canonical_name(desired.account, &desired_account) ||
		    desired_account != account_key ||
		    !canonical_name(desired.name, &desired_name) ||
		    !desired_pids.insert(desired.pid).second ||
		    !desired_names.insert(desired_name).second)
			return flatfile_identity_result::invalid;
		flatfile_identity_record *existing = find_pid(catalog, desired.pid);
		if (existing)
		{
			std::string existing_account;
			if (!existing->active ||
			    !canonical_name(existing->account, &existing_account) ||
			    existing_account != account_key)
				return flatfile_identity_result::conflict;
		}
	}

	identity_catalog candidate = *catalog;
	for (flatfile_identity_record &entry : candidate.entries)
	{
		std::string existing_account;
		if (entry.active && canonical_name(entry.account, &existing_account) &&
		    existing_account == account_key && !desired_pids.contains(entry.pid))
		{
			entry.active = false;
			entry.blocked = true;
		}
	}
	for (flatfile_identity_record desired : records)
	{
		desired.catalog_revision = 0;
		desired.active = true;
		flatfile_identity_record *existing = find_pid(&candidate, desired.pid);
		if (existing)
			*existing = std::move(desired);
		else
			candidate.entries.push_back(std::move(desired));
	}
	if (!validate_catalog(candidate))
		return flatfile_identity_result::conflict;
	*catalog = std::move(candidate);
	return flatfile_identity_result::ok;
}
} // namespace

struct flatfile_identity_lock::state
{
	std::unique_lock<std::mutex> process_lock;
	int fd = -1;
	std::string root;

	state()
		: process_lock(identity_mutex, std::defer_lock)
	{
	}
	~state() { flatfile_lock_release(fd); }
};

flatfile_identity_lock::flatfile_identity_lock() noexcept
	: state_(new(std::nothrow) state)
{
}
flatfile_identity_lock::~flatfile_identity_lock() = default;

bool flatfile_identity_lock::acquire(const std::string &root, std::string *error)
{
	if (!state_ || state_->process_lock.owns_lock() || root.empty())
		return false;
	state_->process_lock.lock();
	if (flatfile_lock_acquire(identity_directory(root), identity_lock_filename, &state_->fd,
				  error))
	{
		state_->root = root;
		return true;
	}
	state_->process_lock.unlock();
	return false;
}

bool flatfile_identity_lock::owns(const std::string &root) const
{
	return state_ && state_->process_lock.owns_lock() && state_->fd >= 0 &&
	       state_->root == root;
}

bool flatfile_identity_lock::matches(const std::string &root) const
{
	return owns(root);
}

flatfile_identity_result flatfile_identity_allocate_pid(const std::string &root, int32_t *pid,
							std::string *error)
{
	if (!pid)
		return flatfile_identity_result::invalid;
	int32_t allocated = 0;
	const flatfile_identity_result result = mutate_catalog(
		root,
		[&allocated](identity_catalog *catalog)
		{
			if (catalog->next_pid > std::numeric_limits<int32_t>::max())
				return flatfile_identity_result::exhausted;
			allocated = static_cast<int32_t>(catalog->next_pid++);
			return flatfile_identity_result::ok;
		},
		error);
	if (result == flatfile_identity_result::ok)
		*pid = allocated;
	return result;
}

flatfile_identity_result flatfile_identity_current_highest_pid(const std::string &root,
							       int32_t *pid, std::string *error)
{
	if (!pid)
		return flatfile_identity_result::invalid;
	std::lock_guard<std::mutex> guard(identity_mutex);
	identity_catalog catalog;
	const flatfile_identity_result result = load_catalog(root, &catalog, error);
	if (result == flatfile_identity_result::not_found)
	{
		*pid = 0;
		return flatfile_identity_result::ok;
	}
	if (result != flatfile_identity_result::ok)
		return result;
	*pid = static_cast<int32_t>(catalog.next_pid - 1);
	return flatfile_identity_result::ok;
}

flatfile_identity_result flatfile_identity_claim(const std::string &root, int32_t pid,
						 const std::string &name,
						 const std::string &account, std::string *error)
{
	std::string name_key, account_key;
	if (pid <= 0 || !canonical_name(name, &name_key) || !canonical_name(account, &account_key))
		return flatfile_identity_result::invalid;
	return mutate_catalog(
		root,
		[&](identity_catalog *catalog)
		{
			if (pid >= catalog->next_pid)
				return flatfile_identity_result::invalid;
			if (find_pid(catalog, pid) || find_active_name(catalog, name))
				return flatfile_identity_result::conflict;
			flatfile_identity_record entry;
			entry.pid = pid;
			entry.name = name;
			entry.account = account;
			entry.active = true;
			catalog->entries.push_back(std::move(entry));
			return flatfile_identity_result::ok;
		},
		error);
}

flatfile_identity_result flatfile_identity_lookup_name(const std::string &root,
						       const std::string &name,
						       flatfile_identity_record *record,
						       std::string *error)
{
	std::string canonical;
	if (!record || !canonical_name(name, &canonical))
		return flatfile_identity_result::invalid;
	std::lock_guard<std::mutex> guard(identity_mutex);
	identity_catalog catalog;
	const flatfile_identity_result result = load_catalog(root, &catalog, error);
	if (result != flatfile_identity_result::ok)
		return result;
	flatfile_identity_record *entry = find_active_name(&catalog, canonical);
	if (!entry)
		return flatfile_identity_result::not_found;
	*record = *entry;
	return flatfile_identity_result::ok;
}

flatfile_identity_result flatfile_identity_lookup_pid(const std::string &root, int32_t pid,
						      flatfile_identity_record *record,
						      std::string *error)
{
	if (!record || pid <= 0)
		return flatfile_identity_result::invalid;
	std::lock_guard<std::mutex> guard(identity_mutex);
	identity_catalog catalog;
	const flatfile_identity_result result = load_catalog(root, &catalog, error);
	if (result != flatfile_identity_result::ok)
		return result;
	flatfile_identity_record *entry = find_pid(&catalog, pid);
	if (!entry)
		return flatfile_identity_result::not_found;
	*record = *entry;
	return flatfile_identity_result::ok;
}

flatfile_identity_result
flatfile_identity_list_account(const std::string &root, const std::string &account,
			       std::vector<flatfile_identity_record> *records, std::string *error)
{
	std::string account_key;
	if (!records || !canonical_name(account, &account_key))
		return flatfile_identity_result::invalid;
	std::lock_guard<std::mutex> guard(identity_mutex);
	identity_catalog catalog;
	const flatfile_identity_result result = load_catalog(root, &catalog, error);
	records->clear();
	if (result == flatfile_identity_result::not_found)
		return flatfile_identity_result::ok;
	if (result != flatfile_identity_result::ok)
		return result;
	for (const flatfile_identity_record &entry : catalog.entries)
	{
		std::string candidate;
		if (entry.active && canonical_name(entry.account, &candidate) &&
		    candidate == account_key)
			records->push_back(entry);
	}
	std::sort(records->begin(), records->end(),
		  [](const auto &left, const auto &right) { return left.pid < right.pid; });
	return flatfile_identity_result::ok;
}

flatfile_identity_result
flatfile_identity_sync_account(const std::string &root, const std::string &account,
			       const std::vector<flatfile_identity_record> &records,
			       std::string *error)
{
	std::string account_key;
	if (!canonical_name(account, &account_key) || records.size() > 1024)
		return flatfile_identity_result::invalid;
	return mutate_catalog(
		root, [&](identity_catalog *catalog)
		{ return apply_account_sync(catalog, account_key, records); }, error);
}

flatfile_identity_result flatfile_identity_prepare_sync_account(
	const std::string &root, const flatfile_identity_lock &identity_lock,
	const flatfile_authority_lock &authority_lock, const std::string &account,
	const std::vector<flatfile_identity_record> &records,
	flatfile_authority_operation *operation, std::string *error)
{
	std::string account_key;
	if (!operation || !identity_lock.matches(root) || !authority_lock.matches(root) ||
	    !canonical_name(account, &account_key) || records.size() > 1024)
		return flatfile_identity_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, authority_lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_identity_result::io_error :
			       flatfile_identity_result::invalid;
	identity_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_identity_result::ok && loaded != flatfile_identity_result::not_found)
		return loaded;
	const auto changed = apply_account_sync(&catalog, account_key, records);
	if (changed != flatfile_identity_result::ok)
		return changed;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_identity_result::exhausted;
	std::vector<uint8_t> bytes;
	if (!encode_file(catalog, catalog.revision + 1, &bytes))
		return flatfile_identity_result::invalid;
	operation->store = flatfile_authority_store::identities;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = identity_filename;
	operation->bytes = std::move(bytes);
	return flatfile_identity_result::ok;
}

flatfile_identity_result flatfile_identity_rename(const std::string &root, int32_t pid,
						  const std::string &expected_name,
						  const std::string &new_name, std::string *error)
{
	std::string expected_key, new_key;
	if (pid <= 0 || !canonical_name(expected_name, &expected_key) ||
	    !canonical_name(new_name, &new_key))
		return flatfile_identity_result::invalid;
	return mutate_catalog(
		root,
		[&](identity_catalog *catalog)
		{
			flatfile_identity_record *entry = find_pid(catalog, pid);
			std::string current_key;
			if (!entry || !entry->active)
				return flatfile_identity_result::not_found;
			if (!canonical_name(entry->name, &current_key) ||
			    current_key != expected_key)
				return flatfile_identity_result::conflict;
			flatfile_identity_record *collision = find_active_name(catalog, new_name);
			if (collision && collision->pid != pid)
				return flatfile_identity_result::conflict;
			entry->name = new_name;
			return flatfile_identity_result::ok;
		},
		error);
}

flatfile_identity_result flatfile_identity_set_blocked(const std::string &root, int32_t pid,
						       bool blocked, std::string *error)
{
	if (pid <= 0)
		return flatfile_identity_result::invalid;
	return mutate_catalog(
		root,
		[&](identity_catalog *catalog)
		{
			flatfile_identity_record *entry = find_pid(catalog, pid);
			if (!entry || !entry->active)
				return flatfile_identity_result::not_found;
			entry->blocked = blocked;
			return flatfile_identity_result::ok;
		},
		error);
}

flatfile_identity_result flatfile_identity_remove(const std::string &root, int32_t pid,
						  const std::string &expected_name,
						  std::string *error)
{
	std::string expected_key;
	if (pid <= 0 || !canonical_name(expected_name, &expected_key))
		return flatfile_identity_result::invalid;
	return mutate_catalog(
		root,
		[&](identity_catalog *catalog)
		{
			flatfile_identity_record *entry = find_pid(catalog, pid);
			std::string current_key;
			if (!entry || !entry->active)
				return flatfile_identity_result::not_found;
			if (!canonical_name(entry->name, &current_key) ||
			    current_key != expected_key)
				return flatfile_identity_result::conflict;
			entry->active = false;
			entry->blocked = true;
			return flatfile_identity_result::ok;
		},
		error);
}

flatfile_identity_result
flatfile_identity_prepare_remove(const std::string &root,
				 const flatfile_identity_lock &identity_lock,
				 const flatfile_authority_lock &authority_lock, int32_t pid,
				 const std::string &expected_name,
				 flatfile_authority_operation *operation, std::string *error)
{
	std::string expected_key;
	if (!operation || pid <= 0 || !identity_lock.matches(root) ||
	    !authority_lock.matches(root) || !canonical_name(expected_name, &expected_key))
		return flatfile_identity_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, authority_lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_identity_result::io_error :
			       flatfile_identity_result::invalid;
	identity_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded != flatfile_identity_result::ok)
		return loaded;
	flatfile_identity_record *entry = find_pid(&catalog, pid);
	std::string current_key;
	if (!entry || !canonical_name(entry->name, &current_key) || current_key != expected_key)
		return entry ? flatfile_identity_result::conflict :
			       flatfile_identity_result::not_found;
	if (!entry->active)
		return entry->blocked ? flatfile_identity_result::unchanged :
					flatfile_identity_result::conflict;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_identity_result::exhausted;
	entry->active = false;
	entry->blocked = true;
	std::vector<uint8_t> bytes;
	if (!encode_file(catalog, catalog.revision + 1, &bytes))
		return flatfile_identity_result::invalid;
	operation->store = flatfile_authority_store::identities;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = identity_filename;
	operation->bytes = std::move(bytes);
	return flatfile_identity_result::ok;
}
