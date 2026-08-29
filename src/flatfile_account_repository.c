#include "flatfile_account_repository.h"

#include "flatfile_store.h"

#include <array>
#include <cctype>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>

namespace
{
constexpr uint32_t account_format_version = 1;
constexpr size_t account_maximum_bytes = 1024 * 1024;
constexpr size_t account_maximum_string = 4096;
constexpr size_t account_maximum_ips = 1024;
constexpr size_t account_maximum_characters = 16;
constexpr std::array<uint8_t, 8> account_magic = { 'D', 'U', 'R', 'A', 'C', 'C', 'T', 0 };
std::mutex account_mutex;

struct authority_lock
{
	int fd = -1;
	~authority_lock() { flatfile_lock_release(fd); }
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
		if (value.size() > account_maximum_string || value.find('\0') != std::string::npos)
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
		if (!number(&length) || length > account_maximum_string || size - offset < length)
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

bool canonical_name(const std::string &input, std::string *canonical)
{
	if (!canonical || input.empty() || input.size() > 64)
		return false;
	canonical->clear();
	canonical->reserve(input.size());
	for (unsigned char character : input)
	{
		if (!std::isalnum(character) && character != '_' && character != '-')
			return false;
		canonical->push_back(static_cast<char>(std::tolower(character)));
	}
	return true;
}

std::string account_filename(const std::string &canonical)
{
	static const char digits[] = "0123456789abcdef";
	std::string encoded;
	encoded.reserve(canonical.size() * 2 + 5);
	for (unsigned char character : canonical)
	{
		encoded.push_back(digits[character >> 4]);
		encoded.push_back(digits[character & 0x0f]);
	}
	return encoded + ".acct";
}

std::string account_directory(const std::string &root)
{
	return root + "/identities/accounts";
}

bool encode_payload(const flatfile_account_record &record, std::vector<uint8_t> *payload)
{
	encoder out;
	out.string(record.name);
	out.string(record.email);
	out.string(record.password_hash);
	out.string(record.confirmation);
	out.number<int8_t>(record.blocked);
	out.number<int8_t>(record.confirmed);
	out.number<int8_t>(record.confirmation_sent);
	out.number<int64_t>(record.last_login);
	out.number<int64_t>(record.last_good);
	out.number<int64_t>(record.last_evil);
	for (uint64_t flag : record.flags)
		out.number<uint64_t>(flag);
	if (record.ips.size() > account_maximum_ips ||
	    record.characters.size() > account_maximum_characters)
		return false;
	out.number<uint32_t>(record.ips.size());
	for (const flatfile_account_ip &ip : record.ips)
	{
		out.string(ip.hostname);
		out.string(ip.address);
		out.number<uint64_t>(ip.count);
	}
	out.number<uint32_t>(record.characters.size());
	for (const flatfile_account_character &character : record.characters)
	{
		out.number<int32_t>(character.pid);
		out.string(character.name);
		out.number<uint64_t>(character.login_count);
		out.number<int64_t>(character.last_login);
		out.number<int8_t>(character.blocked);
		out.number<int8_t>(character.racewar);
		out.number<int32_t>(character.level);
		out.number<int32_t>(character.race);
		out.number<uint32_t>(character.primary_class);
		out.number<uint32_t>(character.secondary_class);
		out.number<int32_t>(character.last_room);
		out.number<int64_t>(character.last_save);
	}
	if (!out.valid || out.bytes.size() > account_maximum_bytes)
		return false;
	*payload = std::move(out.bytes);
	return true;
}

bool decode_payload(const uint8_t *data, size_t size, flatfile_account_record *record)
{
	decoder in{ data, size };
	flatfile_account_record decoded;
	if (!in.string(&decoded.name) || !in.string(&decoded.email) ||
	    !in.string(&decoded.password_hash) || !in.string(&decoded.confirmation) ||
	    !in.number(&decoded.blocked) || !in.number(&decoded.confirmed) ||
	    !in.number(&decoded.confirmation_sent) || !in.number(&decoded.last_login) ||
	    !in.number(&decoded.last_good) || !in.number(&decoded.last_evil))
		return false;
	for (uint64_t &flag : decoded.flags)
		if (!in.number(&flag))
			return false;
	uint32_t ip_count = 0;
	if (!in.number(&ip_count) || ip_count > account_maximum_ips)
		return false;
	decoded.ips.resize(ip_count);
	for (flatfile_account_ip &ip : decoded.ips)
		if (!in.string(&ip.hostname) || !in.string(&ip.address) || !in.number(&ip.count))
			return false;
	uint32_t character_count = 0;
	if (!in.number(&character_count) || character_count > account_maximum_characters)
		return false;
	decoded.characters.resize(character_count);
	for (flatfile_account_character &character : decoded.characters)
		if (!in.number(&character.pid) || !in.string(&character.name) ||
		    !in.number(&character.login_count) || !in.number(&character.last_login) ||
		    !in.number(&character.blocked) || !in.number(&character.racewar) ||
		    !in.number(&character.level) || !in.number(&character.race) ||
		    !in.number(&character.primary_class) ||
		    !in.number(&character.secondary_class) || !in.number(&character.last_room) ||
		    !in.number(&character.last_save))
			return false;
	if (!in.valid || in.offset != in.size)
		return false;
	*record = std::move(decoded);
	return true;
}

std::vector<uint8_t> encode_file(const flatfile_account_record &record, uint64_t revision,
				 bool *valid)
{
	std::vector<uint8_t> payload;
	if (!encode_payload(record, &payload))
	{
		*valid = false;
		return {};
	}
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.data(), payload.size(), digest);
	encoder out;
	out.bytes.insert(out.bytes.end(), account_magic.begin(), account_magic.end());
	out.number<uint32_t>(account_format_version);
	out.number<uint32_t>(payload.size());
	out.number<uint64_t>(revision);
	out.bytes.insert(out.bytes.end(), digest, digest + sizeof(digest));
	out.bytes.insert(out.bytes.end(), payload.begin(), payload.end());
	*valid = out.valid && out.bytes.size() <= account_maximum_bytes;
	return std::move(out.bytes);
}

flatfile_account_result load_unlocked(const std::string &root, const std::string &name,
				      flatfile_account_record *record, std::string *error)
{
	std::string canonical;
	if (!record || !canonical_name(name, &canonical))
		return flatfile_account_result::invalid;
	std::vector<uint8_t> bytes;
	const flatfile_read_result read_result =
		flatfile_read(account_directory(root), account_filename(canonical),
			      account_maximum_bytes, &bytes, error);
	if (read_result == flatfile_read_result::not_found)
		return flatfile_account_result::not_found;
	if (read_result == flatfile_read_result::invalid)
		return flatfile_account_result::invalid;
	if (read_result != flatfile_read_result::ok)
		return flatfile_account_result::io_error;
	constexpr size_t header_size = account_magic.size() + sizeof(uint32_t) * 2 +
				       sizeof(uint64_t) + SHA256_DIGEST_LENGTH;
	if (bytes.size() < header_size ||
	    memcmp(bytes.data(), account_magic.data(), account_magic.size()))
		return flatfile_account_result::invalid;
	decoder header{ bytes.data() + account_magic.size(), bytes.size() - account_magic.size() };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) || version != account_format_version || !revision ||
	    payload_size != bytes.size() - header_size)
		return flatfile_account_result::invalid;
	const uint8_t *stored_digest =
		bytes.data() + account_magic.size() + sizeof(uint32_t) * 2 + sizeof(uint64_t);
	const uint8_t *payload = bytes.data() + header_size;
	unsigned char actual_digest[SHA256_DIGEST_LENGTH];
	SHA256(payload, payload_size, actual_digest);
	if (CRYPTO_memcmp(stored_digest, actual_digest, sizeof(actual_digest)))
		return flatfile_account_result::invalid;
	flatfile_account_record decoded;
	if (!decode_payload(payload, payload_size, &decoded))
		return flatfile_account_result::invalid;
	std::string stored_canonical;
	if (!canonical_name(decoded.name, &stored_canonical) || stored_canonical != canonical)
		return flatfile_account_result::invalid;
	decoded.revision = revision;
	*record = std::move(decoded);
	return flatfile_account_result::ok;
}
} // namespace

flatfile_account_result flatfile_account_load(const std::string &root, const std::string &name,
					      flatfile_account_record *record, std::string *error)
{
	std::lock_guard<std::mutex> guard(account_mutex);
	return load_unlocked(root, name, record, error);
}

flatfile_account_result flatfile_account_save(const std::string &root,
					      const flatfile_account_record &record,
					      uint64_t expected_revision,
					      uint64_t *committed_revision, std::string *error)
{
	std::lock_guard<std::mutex> guard(account_mutex);
	authority_lock authority;
	if (!flatfile_lock_acquire(account_directory(root), ".accounts.lock", &authority.fd, error))
		return flatfile_account_result::io_error;
	std::string canonical;
	if (!committed_revision || !canonical_name(record.name, &canonical) ||
	    expected_revision == std::numeric_limits<uint64_t>::max())
		return flatfile_account_result::invalid;
	flatfile_account_record existing;
	const flatfile_account_result current = load_unlocked(root, record.name, &existing, error);
	if ((current == flatfile_account_result::not_found && expected_revision != 0) ||
	    (current == flatfile_account_result::ok && existing.revision != expected_revision))
		return flatfile_account_result::conflict;
	if (current != flatfile_account_result::ok && current != flatfile_account_result::not_found)
		return current;
	bool valid = false;
	const uint64_t revision = expected_revision + 1;
	std::vector<uint8_t> bytes = encode_file(record, revision, &valid);
	if (!valid)
		return flatfile_account_result::invalid;
	if (!flatfile_atomic_write(account_directory(root), account_filename(canonical), bytes,
				   error))
		return flatfile_account_result::io_error;
	*committed_revision = revision;
	return flatfile_account_result::ok;
}

struct flatfile_account_lock::state
{
	std::unique_lock<std::mutex> process_lock;
	int fd = -1;
	std::string root;

	state()
		: process_lock(account_mutex, std::defer_lock)
	{
	}
	~state() { flatfile_lock_release(fd); }
};

flatfile_account_lock::flatfile_account_lock() noexcept
	: state_(new(std::nothrow) state)
{
}
flatfile_account_lock::~flatfile_account_lock() = default;

bool flatfile_account_lock::acquire(const std::string &root, std::string *error)
{
	if (!state_ || state_->process_lock.owns_lock() || root.empty())
		return false;
	state_->process_lock.lock();
	if (flatfile_lock_acquire(account_directory(root), ".accounts.lock", &state_->fd, error))
	{
		state_->root = root;
		return true;
	}
	state_->process_lock.unlock();
	return false;
}

bool flatfile_account_lock::matches(const std::string &root) const
{
	return state_ && state_->process_lock.owns_lock() && state_->fd >= 0 &&
	       state_->root == root;
}

flatfile_account_result
flatfile_account_prepare_save(const std::string &root, const flatfile_account_lock &account_lock,
			      const flatfile_authority_lock &authority_lock,
			      const flatfile_account_record &record, uint64_t expected_revision,
			      flatfile_authority_operation *operation, uint64_t *committed_revision,
			      std::string *error)
{
	std::string canonical;
	if (!operation || !committed_revision || !account_lock.matches(root) ||
	    !authority_lock.matches(root) || !canonical_name(record.name, &canonical) ||
	    expected_revision == std::numeric_limits<uint64_t>::max())
		return flatfile_account_result::invalid;
	*operation = {};
	flatfile_account_record existing;
	const flatfile_account_result current = load_unlocked(root, record.name, &existing, error);
	if ((current == flatfile_account_result::not_found && expected_revision != 0) ||
	    (current == flatfile_account_result::ok && existing.revision != expected_revision))
		return flatfile_account_result::conflict;
	if (current != flatfile_account_result::ok && current != flatfile_account_result::not_found)
		return current;
	bool valid = false;
	const uint64_t revision = expected_revision + 1;
	std::vector<uint8_t> bytes = encode_file(record, revision, &valid);
	if (!valid)
		return flatfile_account_result::invalid;
	operation->store = flatfile_authority_store::accounts;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = account_filename(canonical);
	operation->bytes = std::move(bytes);
	*committed_revision = revision;
	return flatfile_account_result::ok;
}

flatfile_account_result flatfile_account_exists(const std::string &root, const std::string &name,
						bool *exists, std::string *error)
{
	if (!exists)
		return flatfile_account_result::invalid;
	flatfile_account_record record;
	const flatfile_account_result result = flatfile_account_load(root, name, &record, error);
	if (result == flatfile_account_result::ok)
	{
		*exists = true;
		return result;
	}
	if (result == flatfile_account_result::not_found)
	{
		*exists = false;
		return flatfile_account_result::ok;
	}
	return result;
}
