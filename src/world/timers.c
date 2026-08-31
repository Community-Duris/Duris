#include "world/timers.h"
#include "guild/assocs.h"
#include "world/epic.h"
#include "ships/ships.h"
#include "sql/sql.h"
#include "core/utility.h"
#include <stdlib.h>

#ifdef __NO_MYSQL__
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_mode.h"

#include <algorithm>
#include <array>
#include <bit>
#include <cstdint>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> timer_magic = { 'D', 'U', 'R', 'T', 'I', 'M', 'R', 0 };
constexpr uint32_t timer_version = 1;
constexpr size_t timer_prefix_size = timer_magic.size() + sizeof(uint32_t) + sizeof(uint32_t);
constexpr size_t timer_record_size = timer_prefix_size + SHA256_DIGEST_LENGTH;
constexpr size_t timer_name_maximum = 255;

bool valid_timer_name(const char *name)
{
	if (!name || !*name)
		return false;
	size_t length = 0;
	for (const unsigned char *cursor = reinterpret_cast<const unsigned char *>(name); *cursor;
	     ++cursor)
	{
		const bool alphanumeric = (*cursor >= 'a' && *cursor <= 'z') ||
					  (*cursor >= 'A' && *cursor <= 'Z') ||
					  (*cursor >= '0' && *cursor <= '9');
		if (++length > timer_name_maximum ||
		    !(alphanumeric || *cursor == '_' || *cursor == '-'))
			return false;
	}
	return true;
}

void append_u32(std::vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t offset = 0; offset < sizeof(value); ++offset)
	{
		bytes->push_back(static_cast<uint8_t>(value & 0xff));
		value >>= 8;
	}
}

uint32_t read_u32(const uint8_t *bytes)
{
	uint32_t value = 0;
	for (size_t offset = 0; offset < sizeof(value); ++offset)
		value |= static_cast<uint32_t>(bytes[offset]) << (offset * 8);
	return value;
}

std::vector<uint8_t> encode_timer(int date)
{
	std::vector<uint8_t> bytes;
	bytes.reserve(timer_record_size);
	bytes.insert(bytes.end(), timer_magic.begin(), timer_magic.end());
	append_u32(&bytes, timer_version);
	append_u32(&bytes, static_cast<uint32_t>(date));
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), bytes.size(), digest.data());
	bytes.insert(bytes.end(), digest.begin(), digest.end());
	return bytes;
}

bool decode_timer(const std::vector<uint8_t> &bytes, int *date)
{
	static_assert(sizeof(int) == sizeof(uint32_t));
	if (!date || bytes.size() != timer_record_size ||
	    !std::equal(timer_magic.begin(), timer_magic.end(), bytes.begin()) ||
	    read_u32(bytes.data() + timer_magic.size()) != timer_version)
		return false;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data(), timer_prefix_size, digest.data());
	if (CRYPTO_memcmp(bytes.data() + timer_prefix_size, digest.data(), digest.size()))
		return false;
	const uint32_t encoded_date =
		read_u32(bytes.data() + timer_magic.size() + sizeof(uint32_t));
	*date = static_cast<int>(std::bit_cast<int32_t>(encoded_date));
	return true;
}

bool timer_location(const char *name, std::string *directory, std::string *filename)
{
	const char *root = persistence_mode_flatfile_root();
	if (!root || !*root || !directory || !filename || !valid_timer_name(name))
		return false;
	*directory = std::string(root) + "/metadata";
	*filename = std::string("timer.") + name;
	return true;
}
} // namespace

void set_timer(const char *name)
{
	set_timer(name, static_cast<int>(time(NULL)));
}

void set_timer(const char *name, int date)
{
	std::string directory, filename, error;
	if (!timer_location(name, &directory, &filename))
	{
		logit(LOG_DEBUG, "set_timer: invalid flat-file timer name or state root");
		return;
	}
	if (!flatfile_atomic_write(directory, filename, encode_timer(date), &error))
		logit(LOG_DEBUG, "set_timer: failed to save timer %s: %s", name, error.c_str());
}

int get_timer(const char *name)
{
	std::string directory, filename, error;
	if (!timer_location(name, &directory, &filename))
	{
		logit(LOG_DEBUG, "get_timer: invalid flat-file timer name or state root");
		return 0;
	}
	std::vector<uint8_t> bytes;
	const flatfile_read_result loaded =
		flatfile_read(directory, filename, timer_record_size, &bytes, &error);
	if (loaded == flatfile_read_result::not_found)
		return 0;
	int date = 0;
	if (loaded != flatfile_read_result::ok || !decode_timer(bytes, &date))
	{
		logit(LOG_DEBUG, "get_timer: failed to load timer %s: %s", name,
		      error.empty() ? "invalid timer record" : error.c_str());
		return 0;
	}
	return date;
}
#else
void set_timer(const char *name)
{
	set_timer(name, time(NULL));
}

void set_timer(const char *name, int date)
{
	if (!qry("REPLACE INTO timers (name, date) VALUES ('%s', '%d')", name, date))
		logit(LOG_DEBUG, "set_timer: failed to save timer %s", name ? name : "<null>");
}

int get_timer(const char *name)
{
	if (!qry("SELECT date FROM timers WHERE name = '%s'", name))
	{
		return 0;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return FALSE;
	}

	if (mysql_num_rows(res) < 1)
	{
		mysql_free_result(res);
		return 0;
	}

	MYSQL_ROW row = mysql_fetch_row(res);

	int date = atoi(row[0]);
	mysql_free_result(res);

	return date;
}
#endif

bool has_elapsed(const char *name, int seconds)
{
	int timer = get_timer(name);

	if (time(NULL) > (timer + seconds))
	{
		return TRUE;
	}
	else
	{
		return FALSE;
	}
}

void timers_activity()
{
	//  prestige_update();
	cargo_activity();
}
