#include "flatfile_boon_repository.h"

#include "boon.h"
#include "boon_shop_command.h"
#include "flatfile_authority_transaction.h"
#include "flatfile_player_domain_repository.h"
#include "flatfile_store.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cerrno>
#include <climits>
#include <cmath>
#include <cstring>
#include <limits>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <type_traits>
#include <unordered_set>

namespace
{
constexpr std::array<uint8_t, 8> catalog_magic = { 'D', 'U', 'R', 'B', 'O', 'O', 'N', 0 };
constexpr uint32_t catalog_version = 3;
constexpr uint32_t catalog_reward_event_version = 2;
constexpr uint32_t catalog_legacy_version = 1;
constexpr size_t catalog_maximum_bytes = 256 * 1024 * 1024;
constexpr size_t definition_maximum = 65536;
constexpr size_t progress_maximum = 1048576;
constexpr size_t shop_maximum = 1048576;
constexpr size_t operation_maximum = 32768;
constexpr size_t author_maximum_bytes = 255;
constexpr const char *catalog_filename = "boon_catalog";

struct boon_progress
{
	uint32_t boon_id = 0;
	uint32_t pid = 0;
	double counter = 0;
};

struct boon_shop_record
{
	uint32_t pid = 0;
	int64_t points = 0;
	int64_t stats = 0;
};

struct boon_operation
{
	critical_operation_id operation_id = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest = {};
	unsigned int result_code = 0;
	boon_reward_result result = {};
	double event_data = 0;
	bool reward_published = false;
};

struct boon_shop_operation
{
	critical_operation_id operation_id = {};
	std::array<uint8_t, SHA256_DIGEST_LENGTH> command_digest = {};
	unsigned int result_code = 0;
	boon_shop_result result = {};
};

struct boon_catalog
{
	uint64_t revision = 0;
	std::vector<flatfile_boon_definition> definitions;
	std::vector<boon_progress> progress;
	std::vector<boon_shop_record> shops;
	std::vector<boon_operation> operations;
	std::vector<boon_shop_operation> shop_operations;
};

struct operation_id_hash
{
	size_t operator()(const std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES> &value) const
	{
		size_t result = 0;
		for (uint8_t byte : value)
			result = result * 131 + byte;
		return result;
	}
};

struct encoder
{
	std::vector<uint8_t> bytes;
	bool valid = true;

	template <typename T> void number(T value)
	{
		using U = std::make_unsigned_t<T>;
		U bits = static_cast<U>(value);
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

	void string(const std::string &value)
	{
		number<uint16_t>(value.size());
		raw(reinterpret_cast<const uint8_t *>(value.data()), value.size());
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
		using U = std::make_unsigned_t<T>;
		U bits = 0;
		for (size_t index = 0; index < sizeof(T); ++index)
			bits |= static_cast<U>(data[offset++]) << (index * 8);
		*value = static_cast<T>(bits);
		return true;
	}

	bool raw(uint8_t *output, size_t count)
	{
		if (!output || size - offset < count)
			return false;
		memcpy(output, data + offset, count);
		offset += count;
		return true;
	}

	bool string(std::string *value, size_t maximum)
	{
		uint16_t count = 0;
		if (!value || !number(&count) || count > maximum || size - offset < count)
			return false;
		try
		{
			value->assign(reinterpret_cast<const char *>(data + offset), count);
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
		offset += count;
		return value->find('\0') == std::string::npos;
	}
};

std::string domains_directory(const std::string &root)
{
	return root + "/domains";
}

bool valid_definition(const flatfile_boon_definition &definition)
{
	return definition.id && definition.start_time >= 0 && definition.duration >= -1 &&
	       definition.type < MAX_BTYPE && definition.option < MAX_BOPT &&
	       definition.author.size() <= author_maximum_bytes &&
	       definition.author.find('\0') == std::string::npos &&
	       std::isfinite(definition.criteria) && std::isfinite(definition.criteria2) &&
	       std::isfinite(definition.bonus) && std::isfinite(definition.bonus2);
}

bool valid_result(const boon_reward_result &result)
{
	if (!result.pid || result.entry_count > result.entries.size())
		return false;
	for (size_t index = 0; index < result.entry_count; ++index)
	{
		const auto &entry = result.entries[index];
		if (!entry.boon_id || entry.type >= MAX_BTYPE || entry.option >= MAX_BOPT ||
		    (entry.flags & ~(BOON_RESULT_PROGRESS | BOON_RESULT_COMPLETED)) ||
		    entry.repeat > 1 || !std::isfinite(entry.criteria) ||
		    !std::isfinite(entry.criteria2) || !std::isfinite(entry.bonus) ||
		    !std::isfinite(entry.bonus2) || !std::isfinite(entry.counter))
			return false;
	}
	return true;
}

bool progress_less(const boon_progress &left, const boon_progress &right)
{
	return left.boon_id != right.boon_id ? left.boon_id < right.boon_id : left.pid < right.pid;
}

boon_progress *find_progress(boon_catalog *catalog, uint32_t boon_id, uint32_t pid)
{
	const boon_progress candidate = { boon_id, pid, 0 };
	auto found = std::lower_bound(catalog->progress.begin(), catalog->progress.end(), candidate,
				      progress_less);
	return found != catalog->progress.end() && found->boon_id == boon_id && found->pid == pid ?
		       &*found :
		       nullptr;
}

boon_shop_record *find_shop(boon_catalog *catalog, uint32_t pid)
{
	auto found = std::lower_bound(catalog->shops.begin(), catalog->shops.end(), pid,
				      [](const boon_shop_record &shop, uint32_t candidate)
				      { return shop.pid < candidate; });
	return found != catalog->shops.end() && found->pid == pid ? &*found : nullptr;
}

bool encode_catalog(const boon_catalog &catalog, std::vector<uint8_t> *bytes)
{
	if (!bytes || catalog.definitions.size() > definition_maximum ||
	    catalog.progress.size() > progress_maximum || catalog.shops.size() > shop_maximum ||
	    catalog.operations.size() > operation_maximum ||
	    catalog.shop_operations.size() > operation_maximum)
		return false;
	encoder payload;
	payload.number<uint32_t>(catalog.definitions.size());
	for (const auto &definition : catalog.definitions)
	{
		if (!valid_definition(definition))
			return false;
		payload.number(definition.id);
		payload.number(definition.start_time);
		payload.number(definition.duration);
		payload.number(definition.racewar);
		payload.number(definition.type);
		payload.number(definition.option);
		payload.number<uint8_t>(definition.random);
		payload.number<uint8_t>(definition.active);
		payload.number<uint8_t>(definition.repeat);
		payload.number(definition.target_pid);
		payload.number(std::bit_cast<uint64_t>(definition.criteria));
		payload.number(std::bit_cast<uint64_t>(definition.criteria2));
		payload.number(std::bit_cast<uint64_t>(definition.bonus));
		payload.number(std::bit_cast<uint64_t>(definition.bonus2));
		payload.string(definition.author);
	}
	payload.number<uint32_t>(catalog.progress.size());
	for (const auto &progress : catalog.progress)
	{
		if (!progress.boon_id || !progress.pid || !std::isfinite(progress.counter))
			return false;
		payload.number(progress.boon_id);
		payload.number(progress.pid);
		payload.number(std::bit_cast<uint64_t>(progress.counter));
	}
	payload.number<uint32_t>(catalog.shops.size());
	for (const auto &shop : catalog.shops)
	{
		if (!shop.pid)
			return false;
		payload.number(shop.pid);
		payload.number(shop.points);
		payload.number(shop.stats);
	}
	payload.number<uint32_t>(catalog.operations.size());
	for (const auto &operation : catalog.operations)
	{
		if (!valid_result(operation.result) || !std::isfinite(operation.event_data))
			return false;
		payload.raw(operation.operation_id.bytes.data(),
			    operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number(operation.result_code);
		std::array<uint8_t, BOON_REWARD_RESULT_BYTES> result = {};
		if (!boon_reward_command_encode_result(operation.result, &result))
			return false;
		payload.raw(result.data(), result.size());
		payload.number(std::bit_cast<uint64_t>(operation.event_data));
		payload.number<uint8_t>(operation.reward_published);
	}
	payload.number<uint32_t>(catalog.shop_operations.size());
	for (const auto &operation : catalog.shop_operations)
	{
		payload.raw(operation.operation_id.bytes.data(),
			    operation.operation_id.bytes.size());
		payload.raw(operation.command_digest.data(), operation.command_digest.size());
		payload.number(operation.result_code);
		std::array<uint8_t, BOON_SHOP_RESULT_BYTES> result = {};
		if (!boon_shop_command_encode_result(operation.result, &result))
			return false;
		payload.raw(result.data(), result.size());
	}
	if (!payload.valid || payload.bytes.size() > catalog_maximum_bytes)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.bytes.data(), payload.bytes.size(), digest.data());
	encoder file;
	file.raw(catalog_magic.data(), catalog_magic.size());
	file.number(catalog_version);
	file.number<uint32_t>(payload.bytes.size());
	file.number(std::max(catalog.revision, UINT64_C(1)));
	file.raw(digest.data(), digest.size());
	file.raw(payload.bytes.data(), payload.bytes.size());
	if (!file.valid || file.bytes.size() > catalog_maximum_bytes)
		return false;
	*bytes = std::move(file.bytes);
	return true;
}

bool decode_catalog(const std::vector<uint8_t> &bytes, boon_catalog *catalog)
{
	constexpr size_t header_size = 8 + 4 + 4 + 8 + SHA256_DIGEST_LENGTH;
	if (!catalog || bytes.size() < header_size ||
	    memcmp(bytes.data(), catalog_magic.data(), catalog_magic.size()))
		return false;
	decoder header{ bytes.data() + 8, bytes.size() - 8 };
	uint32_t version = 0, payload_size = 0;
	uint64_t revision = 0;
	if (!header.number(&version) || !header.number(&payload_size) ||
	    !header.number(&revision) ||
	    (version != catalog_version && version != catalog_reward_event_version &&
	     version != catalog_legacy_version) ||
	    !revision || payload_size != bytes.size() - header_size)
		return false;
	const uint8_t *payload_bytes = bytes.data() + header_size;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload_bytes, payload_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + 24, digest.data(), digest.size()))
		return false;
	decoder payload{ payload_bytes, payload_size };
	boon_catalog decoded;
	decoded.revision = revision;
	uint32_t count = 0;
	if (!payload.number(&count) || count > definition_maximum)
		return false;
	try
	{
		decoded.definitions.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &definition : decoded.definitions)
	{
		uint8_t random = 0, active = 0, repeat = 0;
		uint64_t criteria = 0, criteria2 = 0, bonus = 0, bonus2 = 0;
		if (!payload.number(&definition.id) || !payload.number(&definition.start_time) ||
		    !payload.number(&definition.duration) || !payload.number(&definition.racewar) ||
		    !payload.number(&definition.type) || !payload.number(&definition.option) ||
		    !payload.number(&random) || !payload.number(&active) ||
		    !payload.number(&repeat) || !payload.number(&definition.target_pid) ||
		    !payload.number(&criteria) || !payload.number(&criteria2) ||
		    !payload.number(&bonus) || !payload.number(&bonus2) || random > 1 ||
		    active > 1 || repeat > 1 ||
		    !payload.string(&definition.author, author_maximum_bytes))
			return false;
		definition.random = random;
		definition.active = active;
		definition.repeat = repeat;
		definition.criteria = std::bit_cast<double>(criteria);
		definition.criteria2 = std::bit_cast<double>(criteria2);
		definition.bonus = std::bit_cast<double>(bonus);
		definition.bonus2 = std::bit_cast<double>(bonus2);
		if (!valid_definition(definition))
			return false;
	}
	if (!std::is_sorted(decoded.definitions.begin(), decoded.definitions.end(),
			    [](const auto &left, const auto &right) { return left.id < right.id; }))
		return false;
	for (size_t index = 1; index < decoded.definitions.size(); ++index)
		if (decoded.definitions[index - 1].id == decoded.definitions[index].id)
			return false;
	if (!payload.number(&count) || count > progress_maximum)
		return false;
	try
	{
		decoded.progress.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &progress : decoded.progress)
	{
		uint64_t counter = 0;
		if (!payload.number(&progress.boon_id) || !payload.number(&progress.pid) ||
		    !payload.number(&counter))
			return false;
		progress.counter = std::bit_cast<double>(counter);
		if (!progress.boon_id || !progress.pid || !std::isfinite(progress.counter))
			return false;
	}
	if (!std::is_sorted(decoded.progress.begin(), decoded.progress.end(), progress_less))
		return false;
	for (size_t index = 0; index < decoded.progress.size(); ++index)
	{
		const auto definition = std::lower_bound(decoded.definitions.begin(),
							 decoded.definitions.end(),
							 decoded.progress[index].boon_id,
							 [](const auto &candidate, uint32_t id)
							 { return candidate.id < id; });
		if (definition == decoded.definitions.end() ||
		    definition->id != decoded.progress[index].boon_id)
			return false;
		if (index &&
		    decoded.progress[index - 1].boon_id == decoded.progress[index].boon_id &&
		    decoded.progress[index - 1].pid == decoded.progress[index].pid)
			return false;
	}
	if (!payload.number(&count) || count > shop_maximum)
		return false;
	try
	{
		decoded.shops.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	for (auto &shop : decoded.shops)
		if (!payload.number(&shop.pid) || !payload.number(&shop.points) ||
		    !payload.number(&shop.stats) || !shop.pid)
			return false;
	if (!std::is_sorted(decoded.shops.begin(), decoded.shops.end(),
			    [](const auto &left, const auto &right)
			    { return left.pid < right.pid; }))
		return false;
	for (size_t index = 1; index < decoded.shops.size(); ++index)
		if (decoded.shops[index - 1].pid == decoded.shops[index].pid)
			return false;
	if (!payload.number(&count) || count > operation_maximum)
		return false;
	try
	{
		decoded.operations.resize(count);
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	std::unordered_set<std::array<uint8_t, CRITICAL_COMMAND_ID_BYTES>, operation_id_hash> ids;
	std::array<uint8_t, BOON_REWARD_RESULT_BYTES> result = {};
	try
	{
		ids.reserve(count);
		for (auto &operation : decoded.operations)
		{
			uint64_t event_data = 0;
			uint8_t reward_published = version == catalog_legacy_version;
			if (!payload.raw(operation.operation_id.bytes.data(),
					 operation.operation_id.bytes.size()) ||
			    !payload.raw(operation.command_digest.data(),
					 operation.command_digest.size()) ||
			    !payload.number(&operation.result_code) ||
			    !payload.raw(result.data(), result.size()) ||
			    (version >= catalog_reward_event_version &&
			     (!payload.number(&event_data) ||
			      !payload.number(&reward_published))) ||
			    reward_published > 1 ||
			    critical_operation_id_is_zero(operation.operation_id) ||
			    !boon_reward_command_decode_result(result.data(), result.size(),
							       &operation.result) ||
			    !valid_result(operation.result) ||
			    !ids.insert(operation.operation_id.bytes).second)
				return false;
			operation.event_data = std::bit_cast<double>(event_data);
			operation.reward_published = reward_published;
			if (!std::isfinite(operation.event_data))
				return false;
		}
	}
	catch (const std::bad_alloc &)
	{
		return false;
	}
	if (version == catalog_version)
	{
		if (!payload.number(&count) || count > operation_maximum)
			return false;
		try
		{
			decoded.shop_operations.resize(count);
			ids.reserve(ids.size() + count);
			std::array<uint8_t, BOON_SHOP_RESULT_BYTES> shop_result = {};
			for (auto &operation : decoded.shop_operations)
				if (!payload.raw(operation.operation_id.bytes.data(),
						 operation.operation_id.bytes.size()) ||
				    !payload.raw(operation.command_digest.data(),
						 operation.command_digest.size()) ||
				    !payload.number(&operation.result_code) ||
				    !payload.raw(shop_result.data(), shop_result.size()) ||
				    critical_operation_id_is_zero(operation.operation_id) ||
				    !boon_shop_command_decode_result(shop_result.data(),
								     shop_result.size(),
								     &operation.result) ||
				    !ids.insert(operation.operation_id.bytes).second)
					return false;
		}
		catch (const std::bad_alloc &)
		{
			return false;
		}
	}
	if (payload.offset != payload.size)
		return false;
	*catalog = std::move(decoded);
	return true;
}

flatfile_read_result load_catalog(const std::string &root, boon_catalog *catalog,
				  std::string *error)
{
	std::vector<uint8_t> bytes;
	const auto read = flatfile_read(domains_directory(root), catalog_filename,
					catalog_maximum_bytes, &bytes, error);
	if (read == flatfile_read_result::not_found)
	{
		*catalog = {};
		return read;
	}
	if (read != flatfile_read_result::ok)
		return read;
	return decode_catalog(bytes, catalog) ? flatfile_read_result::ok :
						flatfile_read_result::invalid;
}

bool eligible(const flatfile_boon_definition &definition, const boon_reward_payload &payload)
{
	if (!definition.active || definition.option != payload.option ||
	    (definition.racewar && definition.racewar != payload.racewar) ||
	    (definition.target_pid && definition.target_pid != payload.pid))
		return false;
	switch (payload.option)
	{
	case BOPT_NONE:
		return definition.criteria == payload.zone_number;
	case BOPT_ZONE:
	case BOPT_OP:
	case BOPT_NEXUS:
	case BOPT_CTF:
	case BOPT_CTFB:
		return definition.criteria == payload.data;
	case BOPT_LEVEL:
		return definition.criteria == payload.level || definition.criteria == 0;
	case BOPT_MOB:
		return (payload.victim_flags & 1) &&
		       (definition.criteria2 == payload.victim_vnum || definition.criteria2 == -1);
	case BOPT_RACE:
		return (payload.victim_flags & 2) && definition.criteria2 == payload.victim_race;
	case BOPT_FRAG:
		return definition.criteria <= payload.data;
	default:
		return true;
	}
}

bool progress_option(uint8_t option)
{
	return option == BOPT_MOB || option == BOPT_RACE || option == BOPT_FRAGS;
}

critical_apply_result make_result(const boon_operation &operation, uint64_t revision,
				  critical_apply_outcome success)
{
	std::array<uint8_t, BOON_REWARD_RESULT_BYTES> encoded = {};
	if (!boon_reward_command_encode_result(operation.result, &encoded))
		return { critical_apply_outcome::terminal_failure, revision, EILSEQ };
	critical_apply_result result = { operation.result_code ?
						 critical_apply_outcome::terminal_failure :
						 success,
					 revision, operation.result_code };
	result.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), result.result_payload.begin());
	return result;
}

critical_apply_result make_result(const boon_shop_operation &operation, uint64_t revision,
				  critical_apply_outcome success)
{
	std::array<uint8_t, BOON_SHOP_RESULT_BYTES> encoded = {};
	if (!boon_shop_command_encode_result(operation.result, &encoded))
		return { critical_apply_outcome::terminal_failure, revision, EILSEQ };
	critical_apply_result result = { operation.result_code ?
						 critical_apply_outcome::terminal_failure :
						 success,
					 revision, operation.result_code };
	result.result_size = encoded.size();
	std::copy(encoded.begin(), encoded.end(), result.result_payload.begin());
	return result;
}
} // namespace

flatfile_boon_result
flatfile_boon_establish(const std::string &root,
			const std::vector<flatfile_boon_definition> &definitions,
			std::string *error)
{
	if (root.empty() || definitions.size() > definition_maximum)
		return flatfile_boon_result::invalid;
	boon_catalog catalog;
	try
	{
		catalog.definitions = definitions;
		std::sort(catalog.definitions.begin(), catalog.definitions.end(),
			  [](const auto &left, const auto &right) { return left.id < right.id; });
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_boon_result::io_error;
	}
	for (size_t index = 0; index < catalog.definitions.size(); ++index)
		if (!valid_definition(catalog.definitions[index]) ||
		    (index && catalog.definitions[index - 1].id == catalog.definitions[index].id))
			return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog existing;
	const auto loaded = load_catalog(root, &existing, error);
	if (loaded == flatfile_read_result::ok)
		return flatfile_boon_result::already_exists;
	if (loaded != flatfile_read_result::not_found)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	catalog.revision = 1;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_boon_result::invalid;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_boon_result::ok :
		       flatfile_boon_result::io_error;
}

flatfile_boon_result flatfile_boon_create(const std::string &root,
					  flatfile_boon_definition *definition, std::string *error)
{
	if (root.empty() || !definition || definition->id)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	if (catalog.definitions.size() >= definition_maximum ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_boon_result::io_error;
	if (std::count_if(catalog.definitions.begin(), catalog.definitions.end(),
			  [](const auto &entry) { return entry.active; }) >= MAX_BOONS)
		return flatfile_boon_result::invalid;
	const uint32_t last_id = catalog.definitions.empty() ? 0 : catalog.definitions.back().id;
	if (last_id == std::numeric_limits<uint32_t>::max())
		return flatfile_boon_result::io_error;
	flatfile_boon_definition created = *definition;
	created.id = last_id + 1;
	if (!valid_definition(created))
		return flatfile_boon_result::invalid;
	try
	{
		catalog.definitions.push_back(created);
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_boon_result::io_error;
	}
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_boon_result::io_error;
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error))
		return flatfile_boon_result::io_error;
	*definition = std::move(created);
	return flatfile_boon_result::ok;
}

flatfile_boon_result flatfile_boon_deactivate(const std::string &root, uint32_t boon_id,
					      std::string *error)
{
	if (root.empty() || !boon_id)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	auto definition = std::lower_bound(catalog.definitions.begin(), catalog.definitions.end(),
					   boon_id, [](const auto &entry, uint32_t key)
					   { return entry.id < key; });
	if (definition == catalog.definitions.end() || definition->id != boon_id)
		return flatfile_boon_result::not_found;
	if (!definition->active && definition->duration == 0)
		return flatfile_boon_result::ok;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_boon_result::io_error;
	definition->active = false;
	definition->duration = 0;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_boon_result::io_error;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_boon_result::ok :
		       flatfile_boon_result::io_error;
}

flatfile_boon_result flatfile_boon_extend(const std::string &root, uint32_t boon_id,
					  int64_t extend_minutes, int64_t now,
					  const std::string &author, bool *was_active,
					  std::string *error)
{
	if (root.empty() || !boon_id || extend_minutes < 0 || now < 0 || !was_active ||
	    author.empty() || author.size() >= author_maximum_bytes ||
	    author.find('\0') != std::string::npos)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	auto definition = std::lower_bound(catalog.definitions.begin(), catalog.definitions.end(),
					   boon_id, [](const auto &entry, uint32_t key)
					   { return entry.id < key; });
	if (definition == catalog.definitions.end() || definition->id != boon_id)
		return flatfile_boon_result::not_found;
	if (definition->duration == -1)
		return flatfile_boon_result::invalid;
	if (definition->duration > std::numeric_limits<int64_t>::max() / 60)
		return flatfile_boon_result::io_error;
	const int64_t duration_seconds = definition->duration * 60;
	if (definition->start_time > std::numeric_limits<int64_t>::max() - duration_seconds ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_boon_result::io_error;
	const int64_t expires_at = definition->start_time + duration_seconds;
	const int64_t remaining = std::max<int64_t>(0, expires_at - now);
	if (remaining > std::numeric_limits<int64_t>::max() - now)
		return flatfile_boon_result::io_error;
	flatfile_boon_definition updated = *definition;
	*was_active = updated.active;
	updated.start_time = now + remaining;
	updated.duration = extend_minutes;
	updated.active = true;
	updated.author = "*" + author;
	if (!valid_definition(updated))
		return flatfile_boon_result::invalid;
	*definition = std::move(updated);
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_boon_result::io_error;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_boon_result::ok :
		       flatfile_boon_result::io_error;
}

flatfile_boon_result
flatfile_boon_load_definitions(const std::string &root,
			       std::vector<flatfile_boon_definition> *definitions,
			       std::string *error)
{
	if (root.empty() || !definitions)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	try
	{
		*definitions = catalog.definitions;
	}
	catch (const std::bad_alloc &)
	{
		return flatfile_boon_result::io_error;
	}
	return flatfile_boon_result::ok;
}

flatfile_boon_result flatfile_boon_load_progress(const std::string &root, uint32_t boon_id,
						 uint32_t pid, double *counter, std::string *error)
{
	if (root.empty() || !boon_id || !pid || !counter)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	const auto progress = std::lower_bound(
		catalog.progress.begin(), catalog.progress.end(), std::pair{ boon_id, pid },
		[](const boon_progress &entry, const auto &key)
		{ return std::pair{ entry.boon_id, entry.pid } < key; });
	if (progress == catalog.progress.end() || progress->boon_id != boon_id ||
	    progress->pid != pid)
		return flatfile_boon_result::not_found;
	*counter = progress->counter;
	return flatfile_boon_result::ok;
}

flatfile_boon_result flatfile_boon_load_player(const std::string &root, uint32_t pid,
					       flatfile_boon_player_projection *player,
					       std::string *error)
{
	if (root.empty() || !pid || !player)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	if (const boon_shop_record *shop = find_shop(&catalog, pid))
		*player = { shop->points, shop->stats };
	else
		*player = {};
	return flatfile_boon_result::ok;
}

flatfile_boon_result flatfile_boon_find_pending_reward(const std::string &root, uint32_t pid,
						       flatfile_boon_pending_reward *reward,
						       std::string *error)
{
	if (root.empty() || !pid || !reward)
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	const auto operation =
		std::find_if(catalog.operations.begin(), catalog.operations.end(),
			     [&](const boon_operation &entry)
			     { return !entry.reward_published && entry.result.pid == pid; });
	if (operation == catalog.operations.end())
		return flatfile_boon_result::not_found;
	*reward = { operation->operation_id, operation->result.pid, operation->event_data,
		    operation->result };
	return flatfile_boon_result::ok;
}

flatfile_boon_result flatfile_boon_acknowledge_reward(const std::string &root,
						      const critical_operation_id &operation_id,
						      std::string *error)
{
	if (root.empty() || critical_operation_id_is_zero(operation_id))
		return flatfile_boon_result::invalid;
	flatfile_authority_lock lock;
	if (!lock.acquire(root, error))
		return flatfile_boon_result::io_error;
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	auto operation = std::find_if(
		catalog.operations.begin(), catalog.operations.end(),
		[&](const boon_operation &entry)
		{ return critical_operation_id_equal(entry.operation_id, operation_id); });
	if (operation == catalog.operations.end())
		return flatfile_boon_result::not_found;
	if (operation->reward_published)
		return flatfile_boon_result::ok;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_boon_result::io_error;
	operation->reward_published = true;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_boon_result::io_error;
	return flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, error) ?
		       flatfile_boon_result::ok :
		       flatfile_boon_result::io_error;
}

flatfile_boon_result flatfile_boon_prepare_player_remove(const std::string &root,
							 const flatfile_authority_lock &lock,
							 uint32_t pid,
							 flatfile_authority_operation *operation,
							 std::string *error)
{
	if (!operation || !pid || !lock.matches(root))
		return flatfile_boon_result::invalid;
	*operation = {};
	const auto recovered = flatfile_authority_transaction_recover(root, lock, error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return recovered == flatfile_authority_transaction_result::io_error ?
			       flatfile_boon_result::io_error :
			       flatfile_boon_result::invalid;
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, error);
	if (loaded == flatfile_read_result::not_found)
		return flatfile_boon_result::not_found;
	if (loaded != flatfile_read_result::ok)
		return loaded == flatfile_read_result::io_error ? flatfile_boon_result::io_error :
								  flatfile_boon_result::invalid;
	bool changed = false;
	for (auto &definition : catalog.definitions)
	{
		if (definition.target_pid != pid)
			continue;
		definition.target_pid = 0;
		definition.active = false;
		definition.duration = 0;
		changed = true;
	}
	auto erase_matching = [&](auto &records, auto predicate)
	{
		const size_t previous = records.size();
		records.erase(std::remove_if(records.begin(), records.end(), predicate),
			      records.end());
		changed = changed || records.size() != previous;
	};
	erase_matching(catalog.progress, [pid](const auto &entry) { return entry.pid == pid; });
	erase_matching(catalog.shops, [pid](const auto &entry) { return entry.pid == pid; });
	erase_matching(catalog.operations,
		       [pid](const auto &entry) { return entry.result.pid == pid; });
	erase_matching(catalog.shop_operations,
		       [pid](const auto &entry) { return entry.result.pid == pid; });
	if (!changed)
		return flatfile_boon_result::unchanged;
	if (catalog.revision == std::numeric_limits<uint64_t>::max())
		return flatfile_boon_result::invalid;
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return flatfile_boon_result::invalid;
	operation->store = flatfile_authority_store::domains;
	operation->kind = flatfile_authority_operation_kind::write;
	operation->filename = catalog_filename;
	operation->bytes = std::move(bytes);
	return flatfile_boon_result::ok;
}

critical_apply_result flatfile_boon_repository_apply(const std::string &root,
						     const critical_command &command)
{
	boon_reward_payload payload = {};
	std::vector<uint8_t> encoded_command;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !boon_reward_command_decode_payload(command, &payload) ||
	    !std::isfinite(payload.data) ||
	    critical_command_encode(command, &encoded_command) != critical_command_codec_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	SHA256(encoded_command.data(), encoded_command.size(), digest.data());
	flatfile_authority_lock lock;
	std::string error;
	if (!lock.acquire(root, &error))
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	const auto recovered = flatfile_authority_transaction_recover(root, lock, &error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return { recovered == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 recovered == flatfile_authority_transaction_result::io_error ?
					 EIO :
					 EILSEQ) };
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_read_result::ok)
		return { loaded == flatfile_read_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 catalog.revision,
			 static_cast<unsigned int>(
				 loaded == flatfile_read_result::not_found ? ENOENT :
				 loaded == flatfile_read_result::io_error  ? EIO :
									     EILSEQ) };
	for (const auto &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EEXIST };
			return make_result(operation, catalog.revision,
					   critical_apply_outcome::already_applied);
		}
	for (const auto &operation : catalog.shop_operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EEXIST };
	if (catalog.operations.size() >= operation_maximum ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	boon_catalog candidate;
	try
	{
		candidate = catalog;
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	std::vector<size_t> matches;
	try
	{
		for (size_t index = 0; index < candidate.definitions.size(); ++index)
			if (eligible(candidate.definitions[index], payload))
				matches.push_back(index);
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	unsigned int result_code = matches.size() > BOON_REWARD_MAX_RESULTS ? E2BIG : 0;
	boon_reward_result result = {};
	result.pid = payload.pid;
	for (size_t output = 0; !result_code && output < matches.size(); ++output)
	{
		auto &definition = candidate.definitions[matches[output]];
		boon_progress *progress = find_progress(&candidate, definition.id, payload.pid);
		if (!progress)
		{
			if (candidate.progress.size() >= progress_maximum)
			{
				result_code = ENOSPC;
				break;
			}
			const double counter =
				definition.option == BOPT_FRAGS ?
					payload.data :
					(definition.option == BOPT_RACE ||
							 definition.option == BOPT_MOB ?
						 1.0 :
						 0.0);
			try
			{
				candidate.progress.push_back(
					{ definition.id, payload.pid, counter });
				std::sort(candidate.progress.begin(), candidate.progress.end(),
					  progress_less);
			}
			catch (const std::bad_alloc &)
			{
				return { critical_apply_outcome::retryable_failure,
					 catalog.revision, ENOMEM };
			}
			progress = find_progress(&candidate, definition.id, payload.pid);
		}
		else if (progress_option(definition.option) && progress->counter != -1)
			progress->counter += definition.option == BOPT_FRAGS ? payload.data : 1.0;
		if (!progress || !std::isfinite(progress->counter))
		{
			result_code = ERANGE;
			break;
		}
		const bool completed = progress->counter != -1 &&
				       (!progress_option(definition.option) ||
					progress->counter >= definition.criteria);
		progress->counter = completed ? (definition.repeat ? 0.0 : -1.0) :
						progress->counter;
		if (completed && (definition.type == BTYPE_STATS || definition.type == BTYPE_POINT))
		{
			if (definition.bonus < INT_MIN || definition.bonus > INT_MAX)
			{
				result_code = ERANGE;
				break;
			}
			boon_shop_record *shop = find_shop(&candidate, payload.pid);
			if (!shop)
			{
				if (candidate.shops.size() >= shop_maximum)
				{
					result_code = ENOSPC;
					break;
				}
				try
				{
					candidate.shops.push_back({ payload.pid, 0, 0 });
					std::sort(candidate.shops.begin(), candidate.shops.end(),
						  [](const auto &left, const auto &right)
						  { return left.pid < right.pid; });
				}
				catch (const std::bad_alloc &)
				{
					return { critical_apply_outcome::retryable_failure,
						 catalog.revision, ENOMEM };
				}
				shop = find_shop(&candidate, payload.pid);
			}
			const int64_t reward = static_cast<int>(definition.bonus);
			int64_t *balance = definition.type == BTYPE_STATS ? &shop->stats :
									    &shop->points;
			if ((reward > 0 && *balance > INT64_MAX - reward) ||
			    (reward < 0 && *balance < INT64_MIN - reward))
			{
				result_code = ERANGE;
				break;
			}
			*balance += reward;
		}
		if (completed && definition.target_pid == payload.pid && !definition.repeat)
			definition.active = false;
		const uint8_t flags =
			(progress_option(definition.option) ? BOON_RESULT_PROGRESS : 0) |
			(completed ? BOON_RESULT_COMPLETED : 0);
		result.entries[output] = { definition.id,
					   definition.type,
					   definition.option,
					   flags,
					   static_cast<uint8_t>(definition.repeat),
					   definition.criteria,
					   definition.criteria2,
					   definition.bonus,
					   definition.bonus2,
					   progress->counter };
		++result.entry_count;
	}
	if (result_code)
	{
		try
		{
			candidate = catalog;
		}
		catch (const std::bad_alloc &)
		{
			return { critical_apply_outcome::retryable_failure, catalog.revision,
				 ENOMEM };
		}
		result = {};
		result.pid = payload.pid;
	}
	try
	{
		candidate.operations.push_back({ command.operation_id, digest, result_code, result,
						 payload.data,
						 result_code || result.entry_count == 0 });
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	++candidate.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(candidate, &bytes))
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };
	if (!flatfile_atomic_write(domains_directory(root), catalog_filename, bytes, &error))
		return { critical_apply_outcome::retryable_failure, catalog.revision, EIO };
	return make_result(candidate.operations.back(), candidate.revision,
			   critical_apply_outcome::applied);
}

critical_apply_result flatfile_boon_shop_repository_apply(const std::string &root,
							  const critical_command &command)
{
	boon_shop_payload payload = {};
	std::vector<uint8_t> encoded_command;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	if (root.empty() || !critical_command_valid(command) ||
	    !boon_shop_command_decode_payload(command, &payload) ||
	    critical_command_encode(command, &encoded_command) != critical_command_codec_result::ok)
		return { critical_apply_outcome::terminal_failure, 0, EINVAL };
	SHA256(encoded_command.data(), encoded_command.size(), digest.data());
	flatfile_authority_lock lock;
	std::string error;
	if (!lock.acquire(root, &error))
		return { critical_apply_outcome::retryable_failure, 0, EIO };
	const auto recovered = flatfile_authority_transaction_recover(root, lock, &error);
	if (recovered != flatfile_authority_transaction_result::ok)
		return { recovered == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 0,
			 static_cast<unsigned int>(
				 recovered == flatfile_authority_transaction_result::io_error ?
					 EIO :
					 EILSEQ) };
	boon_catalog catalog;
	const auto loaded = load_catalog(root, &catalog, &error);
	if (loaded != flatfile_read_result::ok)
		return { loaded == flatfile_read_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 catalog.revision,
			 static_cast<unsigned int>(
				 loaded == flatfile_read_result::not_found ? ENOENT :
				 loaded == flatfile_read_result::io_error  ? EIO :
									     EILSEQ) };
	for (const auto &operation : catalog.shop_operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
		{
			if (CRYPTO_memcmp(operation.command_digest.data(), digest.data(),
					  digest.size()))
				return { critical_apply_outcome::terminal_failure, catalog.revision,
					 EEXIST };
			return make_result(operation, catalog.revision,
					   critical_apply_outcome::already_applied);
		}
	for (const auto &operation : catalog.operations)
		if (critical_operation_id_equal(operation.operation_id, command.operation_id))
			return { critical_apply_outcome::terminal_failure, catalog.revision,
				 EEXIST };
	if (catalog.shop_operations.size() >= operation_maximum ||
	    catalog.revision == std::numeric_limits<uint64_t>::max())
		return { critical_apply_outcome::terminal_failure, catalog.revision, ENOSPC };

	flatfile_base_stat_mutation stat = {};
	unsigned int result_code = 0;
	const auto inspected = flatfile_player_domain_prepare_base_stat(
		root, lock, payload.pid, payload.stat_index, false, &stat, &result_code, &error);
	if (inspected != flatfile_player_domain_result::ok)
		return { inspected == flatfile_player_domain_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 catalog.revision,
			 static_cast<unsigned int>(
				 inspected == flatfile_player_domain_result::not_found ? ENOENT :
				 inspected == flatfile_player_domain_result::io_error  ? EIO :
											 EILSEQ) };
	boon_shop_record *shop = find_shop(&catalog, payload.pid);
	if (!result_code && (!shop || shop->stats <= 0))
		result_code = ENOSPC;
	bool mutation_applied = false;
	if (!result_code)
	{
		const auto prepared = flatfile_player_domain_prepare_base_stat(
			root, lock, payload.pid, payload.stat_index, true, &stat, &result_code,
			&error);
		if (prepared != flatfile_player_domain_result::ok)
			return { prepared == flatfile_player_domain_result::io_error ?
					 critical_apply_outcome::retryable_failure :
					 critical_apply_outcome::terminal_failure,
				 catalog.revision,
				 static_cast<unsigned int>(
					 prepared == flatfile_player_domain_result::io_error ?
						 EIO :
						 EILSEQ) };
		mutation_applied = !result_code;
	}
	if (mutation_applied)
		--shop->stats;
	boon_shop_result result = { payload.pid, payload.stat_index, stat.stat_value,
				    mutation_applied ? shop->stats :
						       std::max<int64_t>(shop ? shop->stats : 0, 0),
				    stat.stat_revision };
	try
	{
		catalog.shop_operations.push_back(
			{ command.operation_id, digest, result_code, result });
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision, ENOMEM };
	}
	++catalog.revision;
	std::vector<uint8_t> bytes;
	if (!encode_catalog(catalog, &bytes))
		return { critical_apply_outcome::terminal_failure, catalog.revision - 1, ENOSPC };
	std::vector<flatfile_authority_after_image> images;
	try
	{
		images.push_back({ catalog_filename, std::move(bytes) });
		if (mutation_applied)
			images.push_back(std::move(stat.after_image));
	}
	catch (const std::bad_alloc &)
	{
		return { critical_apply_outcome::retryable_failure, catalog.revision - 1, ENOMEM };
	}
	const auto committed = flatfile_authority_transaction_commit(root, lock, images, &error);
	if (committed != flatfile_authority_transaction_result::ok)
		return { committed == flatfile_authority_transaction_result::io_error ?
				 critical_apply_outcome::retryable_failure :
				 critical_apply_outcome::terminal_failure,
			 catalog.revision - 1,
			 static_cast<unsigned int>(
				 committed == flatfile_authority_transaction_result::io_error ?
					 EIO :
					 EILSEQ) };
	return make_result(catalog.shop_operations.back(), catalog.revision,
			   critical_apply_outcome::applied);
}
