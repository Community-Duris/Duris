#include "flatfile_player_domain_repository.h"

#include "flatfile_store.h"
#include "epic_command.h"

#include <algorithm>
#include <array>
#include <cerrno>
#include <cstring>
#include <limits>
#include <mutex>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <vector>

namespace
{
constexpr uint32_t domain_format_version = 2;
constexpr std::array<uint8_t, 8> player_magic = { 'D', 'U', 'R', 'P', 'D', 'O', 'M', 0 };
constexpr std::array<uint8_t, 8> bank_magic = { 'D', 'U', 'R', 'B', 'A', 'N', 'K', 0 };
constexpr size_t domain_maximum_bytes = 64 * 1024;
constexpr size_t domain_maximum_operations = 512;
constexpr size_t account_maximum_bytes = PLAYER_LOAD_ACCOUNT_MAX;
constexpr const char *domain_lock_filename = ".player-domains.lock";
std::mutex domain_mutex;

struct bank_record
{
	std::string account_name;
	int8_t racewar = 0;
	uint64_t revision = 0;
	std::array<uint64_t, 4> balances = {};
};

struct domain_operation
{
	critical_operation_id operation_id;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest;
	unsigned int result_code = 0;
	uint16_t result_size = 0;
	std::array<uint8_t, CRITICAL_COMPLETION_RESULT_MAX_BYTES> result = {};
};

struct player_authority
{
	flatfile_player_domain_record record;
	std::vector<domain_operation> operations;
};

enum class player_publish_result
{
	ok,
	invalid,
	io_error
};

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
		try
		{
			for (size_t index = 0; index < sizeof(T); ++index)
			{
				bytes.push_back(static_cast<uint8_t>(bits & 0xff));
				bits >>= 8;
			}
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	void string(const std::string &value)
	{
		number<uint32_t>(value.size());
		try
		{
			bytes.insert(bytes.end(), value.begin(), value.end());
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}

	void raw(const uint8_t *data, size_t size)
	{
		if (!valid || (!data && size))
		{
			valid = false;
			return;
		}
		try
		{
			bytes.insert(bytes.end(), data, data + size);
		}
		catch (const std::bad_alloc &)
		{
			valid = false;
		}
	}
};

struct decoder
{
	const uint8_t *data;
	size_t size;
	size_t offset = 0;

	template <typename T> bool number(T *value)
	{
		if (!value || size - offset < sizeof(T))
			return false;
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
		if (!value || !number(&length) || !length || length > account_maximum_bytes ||
		    size - offset < length)
			return false;
		value->assign(reinterpret_cast<const char *>(data + offset), length);
		offset += length;
		return value->find('\0') == std::string::npos;
	}

	bool raw(uint8_t *value, size_t count)
	{
		if (!value || size - offset < count)
			return false;
		memcpy(value, data + offset, count);
		offset += count;
		return true;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool canonical_account(const std::string &input, std::string *canonical)
{
	if (!canonical || input.empty() || input.size() > account_maximum_bytes)
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

std::string player_filename(int32_t pid)
{
	return "player-" + std::to_string(pid) + ".domain";
}

std::string bank_filename(const std::string &account, int8_t racewar)
{
	return "bank-" + account + "-" + std::to_string(static_cast<int>(racewar)) + ".domain";
}

bool valid_gameplay(const flatfile_player_domain_record &record)
{
	if (record.recent_pvp_deaths.size() > PLAYER_LOAD_RECENT_PVP_MAX ||
	    record.completed_epic_zones.size() > PLAYER_LOAD_COMPLETED_ZONE_MAX)
		return false;
	for (size_t index = 0; index < record.recent_pvp_deaths.size(); ++index)
		if (record.recent_pvp_deaths[index] <= 0 ||
		    (index &&
		     record.recent_pvp_deaths[index - 1] < record.recent_pvp_deaths[index]))
			return false;
	for (size_t index = 0; index < record.completed_epic_zones.size(); ++index)
		if (record.completed_epic_zones[index] <= 0 ||
		    (index &&
		     record.completed_epic_zones[index - 1] >= record.completed_epic_zones[index]))
			return false;
	return true;
}

bool encode_file(const std::array<uint8_t, 8> &magic, const std::vector<uint8_t> &payload,
		 uint64_t revision, std::vector<uint8_t> *bytes)
{
	if (!bytes || !revision || payload.size() > domain_maximum_bytes)
		return false;
	unsigned char digest[SHA256_DIGEST_LENGTH];
	SHA256(payload.data(), payload.size(), digest);
	encoder file;
	try
	{
		file.bytes.insert(file.bytes.end(), magic.begin(), magic.end());
		file.number<uint32_t>(domain_format_version);
		file.number<uint32_t>(payload.size());
		file.number(revision);
		file.bytes.insert(file.bytes.end(), digest, digest + sizeof(digest));
		file.bytes.insert(file.bytes.end(), payload.begin(), payload.end());
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (!file.valid || file.bytes.size() > domain_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

flatfile_player_domain_result decode_envelope(const std::vector<uint8_t> &bytes,
					      const std::array<uint8_t, 8> &magic, decoder *payload,
					      uint64_t *revision, uint32_t *format_version)
{
	constexpr size_t header_size =
		8 + sizeof(uint32_t) * 2 + sizeof(uint64_t) + SHA256_DIGEST_LENGTH;
	if (!payload || !revision || !format_version || bytes.size() < header_size ||
	    memcmp(bytes.data(), magic.data(), magic.size()))
		return flatfile_player_domain_result::invalid;
	decoder header{ bytes.data() + magic.size(), bytes.size() - magic.size() };
	uint32_t version = 0, payload_size = 0;
	if (!header.number(&version) || !header.number(&payload_size) || !header.number(revision) ||
	    !version || version > domain_format_version || !*revision ||
	    payload_size != bytes.size() - header_size)
		return flatfile_player_domain_result::invalid;
	const uint8_t *digest =
		bytes.data() + magic.size() + sizeof(uint32_t) * 2 + sizeof(uint64_t);
	const uint8_t *data = bytes.data() + header_size;
	unsigned char actual[SHA256_DIGEST_LENGTH];
	SHA256(data, payload_size, actual);
	if (CRYPTO_memcmp(digest, actual, sizeof(actual)))
		return flatfile_player_domain_result::invalid;
	*payload = { data, payload_size };
	*format_version = version;
	return flatfile_player_domain_result::ok;
}

flatfile_player_domain_result read_bytes(const std::string &root, const std::string &filename,
					 std::vector<uint8_t> *bytes, std::string *error)
{
	const flatfile_read_result read = flatfile_read(domains_directory(root), filename,
							domain_maximum_bytes, bytes, error);
	if (read == flatfile_read_result::not_found)
		return flatfile_player_domain_result::not_found;
	if (read == flatfile_read_result::invalid)
		return flatfile_player_domain_result::invalid;
	return read == flatfile_read_result::ok ? flatfile_player_domain_result::ok :
						  flatfile_player_domain_result::io_error;
}

flatfile_player_domain_result load_bank(const std::string &root, const std::string &account,
					int8_t racewar, bank_record *record, std::string *error)
{
	if (!record)
		return flatfile_player_domain_result::invalid;
	std::vector<uint8_t> bytes;
	const auto read = read_bytes(root, bank_filename(account, racewar), &bytes, error);
	if (read != flatfile_player_domain_result::ok)
		return read;
	decoder payload{ nullptr, 0 };
	uint64_t revision = 0;
	uint32_t format_version = 0;
	if (decode_envelope(bytes, bank_magic, &payload, &revision, &format_version) !=
	    flatfile_player_domain_result::ok)
		return flatfile_player_domain_result::invalid;
	(void)format_version;
	bank_record decoded;
	if (!payload.string(&decoded.account_name) || !payload.number(&decoded.racewar))
		return flatfile_player_domain_result::invalid;
	for (uint64_t &balance : decoded.balances)
		if (!payload.number(&balance))
			return flatfile_player_domain_result::invalid;
	std::string canonical;
	if (payload.offset != payload.size ||
	    !canonical_account(decoded.account_name, &canonical) || canonical != account ||
	    decoded.racewar != racewar)
		return flatfile_player_domain_result::invalid;
	decoded.revision = revision;
	*record = std::move(decoded);
	return flatfile_player_domain_result::ok;
}

bool encode_bank_record(const bank_record &record, std::vector<uint8_t> *bytes)
{
	encoder payload;
	payload.string(record.account_name);
	payload.number(record.racewar);
	for (uint64_t balance : record.balances)
		payload.number(balance);
	return payload.valid && encode_file(bank_magic, payload.bytes, record.revision, bytes);
}

bool publish_bank(const std::string &root, const bank_record &record, std::string *error)
{
	std::vector<uint8_t> bytes;
	return encode_bank_record(record, &bytes) &&
	       flatfile_atomic_write(domains_directory(root),
				     bank_filename(record.account_name, record.racewar), bytes,
				     error);
}

flatfile_player_domain_result load_player_authority(const std::string &root, int32_t pid,
						    player_authority *authority, std::string *error)
{
	if (!authority || pid <= 0)
		return flatfile_player_domain_result::invalid;
	std::vector<uint8_t> bytes;
	const auto read = read_bytes(root, player_filename(pid), &bytes, error);
	if (read != flatfile_player_domain_result::ok)
		return read;
	decoder payload{ nullptr, 0 };
	uint64_t file_revision = 0;
	uint32_t format_version = 0;
	if (decode_envelope(bytes, player_magic, &payload, &file_revision, &format_version) !=
	    flatfile_player_domain_result::ok)
		return flatfile_player_domain_result::invalid;
	player_authority decoded;
	uint32_t recent_count = 0, zone_count = 0, operation_count = 0;
	if (!payload.number(&decoded.record.pid) || !payload.string(&decoded.record.account_name) ||
	    !payload.number(&decoded.record.racewar) ||
	    !payload.number(&decoded.record.domains.wallet_revision) ||
	    !payload.number(&decoded.record.domains.epic_revision) ||
	    !payload.number(&decoded.record.domains.frag_revision))
		return flatfile_player_domain_result::invalid;
	for (uint64_t &balance : decoded.record.domains.wallet)
		if (!payload.number(&balance))
			return flatfile_player_domain_result::invalid;
	if (!payload.number(&decoded.record.domains.epics) ||
	    !payload.number(&decoded.record.domains.frags) ||
	    !payload.number(&decoded.record.domains.old_frags) || !payload.number(&recent_count) ||
	    recent_count > PLAYER_LOAD_RECENT_PVP_MAX)
		return flatfile_player_domain_result::invalid;
	try
	{
		decoded.record.recent_pvp_deaths.resize(recent_count);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_player_domain_result::io_error;
	}
	for (int64_t &death : decoded.record.recent_pvp_deaths)
		if (!payload.number(&death))
			return flatfile_player_domain_result::invalid;
	if (!payload.number(&zone_count) || zone_count > PLAYER_LOAD_COMPLETED_ZONE_MAX)
		return flatfile_player_domain_result::invalid;
	try
	{
		decoded.record.completed_epic_zones.resize(zone_count);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_player_domain_result::io_error;
	}
	for (int32_t &zone : decoded.record.completed_epic_zones)
		if (!payload.number(&zone))
			return flatfile_player_domain_result::invalid;
	if (format_version >= 2)
	{
		if (!payload.number(&operation_count) ||
		    operation_count > domain_maximum_operations)
			return flatfile_player_domain_result::invalid;
		try
		{
			decoded.operations.resize(operation_count);
		}
		catch (const std::bad_alloc &)
		{
			return flatfile_player_domain_result::io_error;
		}
		for (domain_operation &operation : decoded.operations)
			if (!payload.raw(operation.operation_id.bytes.data(),
					 operation.operation_id.bytes.size()) ||
			    !payload.raw(operation.command_digest.data(),
					 operation.command_digest.size()) ||
			    !payload.number(&operation.result_code) ||
			    !payload.number(&operation.result_size) ||
			    operation.result_size > operation.result.size() ||
			    !payload.raw(operation.result.data(), operation.result_size) ||
			    critical_operation_id_is_zero(operation.operation_id))
				return flatfile_player_domain_result::invalid;
	}
	std::string canonical;
	if (payload.offset != payload.size || decoded.record.pid != pid ||
	    !canonical_account(decoded.record.account_name, &canonical) ||
	    decoded.record.account_name != canonical || !valid_gameplay(decoded.record) ||
	    file_revision != std::max({ decoded.record.domains.wallet_revision,
					decoded.record.domains.epic_revision,
					decoded.record.domains.frag_revision, UINT64_C(1) }))
		return flatfile_player_domain_result::invalid;
	for (size_t index = 0; index < decoded.operations.size(); ++index)
		for (size_t other = index + 1; other < decoded.operations.size(); ++other)
			if (critical_operation_id_equal(decoded.operations[index].operation_id,
							decoded.operations[other].operation_id))
				return flatfile_player_domain_result::invalid;
	*authority = std::move(decoded);
	return flatfile_player_domain_result::ok;
}

flatfile_player_domain_result load_player(const std::string &root, int32_t pid,
					  flatfile_player_domain_record *record, std::string *error)
{
	player_authority authority;
	const auto loaded = load_player_authority(root, pid, &authority, error);
	if (loaded == flatfile_player_domain_result::ok)
		*record = std::move(authority.record);
	return loaded;
}

bool encode_player_authority(const player_authority &authority, std::vector<uint8_t> *bytes)
{
	const flatfile_player_domain_record &record = authority.record;
	encoder payload;
	payload.number(record.pid);
	payload.string(record.account_name);
	payload.number(record.racewar);
	payload.number(record.domains.wallet_revision);
	payload.number(record.domains.epic_revision);
	payload.number(record.domains.frag_revision);
	for (uint64_t balance : record.domains.wallet)
		payload.number(balance);
	payload.number(record.domains.epics);
	payload.number(record.domains.frags);
	payload.number(record.domains.old_frags);
	payload.number<uint32_t>(record.recent_pvp_deaths.size());
	for (int64_t death : record.recent_pvp_deaths)
		payload.number(death);
	payload.number<uint32_t>(record.completed_epic_zones.size());
	for (int32_t zone : record.completed_epic_zones)
		payload.number(zone);
	payload.number<uint32_t>(authority.operations.size());
	for (const domain_operation &operation : authority.operations)
	{
		payload.raw(operation.operation_id.bytes.data(),
			    operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number(operation.result_code);
		payload.number(operation.result_size);
		payload.raw(operation.result.data(), operation.result_size);
	}
	const uint64_t revision =
		std::max({ record.domains.wallet_revision, record.domains.epic_revision,
			   record.domains.frag_revision, UINT64_C(1) });
	return payload.valid && encode_file(player_magic, payload.bytes, revision, bytes);
}

player_publish_result publish_player_authority(const std::string &root,
					       const player_authority &authority,
					       std::string *error)
{
	std::vector<uint8_t> bytes;
	if (!encode_player_authority(authority, &bytes))
		return player_publish_result::invalid;
	return flatfile_atomic_write(domains_directory(root), player_filename(authority.record.pid),
				     bytes, error) ?
		       player_publish_result::ok :
		       player_publish_result::io_error;
}

bool publish_player(const std::string &root, const flatfile_player_domain_record &record,
		    std::string *error)
{
	return publish_player_authority(root, { record, {} }, error) == player_publish_result::ok;
}

flatfile_player_domain_result establish(const std::string &root,
					const flatfile_player_domain_record &record,
					bool require_bank_match, std::string *error)
{
	std::string account;
	if (root.empty() || record.pid <= 0 || !canonical_account(record.account_name, &account) ||
	    record.domains.wallet_revision || record.domains.epic_revision ||
	    record.domains.frag_revision || record.domains.bank_revision ||
	    !valid_gameplay(record) ||
	    (!require_bank_match && record.domains.bank != std::array<uint64_t, 4>{}))
		return flatfile_player_domain_result::invalid;
	std::lock_guard<std::mutex> guard(domain_mutex);
	authority_lock authority;
	if (!flatfile_lock_acquire(domains_directory(root), domain_lock_filename, &authority.fd,
				   error))
		return flatfile_player_domain_result::io_error;
	flatfile_player_domain_record existing;
	const auto player_loaded = load_player(root, record.pid, &existing, error);
	if (player_loaded == flatfile_player_domain_result::ok)
	{
		if (existing.account_name != account || existing.racewar != record.racewar ||
		    existing.domains.wallet != record.domains.wallet ||
		    existing.domains.epics != record.domains.epics ||
		    existing.domains.frags != record.domains.frags ||
		    existing.domains.old_frags != record.domains.old_frags ||
		    existing.recent_pvp_deaths != record.recent_pvp_deaths ||
		    existing.completed_epic_zones != record.completed_epic_zones)
			return flatfile_player_domain_result::conflict;
		bank_record bank;
		const auto bank_loaded = load_bank(root, account, record.racewar, &bank, error);
		if (bank_loaded != flatfile_player_domain_result::ok)
			return bank_loaded;
		return !require_bank_match || bank.balances == record.domains.bank ?
			       flatfile_player_domain_result::ok :
			       flatfile_player_domain_result::conflict;
	}
	if (player_loaded != flatfile_player_domain_result::not_found)
		return player_loaded;
	bank_record bank;
	const auto bank_loaded = load_bank(root, account, record.racewar, &bank, error);
	if (bank_loaded == flatfile_player_domain_result::not_found)
	{
		bank = { account, record.racewar, 1, record.domains.bank };
		if (!publish_bank(root, bank, error))
			return flatfile_player_domain_result::io_error;
	}
	else if (bank_loaded != flatfile_player_domain_result::ok)
		return bank_loaded;
	else if (require_bank_match && bank.balances != record.domains.bank)
		return flatfile_player_domain_result::conflict;
	flatfile_player_domain_record canonical = record;
	canonical.account_name = account;
	if (!publish_player(root, canonical, error))
		return flatfile_player_domain_result::io_error;
	return flatfile_player_domain_result::ok;
}
} // namespace

flatfile_player_domain_result
flatfile_player_domain_establish(const std::string &root,
				 const flatfile_player_domain_record &record, std::string *error)
{
	return establish(root, record, true, error);
}

flatfile_player_domain_result flatfile_player_domain_establish_initial_player(
	const std::string &root, const flatfile_player_domain_record &record, std::string *error)
{
	return establish(root, record, false, error);
}

flatfile_player_domain_result flatfile_player_domain_load(const std::string &root, int32_t pid,
							  const std::string &account_name,
							  int8_t racewar,
							  flatfile_player_domain_record *record,
							  std::string *error)
{
	std::string account;
	if (!record || pid <= 0 || !canonical_account(account_name, &account))
		return flatfile_player_domain_result::invalid;
	std::lock_guard<std::mutex> guard(domain_mutex);
	flatfile_player_domain_record loaded;
	const auto player_loaded = load_player(root, pid, &loaded, error);
	if (player_loaded != flatfile_player_domain_result::ok)
		return player_loaded;
	if (loaded.account_name != account || loaded.racewar != racewar)
		return flatfile_player_domain_result::conflict;
	bank_record bank;
	const auto bank_loaded = load_bank(root, account, racewar, &bank, error);
	if (bank_loaded != flatfile_player_domain_result::ok)
		return bank_loaded;
	loaded.domains.bank = bank.balances;
	loaded.domains.bank_revision = bank.revision;
	*record = std::move(loaded);
	return flatfile_player_domain_result::ok;
}

critical_apply_result flatfile_player_domain_apply(const std::string &root,
						   const critical_command &command)
{
	epic_command_payload payload = {};
	std::vector<uint8_t> encoded_command;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !epic_command_decode_payload(command, &payload) ||
	    critical_command_encode(command, &encoded_command) != critical_command_codec_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	SHA256(encoded_command.data(), encoded_command.size(), digest.data());
	std::lock_guard<std::mutex> guard(domain_mutex);
	authority_lock lock;
	std::string error;
	if (!flatfile_lock_acquire(domains_directory(root), domain_lock_filename, &lock.fd, &error))
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	player_authority authority;
	const auto loaded = load_player_authority(root, payload.pid, &authority, &error);
	if (loaded != flatfile_player_domain_result::ok)
		return { loaded == flatfile_player_domain_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 loaded == flatfile_player_domain_result::not_found ? ENOENT :
				 loaded == flatfile_player_domain_result::io_error  ? EIO :
										      EILSEQ) };
	for (const domain_operation &operation : authority.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure,
					 authority.record.domains.epic_revision, EEXIST };
			epic_command_result replay = {};
			if (!epic_command_decode_result(operation.result.data(),
							operation.result_size, &replay))
				return { critical_apply_outcome::terminal_failure, 0, EILSEQ };
			critical_apply_result result = {
				operation.result_code ? critical_apply_outcome::terminal_failure :
							critical_apply_outcome::already_applied,
				replay.revision, operation.result_code
			};
			result.result_size = operation.result_size;
			std::copy_n(operation.result.begin(), operation.result_size,
				    result.result_payload.begin());
			return result;
		}
	if (authority.operations.size() >= domain_maximum_operations)
		return { critical_apply_outcome::terminal_failure,
			 authority.record.domains.epic_revision, ENOSPC };
	epic_command_result epic_result = { authority.record.domains.epics,
					    authority.record.domains.epic_revision, payload.delta };
	unsigned int result_code = 0;
	const uint64_t expected = command.expected_revisions[0].revision;
	if (expected != std::numeric_limits<uint64_t>::max() && expected != epic_result.revision)
		result_code = ESTALE;
	else if (payload.delta < 0 && (payload.flags & EPIC_COMMAND_REQUIRE_FUNDS) &&
		 (payload.delta == std::numeric_limits<int64_t>::min() ||
		  epic_result.balance < -payload.delta))
		result_code = ENOSPC;
	else if ((payload.delta > 0 &&
		  epic_result.balance > std::numeric_limits<int64_t>::max() - payload.delta) ||
		 (payload.delta < 0 &&
		  epic_result.balance < std::numeric_limits<int64_t>::min() - payload.delta) ||
		 epic_result.revision == std::numeric_limits<uint64_t>::max())
		result_code = ERANGE;
	else
	{
		epic_result.balance += payload.delta;
		++epic_result.revision;
		authority.record.domains.epics = epic_result.balance;
		authority.record.domains.epic_revision = epic_result.revision;
	}
	std::array<uint8_t, EPIC_RESULT_PAYLOAD_BYTES> encoded_result = {};
	if (!epic_command_encode_result(epic_result, &encoded_result))
		return { critical_apply_outcome::terminal_failure, epic_result.revision, EBADMSG };
	domain_operation operation = {};
	operation.operation_id = command.operation_id;
	operation.command_digest = digest;
	operation.result_code = result_code;
	operation.result_size = encoded_result.size();
	std::copy(encoded_result.begin(), encoded_result.end(), operation.result.begin());
	try
	{
		authority.operations.push_back(operation);
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, epic_result.revision, ENOMEM };
	}
	const player_publish_result published = publish_player_authority(root, authority, &error);
	if (published != player_publish_result::ok)
		return { published == player_publish_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 epic_result.revision,
			 static_cast<unsigned int>(
				 published == player_publish_result::io_error ? EIO : ENOSPC) };
	critical_apply_result result = { result_code ? critical_apply_outcome::terminal_failure :
						       critical_apply_outcome::applied,
					 epic_result.revision, result_code };
	result.result_size = encoded_result.size();
	std::copy(encoded_result.begin(), encoded_result.end(), result.result_payload.begin());
	return result;
}
