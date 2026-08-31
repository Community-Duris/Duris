#include "prototypes.h"
#include "structs.h"
#include "utility.h"
#include "utils.h"
#include "account/multiplay_whitelist.h"
#include <string.h>
#include "sql/sql.h"

#ifdef __NO_MYSQL__
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_mode.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <climits>
#include <ctime>
#include <new>
#include <openssl/crypto.h>
#include <openssl/sha.h>
#include <string>
#include <vector>

namespace
{
constexpr std::array<uint8_t, 8> whitelist_magic = { 'D', 'U', 'R', 'M', 'W', 'L', 'S', 'T' };
constexpr uint32_t whitelist_version = 1;
constexpr size_t whitelist_maximum_entries = 4096;
constexpr size_t whitelist_field_maximum = 255;
constexpr size_t whitelist_maximum_bytes = 6 * 1024 * 1024;
constexpr size_t whitelist_header_size =
	whitelist_magic.size() + sizeof(uint32_t) * 2 + SHA256_DIGEST_LENGTH;
constexpr const char *whitelist_filename = "multiplay_whitelist";
constexpr const char *whitelist_lock_filename = ".multiplay-whitelist.lock";

enum class flat_whitelist_load_result
{
	ok,
	not_found,
	failed
};

void append_u32(vector<uint8_t> *bytes, uint32_t value)
{
	for (size_t offset = 0; offset < sizeof(value); ++offset)
	{
		bytes->push_back(static_cast<uint8_t>(value & 0xff));
		value >>= 8;
	}
}

bool read_u32(const uint8_t *data, size_t size, size_t *offset, uint32_t *value)
{
	if (!data || !offset || !value || *offset > size || size - *offset < sizeof(*value))
		return false;
	uint32_t decoded = 0;
	for (size_t index = 0; index < sizeof(decoded); ++index)
		decoded |= static_cast<uint32_t>(data[(*offset)++]) << (index * 8);
	*value = decoded;
	return true;
}

bool valid_field(const string &value)
{
	if (value.empty() || value.size() > whitelist_field_maximum)
		return false;
	for (unsigned char character : value)
		if (character < 0x20 || character == 0x7f)
			return false;
	return true;
}

bool valid_date(const string &value)
{
	if (value.size() != 10 || value[4] != '-' || value[7] != '-')
		return false;
	for (size_t index = 0; index < value.size(); ++index)
		if (index != 4 && index != 7 && !isdigit(static_cast<unsigned char>(value[index])))
			return false;
	return true;
}

bool append_field(vector<uint8_t> *bytes, const string &value)
{
	if (!bytes || !valid_field(value))
		return false;
	append_u32(bytes, static_cast<uint32_t>(value.size()));
	bytes->insert(bytes->end(), value.begin(), value.end());
	return true;
}

bool read_field(const uint8_t *data, size_t size, size_t *offset, string *value)
{
	uint32_t length = 0;
	if (!value || !read_u32(data, size, offset, &length) || !length ||
	    length > whitelist_field_maximum || *offset > size || size - *offset < length)
		return false;
	try
	{
		value->assign(reinterpret_cast<const char *>(data + *offset), length);
	}
	catch (const bad_alloc &)
	{
		return false;
	}
	*offset += length;
	return valid_field(*value);
}

bool valid_whitelist(const vector<whitelist_data> &whitelist)
{
	if (whitelist.size() > whitelist_maximum_entries)
		return false;
	int previous_id = 0;
	for (const auto &entry : whitelist)
	{
		if (entry.id <= previous_id || !valid_date(entry.created_on) ||
		    !valid_field(entry.pattern) || !valid_field(entry.player) ||
		    !valid_field(entry.admin) || !valid_field(entry.description))
			return false;
		previous_id = entry.id;
	}
	return true;
}

bool encode_whitelist(const vector<whitelist_data> &whitelist, vector<uint8_t> *bytes)
{
	if (!bytes || !valid_whitelist(whitelist))
		return false;
	try
	{
		vector<uint8_t> payload;
		payload.reserve(sizeof(uint32_t) + whitelist.size() * 128);
		append_u32(&payload, static_cast<uint32_t>(whitelist.size()));
		for (const auto &entry : whitelist)
		{
			append_u32(&payload, static_cast<uint32_t>(entry.id));
			if (!append_field(&payload, entry.created_on) ||
			    !append_field(&payload, entry.pattern) ||
			    !append_field(&payload, entry.player) ||
			    !append_field(&payload, entry.admin) ||
			    !append_field(&payload, entry.description))
				return false;
		}
		if (payload.size() > UINT32_MAX ||
		    payload.size() > whitelist_maximum_bytes - whitelist_header_size)
			return false;
		std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
		SHA256(payload.data(), payload.size(), digest.data());
		bytes->clear();
		bytes->reserve(whitelist_header_size + payload.size());
		bytes->insert(bytes->end(), whitelist_magic.begin(), whitelist_magic.end());
		append_u32(bytes, whitelist_version);
		append_u32(bytes, static_cast<uint32_t>(payload.size()));
		bytes->insert(bytes->end(), digest.begin(), digest.end());
		bytes->insert(bytes->end(), payload.begin(), payload.end());
		return true;
	}
	catch (const bad_alloc &)
	{
		return false;
	}
}

bool decode_whitelist(const vector<uint8_t> &bytes, vector<whitelist_data> *whitelist)
{
	if (!whitelist || bytes.size() < whitelist_header_size ||
	    !equal(whitelist_magic.begin(), whitelist_magic.end(), bytes.begin()))
		return false;
	size_t header_offset = whitelist_magic.size();
	uint32_t version = 0, payload_size = 0;
	if (!read_u32(bytes.data(), bytes.size(), &header_offset, &version) ||
	    !read_u32(bytes.data(), bytes.size(), &header_offset, &payload_size) ||
	    version != whitelist_version || payload_size != bytes.size() - whitelist_header_size)
		return false;
	const uint8_t *expected_digest = bytes.data() + header_offset;
	header_offset += SHA256_DIGEST_LENGTH;
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(bytes.data() + header_offset, payload_size, digest.data());
	if (CRYPTO_memcmp(expected_digest, digest.data(), digest.size()))
		return false;

	const uint8_t *payload = bytes.data() + header_offset;
	size_t offset = 0;
	uint32_t count = 0;
	if (!read_u32(payload, payload_size, &offset, &count) || count > whitelist_maximum_entries)
		return false;
	vector<whitelist_data> decoded;
	try
	{
		decoded.reserve(count);
		for (uint32_t index = 0; index < count; ++index)
		{
			uint32_t id = 0;
			string created_on, pattern, player, admin, description;
			if (!read_u32(payload, payload_size, &offset, &id) || !id || id > INT_MAX ||
			    !read_field(payload, payload_size, &offset, &created_on) ||
			    !read_field(payload, payload_size, &offset, &pattern) ||
			    !read_field(payload, payload_size, &offset, &player) ||
			    !read_field(payload, payload_size, &offset, &admin) ||
			    !read_field(payload, payload_size, &offset, &description))
				return false;
			decoded.emplace_back(static_cast<int>(id), created_on, pattern, player,
					     admin, description);
		}
	}
	catch (const bad_alloc &)
	{
		return false;
	}
	if (offset != payload_size || !valid_whitelist(decoded))
		return false;
	*whitelist = std::move(decoded);
	return true;
}

string whitelist_directory()
{
	const char *root = persistence_mode_flatfile_root();
	return root && *root ? string(root) + "/metadata" : string();
}

flat_whitelist_load_result load_flat_whitelist(const string &directory,
					       vector<whitelist_data> *whitelist, string *error)
{
	if (directory.empty() || !whitelist)
	{
		if (error)
			*error = "flat-file state root is unavailable";
		return flat_whitelist_load_result::failed;
	}
	vector<uint8_t> bytes;
	const auto loaded = flatfile_read(directory, whitelist_filename, whitelist_maximum_bytes,
					  &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flat_whitelist_load_result::not_found;
	if (loaded != flatfile_read_result::ok || !decode_whitelist(bytes, whitelist))
	{
		if (error && error->empty())
			*error = "multiplay whitelist record is corrupt";
		return flat_whitelist_load_result::failed;
	}
	return flat_whitelist_load_result::ok;
}

bool save_flat_whitelist(const string &directory, const vector<whitelist_data> &whitelist,
			 string *error)
{
	vector<uint8_t> bytes;
	if (!encode_whitelist(whitelist, &bytes))
	{
		if (error)
			*error = "multiplay whitelist values exceed format limits";
		return false;
	}
	return flatfile_atomic_write(directory, whitelist_filename, bytes, error);
}

string trimmed(const char *value)
{
	if (!value)
		return {};
	const unsigned char *begin = reinterpret_cast<const unsigned char *>(value);
	while (*begin && isspace(*begin))
		++begin;
	const unsigned char *end = begin + strlen(reinterpret_cast<const char *>(begin));
	while (end > begin && isspace(end[-1]))
		--end;
	return string(reinterpret_cast<const char *>(begin), reinterpret_cast<const char *>(end));
}

bool current_date(string *value)
{
	if (!value)
		return false;
	const time_t now = time(nullptr);
	struct tm local = {};
	char date[11];
	if (now == static_cast<time_t>(-1) || !localtime_r(&now, &local) ||
	    strftime(date, sizeof(date), "%Y-%m-%d", &local) != 10)
		return false;
	*value = date;
	return true;
}
} // namespace
#endif

extern P_desc descriptor_list;

bool whitelisted_host(const char *host)
{
	vector<whitelist_data> whitelist = get_whitelist();

	for (vector<whitelist_data>::iterator it = whitelist.begin(); it != whitelist.end(); it++)
	{
		if (match_pattern(it->pattern.c_str(), host))
			return true;
	}

	return false;
}

vector<whitelist_data> get_whitelist()
{
	vector<whitelist_data> whitelist;

#ifdef __NO_MYSQL__
	string error;
	const auto loaded = load_flat_whitelist(whitelist_directory(), &whitelist, &error);
	if (loaded == flat_whitelist_load_result::failed)
		logit(LOG_DEBUG, "get_whitelist: %s", error.c_str());
	return whitelist;
#else
	if (!qry("SELECT id, created_on, pattern, player, admin, description FROM %s",
		 MULTIPLAY_WHITELIST_TABLE_NAME))
	{
		logit(LOG_DEBUG, "get_whitelist(): qry failed");
		return whitelist;
	}

	MYSQL_RES *res = mysql_store_result(DB);
	if (!res)
	{
		logit(LOG_DEBUG, "%s: mysql_store_result failed", __func__);
		return whitelist;
	}
	while (MYSQL_ROW row = mysql_fetch_row(res))
	{
		whitelist.push_back(whitelist_data(atoi(row[0]), string(row[1]), string(row[2]),
						   string(row[3]), string(row[4]), string(row[5])));
	}

	mysql_free_result(res);

	return whitelist;
#endif
}

bool add_to_whitelist(P_char ch, const char *player, const char *pattern, const char *description)
{
#ifdef __NO_MYSQL__
	if (!ch || !GET_NAME(ch))
		return false;
	const string directory = whitelist_directory();
	const string saved_player = trimmed(player);
	const string saved_pattern = trimmed(pattern);
	const string saved_description = trimmed(description);
	const string saved_admin = trimmed(GET_NAME(ch));
	if (!valid_field(saved_player) || !valid_field(saved_pattern) ||
	    !valid_field(saved_description) || !valid_field(saved_admin))
		return false;
	string error;
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, whitelist_lock_filename, &lock_fd, &error))
	{
		logit(LOG_DEBUG, "add_to_whitelist: %s", error.c_str());
		return false;
	}
	vector<whitelist_data> whitelist;
	const auto loaded = load_flat_whitelist(directory, &whitelist, &error);
	if (loaded == flat_whitelist_load_result::failed ||
	    whitelist.size() >= whitelist_maximum_entries ||
	    (!whitelist.empty() && whitelist.back().id == INT_MAX))
	{
		flatfile_lock_release(lock_fd);
		logit(LOG_DEBUG, "add_to_whitelist: %s",
		      error.empty() ? "multiplay whitelist limit reached" : error.c_str());
		return false;
	}
	string created_on;
	if (!current_date(&created_on))
	{
		flatfile_lock_release(lock_fd);
		return false;
	}
	try
	{
		const int id = whitelist.empty() ? 1 : whitelist.back().id + 1;
		whitelist.emplace_back(id, created_on, saved_pattern, saved_player, saved_admin,
				       saved_description);
	}
	catch (const bad_alloc &)
	{
		flatfile_lock_release(lock_fd);
		return false;
	}
	const bool saved = save_flat_whitelist(directory, whitelist, &error);
	flatfile_lock_release(lock_fd);
	if (!saved)
	{
		logit(LOG_DEBUG, "add_to_whitelist: %s", error.c_str());
		return false;
	}
	logit(WIZLOG, "Added '%s' (%s: %s) to multiplay whitelist", saved_pattern.c_str(),
	      saved_player.c_str(), saved_description.c_str());
	return true;
#else
	char descbuff[MAX_STRING_LENGTH];

	mysql_real_escape_string(DB, descbuff, description, strlen(description));

	if (!qry("INSERT INTO %s (created_on, admin, player, pattern, description) VALUES (now(), trim('%s'), trim('%s'), trim('%s'), trim('%s'))",
		 MULTIPLAY_WHITELIST_TABLE_NAME, GET_NAME(ch), player, pattern, descbuff))
	{
		logit(LOG_DEBUG, "add_to_whitelist(): qry failed");
		return false;
	}

	sql_log(ch, WIZLOG, "Added '%s' (%s: %s) to multiplay whitelist", pattern, player,
		description);
	return true;
#endif
}

bool remove_from_whitelist(P_char ch, const char *pattern)
{
#ifdef __NO_MYSQL__
	if (!ch || !GET_NAME(ch))
		return false;
	const string directory = whitelist_directory();
	const string saved_pattern = trimmed(pattern);
	if (!valid_field(saved_pattern))
		return false;
	string error;
	int lock_fd = -1;
	if (!flatfile_lock_acquire(directory, whitelist_lock_filename, &lock_fd, &error))
	{
		logit(LOG_DEBUG, "remove_from_whitelist: %s", error.c_str());
		return false;
	}
	vector<whitelist_data> whitelist;
	const auto loaded = load_flat_whitelist(directory, &whitelist, &error);
	if (loaded == flat_whitelist_load_result::failed)
	{
		flatfile_lock_release(lock_fd);
		logit(LOG_DEBUG, "remove_from_whitelist: %s", error.c_str());
		return false;
	}
	bool saved = true;
	if (loaded == flat_whitelist_load_result::ok)
	{
		const auto first_removed = remove_if(whitelist.begin(), whitelist.end(),
						     [&](const auto &entry)
						     { return entry.pattern == saved_pattern; });
		if (first_removed != whitelist.end())
		{
			whitelist.erase(first_removed, whitelist.end());
			saved = save_flat_whitelist(directory, whitelist, &error);
		}
	}
	flatfile_lock_release(lock_fd);
	if (!saved)
	{
		logit(LOG_DEBUG, "remove_from_whitelist: %s", error.c_str());
		return false;
	}
	logit(WIZLOG, "Removed '%s' from multiplay whitelist", saved_pattern.c_str());
	return true;
#else
	if (!qry("DELETE FROM %s WHERE pattern = trim('%s')", MULTIPLAY_WHITELIST_TABLE_NAME,
		 pattern))
	{
		logit(LOG_DEBUG, "remove_from_whitelist(): qry failed");
		return false;
	}

	sql_log(ch, WIZLOG, "Removed '%s' from multiplay whitelist", pattern);
	return true;
#endif
}

void do_whitelist_help(P_char ch)
{
	send_to_char("usage: whitelist\r\n"
		     "       whitelist add <player> <*.*.*.* ip address pattern> <description>\r\n"
		     "       whitelist remove <ip address pattern>\r\n\r\n",
		     ch);
}

bool is_connected(const char *pattern)
{
	for (P_desc k = descriptor_list; k; k = k->next)
	{
		if (match_pattern(pattern, k->host))
		{
			return true;
		}
	}
	return false;
}

void do_whitelist(P_char ch, char *argument, int /*cmd*/)
{
	char argbuf[MAX_STRING_LENGTH], linebuf[MAX_STRING_LENGTH];

	if (IS_NPC(ch) || !IS_TRUSTED(ch))
		return;

	argument = one_argument(argument, argbuf);

	if (!*argbuf)
	{
		// no arguments: list existing whitelist
		vector<whitelist_data> whitelist = get_whitelist();

		send_to_char(
			" Host pattern    | Added by     | On         | Player       | Description\r\n"
			"-----------------------------------------------------------------------------\r\n",
			ch);
		//               " 127.345.234.113 | Torgal       | 2009-12-22 | Zion         | Contacted by email, brothers who play from same net"

		for (vector<whitelist_data>::iterator it = whitelist.begin(); it != whitelist.end();
		     it++)
		{
			snprintf(linebuf, MAX_STRING_LENGTH, " %s%s | %s | %s | %s | %s&n\r\n",
				 (is_connected(it->pattern.c_str()) ? string("&+R").c_str() : ""),
				 pad_ansi(it->pattern.c_str(), 15).c_str(),
				 pad_ansi(it->admin.c_str(), 12).c_str(),
				 pad_ansi(it->created_on.c_str(), 10).c_str(),
				 pad_ansi(it->player.c_str(), 12).c_str(), it->description.c_str());

			send_to_char(linebuf, ch);
		}

		return;
	}
	else if (!strcmp(argbuf, "add"))
	{
		// argument should be: <player> <ip pattern> <description>
		char player[MAX_STRING_LENGTH];
		char pattern[MAX_STRING_LENGTH];

		argument = one_argument(argument, player);

		if (!(*player))
		{
			send_to_char("Missing player name\r\n", ch);
			do_whitelist_help(ch);
			return;
		}

		argument = one_argument(argument, pattern);

		if (!(*pattern) || !match_pattern("*.*.*.*", pattern))
		{
			send_to_char("Invalid host pattern.\r\n", ch);
			do_whitelist_help(ch);
			return;
		}

		if (!(*argument))
		{
			send_to_char("Missing description.\r\n", ch);
			do_whitelist_help(ch);
			return;
		}

		if (add_to_whitelist(ch, player, pattern, argument))
		{
			send_to_char("Host pattern added to multiplay whitelist.\r\n", ch);
		}
		else
		{
			send_to_char(
				"ERROR: host pattern could not be added to multiplay whitelist.\r\n",
				ch);
		}

		return;
	}
	else if (!strcmp(argbuf, "remove"))
	{
		// argument should be: <*.*.*.* pattern>

		if (!match_pattern("*.*.*.*", argument))
		{
			send_to_char("Invalid host pattern.\r\n", ch);
			do_whitelist_help(ch);
			return;
		}

		if (remove_from_whitelist(ch, argument))
		{
			send_to_char("Host pattern removed from multiplay whitelist.\r\n", ch);
		}
		else
		{
			send_to_char(
				"Host pattern not found or could not be removed from multiplay whitelist.\r\n",
				ch);
		}

		return;
	}
	else
	{
		do_whitelist_help(ch);
		return;
	}
}
