/*
 * poll.c - poll system for durismud
 *
 * allows immortals (level 57+) to create polls
 * mortals (level 30+) can vote and view
 * votes tracked per account (not per character)
 */

#include "prototypes.h"
#include "structs.h"
#include "net/comm.h"
#include "cmd/interp.h"
#include "utils.h"
#include "net/poll.h"
#include <ctype.h>
#include <map>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include "account/account.h"
#include "config.h"
#include "json_utils.h"
#include "sql/sql.h"
#include "net/websocket.h"

#ifdef __NO_MYSQL__
#include "flatfile/flatfile_store.h"
#include "persistence/persistence_mode.h"

#include <algorithm>
#include <climits>
#include <iomanip>
#include <limits>
#include <openssl/sha.h>
#include <sstream>
#include <strings.h>
#include <unordered_map>
#include <unordered_set>
#endif

using namespace std;

/* in-memory wizard sessions */
static map<P_char, poll_wizard_data> poll_wizards;

/* external declarations */
#ifndef __NO_MYSQL__
extern MYSQL *DB;
#endif

/* forward declarations */
static void poll_handle_vote(P_char ch, char *argument);
static void poll_wizard_show_summary(P_char ch, poll_wizard_data *wiz);
static void poll_display_bar(char *buf, int count, int max_count, int bar_width);
static void poll_send_wrapped(P_char ch, const char *text, int width, const char *prefix,
			      const char *suffix);

#ifdef __NO_MYSQL__
namespace
{
constexpr const char *flat_poll_filename = "polls";
constexpr const char *flat_poll_lock_filename = "polls.lock";
constexpr const char *flat_poll_magic = "DURIS-POLLS";
constexpr unsigned int flat_poll_version = 1;
constexpr size_t flat_poll_maximum_size = 64 * 1024 * 1024;
constexpr size_t flat_poll_maximum_count = 10000;
constexpr size_t flat_poll_maximum_votes = 200000;
constexpr size_t flat_poll_creator_maximum = 32;
constexpr size_t flat_poll_account_maximum = 64;
constexpr size_t flat_poll_character_maximum = 32;

struct flat_poll_vote
{
	int poll_id = 0;
	int option_id = 0;
	time_t voted_at = 0;
	string account_name;
	string character_name;
};

struct flat_poll_catalog
{
	uint64_t revision = 0;
	uint32_t next_poll_id = 1;
	uint32_t next_option_id = 1;
	vector<poll_data> polls;
	vector<flat_poll_vote> votes;
};

enum class flat_poll_load_result
{
	ok,
	missing,
	invalid,
	io_error
};

enum class flat_poll_close_result
{
	ok,
	not_found,
	already_closed,
	error
};

class flat_poll_lock
{
    public:
	~flat_poll_lock() { flatfile_lock_release(fd); }

	int *destination() { return &fd; }

    private:
	int fd = -1;
};

string flat_poll_directory()
{
	const char *root = persistence_mode_flatfile_root();
	return root && *root ? string(root) + "/metadata" : string();
}

bool valid_flat_poll_string(const string &value, size_t maximum, bool allow_empty = false)
{
	return (allow_empty || !value.empty()) && value.size() <= maximum &&
	       value.find('\0') == string::npos;
}

string flat_poll_account_key(const string &account)
{
	string key = account;
	transform(key.begin(), key.end(), key.begin(),
		  [](unsigned char value) { return static_cast<char>(tolower(value)); });
	return key;
}

bool valid_flat_poll_definition(const poll_data &poll)
{
	if (!valid_flat_poll_string(poll.question, MAX_POLL_QUESTION) ||
	    !valid_flat_poll_string(poll.created_by, flat_poll_creator_maximum) ||
	    poll.created_at < 0 || poll.expires_at <= poll.created_at || poll.options.size() < 2 ||
	    poll.options.size() > MAX_POLL_OPTIONS)
		return false;
	if ((!poll.multi_select && poll.max_choices != 1) ||
	    (poll.multi_select &&
	     (poll.max_choices < 2 || static_cast<size_t>(poll.max_choices) > poll.options.size())))
		return false;

	unordered_set<int> option_numbers;
	for (const poll_option &option : poll.options)
	{
		if (option.option_num < 1 || option.option_num > MAX_POLL_OPTIONS ||
		    !option_numbers.insert(option.option_num).second ||
		    !valid_flat_poll_string(option.text, MAX_OPTION_TEXT))
			return false;
	}
	return true;
}

bool validate_flat_poll_catalog(const flat_poll_catalog &catalog, string *error)
{
	if (!catalog.next_poll_id || !catalog.next_option_id ||
	    catalog.polls.size() > flat_poll_maximum_count ||
	    catalog.votes.size() > flat_poll_maximum_votes)
	{
		if (error)
			*error = "invalid poll catalog header";
		return false;
	}

	uint32_t maximum_poll_id = 0;
	uint32_t maximum_option_id = 0;
	unordered_set<int> poll_ids;
	unordered_map<int, int> option_owners;
	for (const poll_data &poll : catalog.polls)
	{
		if (poll.id < 1 || !valid_flat_poll_definition(poll) ||
		    !poll_ids.insert(poll.id).second)
		{
			if (error)
				*error = "invalid poll catalog entry";
			return false;
		}
		maximum_poll_id = max(maximum_poll_id, static_cast<uint32_t>(poll.id));
		for (const poll_option &option : poll.options)
		{
			if (option.id < 1 || !option_owners.emplace(option.id, poll.id).second)
			{
				if (error)
					*error = "invalid poll option identity";
				return false;
			}
			maximum_option_id =
				max(maximum_option_id, static_cast<uint32_t>(option.id));
		}
	}
	if (catalog.next_poll_id <= maximum_poll_id || catalog.next_option_id <= maximum_option_id)
	{
		if (error)
			*error = "invalid poll identity allocator";
		return false;
	}

	unordered_set<string> vote_keys;
	for (const flat_poll_vote &vote : catalog.votes)
	{
		const auto owner = option_owners.find(vote.option_id);
		if (poll_ids.find(vote.poll_id) == poll_ids.end() || owner == option_owners.end() ||
		    owner->second != vote.poll_id ||
		    !valid_flat_poll_string(vote.account_name, flat_poll_account_maximum) ||
		    !valid_flat_poll_string(vote.character_name, flat_poll_character_maximum))
		{
			if (error)
				*error = "invalid poll vote entry";
			return false;
		}
		const string key = to_string(vote.poll_id) + ":" +
				   flat_poll_account_key(vote.account_name) + ":" +
				   to_string(vote.option_id);
		if (!vote_keys.insert(key).second)
		{
			if (error)
				*error = "duplicate poll vote entry";
			return false;
		}
	}
	return true;
}

string flat_poll_digest(const string &body)
{
	unsigned char digest[SHA256_DIGEST_LENGTH] = {};
	SHA256(reinterpret_cast<const unsigned char *>(body.data()), body.size(), digest);
	ostringstream encoded;
	encoded << hex << setfill('0');
	for (unsigned char byte : digest)
		encoded << setw(2) << static_cast<unsigned int>(byte);
	return encoded.str();
}

bool encode_flat_poll_catalog(const flat_poll_catalog &catalog, vector<uint8_t> *bytes,
			      string *error)
{
	if (!bytes || !validate_flat_poll_catalog(catalog, error))
		return false;

	ostringstream output;
	output << flat_poll_magic << ' ' << flat_poll_version << '\n';
	output << catalog.revision << ' ' << catalog.next_poll_id << ' ' << catalog.next_option_id
	       << '\n';
	output << catalog.polls.size() << ' ' << catalog.votes.size() << '\n';
	for (const poll_data &poll : catalog.polls)
	{
		output << "P " << poll.id << ' ' << static_cast<long long>(poll.created_at) << ' '
		       << static_cast<long long>(poll.expires_at) << ' ' << (poll.is_active ? 1 : 0)
		       << ' ' << (poll.multi_select ? 1 : 0) << ' ' << poll.max_choices << ' '
		       << quoted(poll.question) << ' ' << quoted(poll.created_by) << ' '
		       << poll.options.size() << '\n';
		for (const poll_option &option : poll.options)
			output << "O " << option.id << ' ' << option.option_num << ' '
			       << quoted(option.text) << '\n';
	}
	for (const flat_poll_vote &vote : catalog.votes)
		output << "V " << vote.poll_id << ' ' << vote.option_id << ' '
		       << static_cast<long long>(vote.voted_at) << ' ' << quoted(vote.account_name)
		       << ' ' << quoted(vote.character_name) << '\n';
	if (!output)
	{
		if (error)
			*error = "could not encode poll catalog";
		return false;
	}
	const string body = output.str();
	const string record = body + "S " + flat_poll_digest(body) + "\n";
	if (record.size() > flat_poll_maximum_size)
	{
		if (error)
			*error = "poll catalog exceeds storage limit";
		return false;
	}
	bytes->assign(record.begin(), record.end());
	return true;
}

bool decode_flat_poll_catalog(const vector<uint8_t> &bytes, flat_poll_catalog *catalog,
			      string *error)
{
	if (!catalog || bytes.empty())
	{
		if (error)
			*error = "empty poll catalog";
		return false;
	}
	const string record(bytes.begin(), bytes.end());
	const size_t checksum_marker = record.rfind("\nS ");
	if (checksum_marker == string::npos || record.back() != '\n')
	{
		if (error)
			*error = "poll catalog checksum is missing";
		return false;
	}
	const string body = record.substr(0, checksum_marker + 1);
	const string checksum =
		record.substr(checksum_marker + 3, record.size() - checksum_marker - 4);
	if (checksum.size() != SHA256_DIGEST_LENGTH * 2 || checksum != flat_poll_digest(body))
	{
		if (error)
			*error = "poll catalog checksum mismatch";
		return false;
	}

	istringstream input(body);
	string magic;
	unsigned int version = 0;
	uint64_t poll_count = 0;
	uint64_t vote_count = 0;
	flat_poll_catalog decoded;
	if (!(input >> magic >> version >> decoded.revision >> decoded.next_poll_id >>
	      decoded.next_option_id >> poll_count >> vote_count) ||
	    magic != flat_poll_magic || version != flat_poll_version ||
	    poll_count > flat_poll_maximum_count || vote_count > flat_poll_maximum_votes)
	{
		if (error)
			*error = "invalid poll catalog header";
		return false;
	}

	decoded.polls.reserve(static_cast<size_t>(poll_count));
	for (uint64_t index = 0; index < poll_count; ++index)
	{
		char marker = 0;
		long long created_at = 0;
		long long expires_at = 0;
		unsigned int active = 0;
		unsigned int multi = 0;
		uint64_t option_count = 0;
		poll_data poll = {};
		if (!(input >> marker >> poll.id >> created_at >> expires_at >> active >> multi >>
		      poll.max_choices >> quoted(poll.question) >> quoted(poll.created_by) >>
		      option_count) ||
		    marker != 'P' || active > 1 || multi > 1 || option_count > MAX_POLL_OPTIONS)
		{
			if (error)
				*error = "invalid poll catalog entry";
			return false;
		}
		poll.created_at = static_cast<time_t>(created_at);
		poll.expires_at = static_cast<time_t>(expires_at);
		if (static_cast<long long>(poll.created_at) != created_at ||
		    static_cast<long long>(poll.expires_at) != expires_at)
		{
			if (error)
				*error = "poll timestamp is out of range";
			return false;
		}
		poll.is_active = active != 0;
		poll.multi_select = multi != 0;
		poll.total_votes = 0;
		poll.options.reserve(static_cast<size_t>(option_count));
		for (uint64_t option_index = 0; option_index < option_count; ++option_index)
		{
			poll_option option = {};
			if (!(input >> marker >> option.id >> option.option_num >>
			      quoted(option.text)) ||
			    marker != 'O')
			{
				if (error)
					*error = "invalid poll option entry";
				return false;
			}
			option.vote_count = 0;
			poll.options.push_back(option);
		}
		decoded.polls.push_back(poll);
	}

	decoded.votes.reserve(static_cast<size_t>(vote_count));
	for (uint64_t index = 0; index < vote_count; ++index)
	{
		char marker = 0;
		long long voted_at = 0;
		flat_poll_vote vote;
		if (!(input >> marker >> vote.poll_id >> vote.option_id >> voted_at >>
		      quoted(vote.account_name) >> quoted(vote.character_name)) ||
		    marker != 'V')
		{
			if (error)
				*error = "invalid poll vote entry";
			return false;
		}
		vote.voted_at = static_cast<time_t>(voted_at);
		if (static_cast<long long>(vote.voted_at) != voted_at)
		{
			if (error)
				*error = "poll vote timestamp is out of range";
			return false;
		}
		decoded.votes.push_back(vote);
	}
	input >> ws;
	if (!input.eof() || !validate_flat_poll_catalog(decoded, error))
	{
		if (error && error->empty())
			*error = "poll catalog has trailing data";
		return false;
	}
	*catalog = std::move(decoded);
	return true;
}

flat_poll_load_result load_flat_poll_catalog(flat_poll_catalog *catalog, string *error)
{
	if (!catalog)
		return flat_poll_load_result::invalid;
	*catalog = {};
	const string directory = flat_poll_directory();
	if (directory.empty())
	{
		if (error)
			*error = "flat-file state root is unavailable";
		return flat_poll_load_result::io_error;
	}
	vector<uint8_t> bytes;
	const flatfile_read_result loaded =
		flatfile_read(directory, flat_poll_filename, flat_poll_maximum_size, &bytes, error);
	if (loaded == flatfile_read_result::not_found)
		return flat_poll_load_result::missing;
	if (loaded == flatfile_read_result::io_error)
		return flat_poll_load_result::io_error;
	if (loaded != flatfile_read_result::ok || !decode_flat_poll_catalog(bytes, catalog, error))
		return flat_poll_load_result::invalid;
	return flat_poll_load_result::ok;
}

bool save_flat_poll_catalog(const flat_poll_catalog &catalog, string *error)
{
	vector<uint8_t> bytes;
	const string directory = flat_poll_directory();
	return !directory.empty() && encode_flat_poll_catalog(catalog, &bytes, error) &&
	       flatfile_atomic_write(directory, flat_poll_filename, bytes, error);
}

bool lock_flat_poll_catalog(flat_poll_lock *lock, string *error)
{
	const string directory = flat_poll_directory();
	if (!lock || directory.empty())
	{
		if (error)
			*error = "flat-file state root is unavailable";
		return false;
	}
	return flatfile_lock_acquire(directory, flat_poll_lock_filename, lock->destination(),
				     error);
}

void log_flat_poll_error(const char *operation, const string &error)
{
	logit(LOG_FILE, "poll %s failed: %s", operation,
	      error.empty() ? "invalid state" : error.c_str());
}

void apply_flat_poll_vote_counts(const flat_poll_catalog &catalog, vector<poll_data> *polls)
{
	if (!polls)
		return;
	unordered_map<int, size_t> poll_positions;
	unordered_map<int, pair<size_t, size_t>> option_positions;
	vector<unordered_set<string>> voters(polls->size());
	for (size_t poll_index = 0; poll_index < polls->size(); ++poll_index)
	{
		poll_data &poll = (*polls)[poll_index];
		poll.total_votes = 0;
		poll_positions.emplace(poll.id, poll_index);
		for (size_t option_index = 0; option_index < poll.options.size(); ++option_index)
		{
			poll.options[option_index].vote_count = 0;
			option_positions.emplace(poll.options[option_index].id,
						 pair<size_t, size_t>{ poll_index, option_index });
		}
	}
	for (const flat_poll_vote &vote : catalog.votes)
	{
		const auto poll_position = poll_positions.find(vote.poll_id);
		const auto option_position = option_positions.find(vote.option_id);
		if (poll_position == poll_positions.end() ||
		    option_position == option_positions.end())
			continue;
		const size_t poll_index = poll_position->second;
		if (voters[poll_index].insert(flat_poll_account_key(vote.account_name)).second)
			++(*polls)[poll_index].total_votes;
		++(*polls)[option_position->second.first]
			  .options[option_position->second.second]
			  .vote_count;
	}
}

vector<poll_data> get_flat_polls(bool active_only, bool *loaded_ok)
{
	flat_poll_catalog catalog;
	string error;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded_ok)
		*loaded_ok = loaded == flat_poll_load_result::ok ||
			     loaded == flat_poll_load_result::missing;
	if (loaded != flat_poll_load_result::ok && loaded != flat_poll_load_result::missing)
	{
		log_flat_poll_error("read", error);
		return {};
	}
	vector<poll_data> polls;
	const time_t now = time(NULL);
	for (const poll_data &poll : catalog.polls)
		if (!active_only || (poll.is_active && poll.expires_at > now))
			polls.push_back(poll);
	apply_flat_poll_vote_counts(catalog, &polls);
	sort(polls.begin(), polls.end(),
	     [](const poll_data &left, const poll_data &right) { return left.id > right.id; });
	return polls;
}

poll_data get_flat_poll_by_id(int poll_id, bool *loaded_ok)
{
	flat_poll_catalog catalog;
	string error;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded_ok)
		*loaded_ok = loaded == flat_poll_load_result::ok ||
			     loaded == flat_poll_load_result::missing;
	if (loaded != flat_poll_load_result::ok && loaded != flat_poll_load_result::missing)
	{
		log_flat_poll_error("read", error);
		return {};
	}
	for (const poll_data &stored : catalog.polls)
	{
		if (stored.id != poll_id)
			continue;
		vector<poll_data> one{ stored };
		apply_flat_poll_vote_counts(catalog, &one);
		return one.front();
	}
	return {};
}

bool create_flat_poll(poll_data *poll)
{
	if (!poll || !valid_flat_poll_definition(*poll))
		return false;
	flat_poll_lock lock;
	string error;
	if (!lock_flat_poll_catalog(&lock, &error))
	{
		log_flat_poll_error("create lock", error);
		return false;
	}
	flat_poll_catalog catalog;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded != flat_poll_load_result::ok && loaded != flat_poll_load_result::missing)
	{
		log_flat_poll_error("create read", error);
		return false;
	}
	if (catalog.polls.size() >= flat_poll_maximum_count ||
	    catalog.next_poll_id > static_cast<uint32_t>(INT_MAX) ||
	    poll->options.size() > UINT32_MAX - catalog.next_option_id + 1 ||
	    catalog.next_option_id > static_cast<uint32_t>(INT_MAX) - poll->options.size() + 1)
	{
		log_flat_poll_error("create", "poll identity or count limit reached");
		return false;
	}

	poll_data created = *poll;
	created.id = static_cast<int>(catalog.next_poll_id++);
	created.total_votes = 0;
	for (poll_option &option : created.options)
	{
		option.id = static_cast<int>(catalog.next_option_id++);
		option.vote_count = 0;
	}
	catalog.polls.push_back(created);
	++catalog.revision;
	if (!save_flat_poll_catalog(catalog, &error))
	{
		log_flat_poll_error("create write", error);
		return false;
	}
	*poll = created;
	return true;
}

flat_poll_close_result close_flat_poll(int poll_id)
{
	flat_poll_lock lock;
	string error;
	if (!lock_flat_poll_catalog(&lock, &error))
	{
		log_flat_poll_error("close lock", error);
		return flat_poll_close_result::error;
	}
	flat_poll_catalog catalog;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded != flat_poll_load_result::ok && loaded != flat_poll_load_result::missing)
	{
		log_flat_poll_error("close read", error);
		return flat_poll_close_result::error;
	}
	for (poll_data &poll : catalog.polls)
	{
		if (poll.id != poll_id)
			continue;
		if (!poll.is_active)
			return flat_poll_close_result::already_closed;
		poll.is_active = false;
		++catalog.revision;
		if (!save_flat_poll_catalog(catalog, &error))
		{
			log_flat_poll_error("close write", error);
			return flat_poll_close_result::error;
		}
		return flat_poll_close_result::ok;
	}
	return flat_poll_close_result::not_found;
}

int record_flat_poll_votes(const char *account_name, const char *character_name, int poll_id,
			   const vector<int> &choices)
{
	if (!account_name || !character_name || poll_id < 1 || choices.empty() ||
	    !valid_flat_poll_string(account_name, flat_poll_account_maximum) ||
	    !valid_flat_poll_string(character_name, flat_poll_character_maximum))
		return 0;
	flat_poll_lock lock;
	string error;
	if (!lock_flat_poll_catalog(&lock, &error))
	{
		log_flat_poll_error("vote lock", error);
		return 0;
	}
	flat_poll_catalog catalog;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded != flat_poll_load_result::ok)
	{
		if (loaded != flat_poll_load_result::missing)
			log_flat_poll_error("vote read", error);
		return 0;
	}
	const auto poll_position = find_if(catalog.polls.begin(), catalog.polls.end(),
					   [poll_id](const poll_data &poll)
					   { return poll.id == poll_id; });
	if (poll_position == catalog.polls.end())
		return 0;

	int votes_cast = 0;
	const string account_key = flat_poll_account_key(account_name);
	for (int choice : choices)
	{
		const auto option = find_if(poll_position->options.begin(),
					    poll_position->options.end(),
					    [choice](const poll_option &candidate)
					    { return candidate.option_num == choice; });
		if (option == poll_position->options.end())
			continue;
		const bool duplicate = any_of(
			catalog.votes.begin(), catalog.votes.end(),
			[&](const flat_poll_vote &vote)
			{
				return vote.poll_id == poll_id && vote.option_id == option->id &&
				       flat_poll_account_key(vote.account_name) == account_key;
			});
		if (duplicate || catalog.votes.size() >= flat_poll_maximum_votes)
			continue;
		catalog.votes.push_back(
			{ poll_id, option->id, time(NULL), account_name, character_name });
		++votes_cast;
	}
	if (!votes_cast)
		return 0;
	++catalog.revision;
	if (!save_flat_poll_catalog(catalog, &error))
	{
		log_flat_poll_error("vote write", error);
		return 0;
	}
	return votes_cast;
}

void expire_flat_polls()
{
	flat_poll_lock lock;
	string error;
	if (!lock_flat_poll_catalog(&lock, &error))
	{
		log_flat_poll_error("expiration lock", error);
		return;
	}
	flat_poll_catalog catalog;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded == flat_poll_load_result::missing)
		return;
	if (loaded != flat_poll_load_result::ok)
	{
		log_flat_poll_error("expiration read", error);
		return;
	}
	const time_t now = time(NULL);
	bool changed = false;
	for (poll_data &poll : catalog.polls)
	{
		if (poll.is_active && poll.expires_at < now)
		{
			poll.is_active = false;
			changed = true;
		}
	}
	if (!changed)
		return;
	++catalog.revision;
	if (!save_flat_poll_catalog(catalog, &error))
		log_flat_poll_error("expiration write", error);
}
} // namespace
#endif

/* send wrapped text */
static void poll_send_wrapped(P_char ch, const char *text, int width, const char *prefix,
			      const char *suffix)
{
	char line[MAX_STRING_LENGTH];
	char word[256];
	int line_len = 0;
	int word_len = 0;
	const char *p = text;

	line[0] = '\0';

	while (*p)
	{
		/* skip leading spaces */
		while (*p == ' ')
			p++;
		if (!*p)
			break;

		/* get next word */
		word_len = 0;
		while (*p && *p != ' ' && word_len < 255)
		{
			word[word_len++] = *p++;
		}
		word[word_len] = '\0';

		/* check if word fits on current line */
		if (line_len > 0 && line_len + 1 + word_len > width)
		{
			/* flush current line */
			char buf[MAX_STRING_LENGTH];
			checked_snprintf(buf, MAX_STRING_LENGTH, "%s%-*s%s\r\n", prefix, width,
					 line, suffix);
			send_to_char(buf, ch);
			line[0] = '\0';
			line_len = 0;
		}

		/* add word to line */
		if (line_len > 0)
		{
			strcat(line, " ");
			line_len++;
		}
		strcat(line, word);
		line_len += word_len;
	}

	/* flush remaining text */
	if (line_len > 0)
	{
		char buf[MAX_STRING_LENGTH];
		checked_snprintf(buf, MAX_STRING_LENGTH, "%s%-*s%s\r\n", prefix, width, line,
				 suffix);
		send_to_char(buf, ch);
	}
}

/* wrap text to lines */
static vector<string> poll_wrap_text(const char *text, int width)
{
	vector<string> lines;
	char line[MAX_STRING_LENGTH];
	char word[256];
	int line_len = 0;
	int word_len = 0;
	const char *p = text;

	line[0] = '\0';

	while (*p)
	{
		while (*p == ' ')
			p++;
		if (!*p)
			break;

		word_len = 0;
		while (*p && *p != ' ' && word_len < 255)
		{
			word[word_len++] = *p++;
		}
		word[word_len] = '\0';

		if (line_len > 0 && line_len + 1 + word_len > width)
		{
			lines.push_back(string(line));
			line[0] = '\0';
			line_len = 0;
		}

		if (line_len > 0)
		{
			strcat(line, " ");
			line_len++;
		}
		strcat(line, word);
		line_len += word_len;
	}

	if (line_len > 0)
	{
		lines.push_back(string(line));
	}

	return lines;
}

/* display option with bar */
static void poll_send_option(P_char ch, int opt_num, const char *text, const char *bar, int count)
{
	vector<string> lines = poll_wrap_text(text, 30);
	char buf[MAX_STRING_LENGTH];

	/* first line: number + text + bar + count */
	snprintf(buf, MAX_STRING_LENGTH, "&+c|  &+W%2d) &n%-30s &+c[&+g%s&+c] &+W%3d    &+c|\r\n",
		 opt_num, lines.empty() ? "" : lines[0].c_str(), bar, count);
	send_to_char(buf, ch);

	/* continuation lines: indented text */
	for (size_t i = 1; i < lines.size(); i++)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+c|      %-51s&+c|\r\n", lines[i].c_str());
		send_to_char(buf, ch);
	}
}

/* display option for results */
static void poll_send_option_results(P_char ch, int opt_num, const char *text, const char *bar,
				     int count, float pct)
{
	vector<string> lines = poll_wrap_text(text, 23);
	char buf[MAX_STRING_LENGTH];

	/* first line: number + text + count + pct + bar */
	snprintf(buf, MAX_STRING_LENGTH,
		 "&+c|  &+W%2d) &n%-23s &+W%3d &+c(&n%5.1f%%&+c) [&+g%s&+c]  &+c|\r\n", opt_num,
		 lines.empty() ? "" : lines[0].c_str(), count, pct, bar);
	send_to_char(buf, ch);

	/* continuation lines: indented text */
	for (size_t i = 1; i < lines.size(); i++)
	{
		snprintf(buf, MAX_STRING_LENGTH, "&+c|      %-51s&+c|\r\n", lines[i].c_str());
		send_to_char(buf, ch);
	}
}

/* format time remaining */
static void format_time_remaining(time_t expires_at, char *buf, size_t buflen)
{
	time_t remaining = expires_at - time(NULL);

	if (remaining <= 0)
	{
		snprintf(buf, buflen, "expired");
	}
	else if (remaining > 86400)
	{
		snprintf(buf, buflen, "%ldd", (long)(remaining / 86400));
	}
	else if (remaining > 3600)
	{
		snprintf(buf, buflen, "%ldh", (long)(remaining / 3600));
	}
	else if (remaining > 60)
	{
		snprintf(buf, buflen, "%ldm", (long)(remaining / 60));
	}
	else
	{
		snprintf(buf, buflen, "soon");
	}
}

/* check if account voted */
bool poll_has_voted(const char *account_name, int poll_id)
{
#ifdef __NO_MYSQL__
	if (!account_name || !*account_name || poll_id < 1)
		return false;
	flat_poll_catalog catalog;
	string error;
	const flat_poll_load_result loaded = load_flat_poll_catalog(&catalog, &error);
	if (loaded == flat_poll_load_result::missing)
		return false;
	if (loaded != flat_poll_load_result::ok)
	{
		log_flat_poll_error("voter read", error);
		return false;
	}
	const string account_key = flat_poll_account_key(account_name);
	for (const flat_poll_vote &vote : catalog.votes)
		if (vote.poll_id == poll_id &&
		    flat_poll_account_key(vote.account_name) == account_key)
			return true;
	return false;
#else
	if (!account_name || !*account_name)
		return false;

	string acct_esc = escape_str(account_name);
	MYSQL_RES *res = db_query(
		"SELECT id FROM poll_votes WHERE poll_id = %d AND account_name = '%s' LIMIT 1",
		poll_id, acct_esc.c_str());

	if (!res)
		return false;

	MYSQL_ROW row = mysql_fetch_row(res);
	bool voted = (row != NULL);
	mysql_free_result(res);
	return voted;
#endif
}

/* get all polls */
vector<poll_data> poll_get_all(bool active_only)
{
	vector<poll_data> polls;

#ifdef __NO_MYSQL__
	return get_flat_polls(active_only, nullptr);
#else
	MYSQL_RES *res;

	if (active_only)
	{
		res = db_query(
			"SELECT id, question, created_by, UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(expires_at), is_active, multi_select, max_choices "
			"FROM polls WHERE is_active = 1 AND expires_at > FROM_UNIXTIME(%ld) ORDER BY id DESC",
			(long)time(NULL));
	}
	else
	{
		res = db_query(
			"SELECT id, question, created_by, UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(expires_at), is_active, multi_select, max_choices "
			"FROM polls ORDER BY id DESC");
	}

	if (!res)
		return polls;

	MYSQL_ROW row;
	while ((row = mysql_fetch_row(res)))
	{
		poll_data poll;
		poll.id = atoi(row[0]);
		poll.question = row[1] ? row[1] : "";
		poll.created_by = row[2] ? row[2] : "";
		poll.created_at = atol(row[3]);
		poll.expires_at = atol(row[4]);
		poll.is_active = (atoi(row[5]) == 1);
		poll.multi_select = (atoi(row[6]) == 1);
		poll.max_choices = atoi(row[7]);
		poll.total_votes = 0;
		polls.push_back(poll);
	}
	mysql_free_result(res);

	/* vote counts */
	for (size_t i = 0; i < polls.size(); i++)
	{
		res = db_query(
			"SELECT COUNT(DISTINCT account_name) FROM poll_votes WHERE poll_id = %d",
			polls[i].id);
		if (res)
		{
			row = mysql_fetch_row(res);
			if (row && row[0])
			{
				polls[i].total_votes = atoi(row[0]);
			}
			mysql_free_result(res);
		}
	}
#endif

	return polls;
}

/* get poll by id */
poll_data poll_get_by_id(int poll_id)
{
	poll_data poll;
	poll.id = 0;

#ifdef __NO_MYSQL__
	return get_flat_poll_by_id(poll_id, nullptr);
#else
	MYSQL_RES *res = db_query(
		"SELECT id, question, created_by, UNIX_TIMESTAMP(created_at), UNIX_TIMESTAMP(expires_at), is_active, multi_select, max_choices "
		"FROM polls WHERE id = %d",
		poll_id);

	if (!res)
		return poll;

	MYSQL_ROW row = mysql_fetch_row(res);
	if (!row)
	{
		mysql_free_result(res);
		return poll;
	}

	poll.id = atoi(row[0]);
	poll.question = row[1] ? row[1] : "";
	poll.created_by = row[2] ? row[2] : "";
	poll.created_at = atol(row[3]);
	poll.expires_at = atol(row[4]);
	poll.is_active = (atoi(row[5]) == 1);
	poll.multi_select = (atoi(row[6]) == 1);
	poll.max_choices = atoi(row[7]);
	poll.total_votes = 0;
	mysql_free_result(res);

	/* options */
	res = db_query(
		"SELECT id, option_num, option_text FROM poll_options WHERE poll_id = %d ORDER BY option_num",
		poll_id);
	if (res)
	{
		while ((row = mysql_fetch_row(res)))
		{
			poll_option opt;
			opt.id = atoi(row[0]);
			opt.option_num = atoi(row[1]);
			opt.text = row[2] ? row[2] : "";
			opt.vote_count = 0;
			poll.options.push_back(opt);
		}
		mysql_free_result(res);
	}

	/* vote counts per option */
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		res = db_query("SELECT COUNT(*) FROM poll_votes WHERE option_id = %d",
			       poll.options[i].id);
		if (res)
		{
			row = mysql_fetch_row(res);
			if (row && row[0])
			{
				poll.options[i].vote_count = atoi(row[0]);
			}
			mysql_free_result(res);
		}
	}

	/* total voters */
	res = db_query("SELECT COUNT(DISTINCT account_name) FROM poll_votes WHERE poll_id = %d",
		       poll_id);
	if (res)
	{
		row = mysql_fetch_row(res);
		if (row && row[0])
		{
			poll.total_votes = atoi(row[0]);
		}
		mysql_free_result(res);
	}
#endif

	return poll;
}

/* create poll */
bool poll_create(poll_data *poll)
{
#ifdef __NO_MYSQL__
	return create_flat_poll(poll);
#else
	string question_esc = escape_str(poll->question.c_str());
	string creator_esc = escape_str(poll->created_by.c_str());

	if (!qry("INSERT INTO polls (question, created_by, created_at, expires_at, is_active, multi_select, max_choices) "
		 "VALUES ('%s', '%s', FROM_UNIXTIME(%ld), FROM_UNIXTIME(%ld), 1, %d, %d)",
		 question_esc.c_str(), creator_esc.c_str(), (long)poll->created_at,
		 (long)poll->expires_at, poll->multi_select ? 1 : 0, poll->max_choices))
	{
		return false;
	}

	int poll_id = (int)mysql_insert_id(DB);
	poll->id = poll_id;

	/* options */
	for (size_t i = 0; i < poll->options.size(); i++)
	{
		string opt_esc = escape_str(poll->options[i].text.c_str());
		qry("INSERT INTO poll_options (poll_id, option_num, option_text) VALUES (%d, %d, '%s')",
		    poll_id, poll->options[i].option_num, opt_esc.c_str());
	}

	return true;
#endif
}

/* close poll */
bool poll_close(int poll_id, P_char ch)
{
#ifdef __NO_MYSQL__
	poll_data poll = poll_get_by_id(poll_id);
	const flat_poll_close_result closed = close_flat_poll(poll_id);
	if (closed == flat_poll_close_result::not_found)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return false;
	}
	if (closed == flat_poll_close_result::already_closed)
	{
		send_to_char("That poll is already closed.\r\n", ch);
		return false;
	}
	if (closed != flat_poll_close_result::ok)
	{
		send_to_char("Failed to close poll.\r\n", ch);
		return false;
	}

	char buf[MAX_STRING_LENGTH];
	snprintf(buf, MAX_STRING_LENGTH, "&+W[POLL]&n Poll #%d has been closed by %s.\r\n", poll_id,
		 GET_NAME(ch));
	send_to_all(buf);
	poll_broadcast_close(poll_id, poll.question.c_str());
	return true;
#else
	poll_data poll = poll_get_by_id(poll_id);
	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return false;
	}

	if (!poll.is_active)
	{
		send_to_char("That poll is already closed.\r\n", ch);
		return false;
	}

	if (!qry("UPDATE polls SET is_active = 0 WHERE id = %d", poll_id))
	{
		send_to_char("Failed to close poll.\r\n", ch);
		return false;
	}

	char buf[MAX_STRING_LENGTH];
	snprintf(buf, MAX_STRING_LENGTH, "&+W[POLL]&n Poll #%d has been closed by %s.\r\n", poll_id,
		 GET_NAME(ch));
	send_to_all(buf);

	poll_broadcast_close(poll_id, poll.question.c_str());

	return true;
#endif
}

/* cast vote */
int poll_cast_vote(P_char ch, int poll_id, vector<int> &choices)
{
	const char *acct = get_account_name_safe(ch);
	if (!acct || !strcmp(acct, "Unknown"))
	{
		send_to_char("You must be logged in with an account to vote.\r\n", ch);
		return 0;
	}

	poll_data poll = poll_get_by_id(poll_id);
	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return 0;
	}

	int votes_cast = poll_record_votes(acct, GET_NAME(ch), poll_id, poll, choices);

	/* broadcast */
	if (votes_cast > 0)
	{
		poll = poll_get_by_id(poll_id);
		poll_broadcast_vote(poll_id, poll.total_votes);
	}

	return votes_cast;
}

/* close expired polls */
void poll_check_expirations(void)
{
#ifdef __NO_MYSQL__
	expire_flat_polls();
#else
	qry("UPDATE polls SET is_active = 0 WHERE is_active = 1 AND expires_at < FROM_UNIXTIME(%ld)",
	    (long)time(NULL));
#endif
}

/* record votes to db - shared by command and websocket */
int poll_record_votes(const char *acct_name, const char *char_name, int poll_id, poll_data &poll,
		      vector<int> &choices)
{
#ifdef __NO_MYSQL__
	(void)poll;
	return record_flat_poll_votes(acct_name, char_name, poll_id, choices);
#else
	string acct_esc = escape_str(acct_name);
	string char_esc = escape_str(char_name);

	int votes_cast = 0;
	for (size_t i = 0; i < choices.size(); i++)
	{
		int option_id = 0;
		for (size_t j = 0; j < poll.options.size(); j++)
		{
			if (poll.options[j].option_num == choices[i])
			{
				option_id = poll.options[j].id;
				break;
			}
		}
		if (option_id == 0)
			continue;

		if (qry("INSERT IGNORE INTO poll_votes (poll_id, account_name, option_id, voted_at, char_name) "
			"VALUES (%d, '%s', %d, FROM_UNIXTIME(%ld), '%s')",
			poll_id, acct_esc.c_str(), option_id, (long)time(NULL), char_esc.c_str()))
		{
			votes_cast++;
		}
	}
	return votes_cast;
#endif
}

/* list polls */
void poll_display_list(P_char ch, bool show_all)
{
	vector<poll_data> polls = poll_get_all(!show_all);

	char buf[MAX_STRING_LENGTH];

	send_to_char("\r\n&+c.---------------------------------------------------------.\r\n", ch);
	if (show_all)
	{
		send_to_char(
			"|  &+WAll Polls&+c                                              |\r\n",
			ch);
	}
	else
	{
		send_to_char(
			"|  &+WActive Polls&+c                                           |\r\n",
			ch);
	}
	send_to_char("|---------------------------------------------------------|\r\n", ch);
	send_to_char(
		"| &+YID&+c  |  &+YQuestion&+c                         | &+YExp&+c   | &+YVotes&+c |\r\n",
		ch);
	send_to_char("|-----|-----------------------------------|-------|-------|\r\n&n", ch);

	if (polls.empty())
	{
		send_to_char(
			"&+c|             &+wNo polls available.&+c                         |\r\n",
			ch);
	}
	else
	{
		for (size_t i = 0; i < polls.size(); i++)
		{
			string q = polls[i].question;
			if (q.length() > 33)
			{
				q = q.substr(0, 30) + "...";
			}

			char expire_str[16];
			if (!polls[i].is_active)
			{
				snprintf(expire_str, 16, "closed");
			}
			else
			{
				format_time_remaining(polls[i].expires_at, expire_str, 16);
			}

			snprintf(buf, MAX_STRING_LENGTH,
				 "&+c| &+w%3d &+c| &n%-33s &+c| &n%-5s &+c| &+W%5d &+c|\r\n",
				 polls[i].id, q.c_str(), expire_str, polls[i].total_votes);
			send_to_char(buf, ch);
		}
	}

	send_to_char("&+c'---------------------------------------------------------'&n\r\n", ch);
	send_to_char("&+yUse 'poll <id>' to view, 'poll vote <id> <opt>' to vote.&n\r\n", ch);
}

/* show single poll */
void poll_display_single(P_char ch, int poll_id)
{
	poll_data poll = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];
	char expire_str[16];

	if (!poll.is_active)
	{
		snprintf(expire_str, 16, "closed");
	}
	else
	{
		format_time_remaining(poll.expires_at, expire_str, 16);
	}

	send_to_char("\r\n", ch);
	send_to_char("&+c.---------------------------------------------------------.\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH,
		 "&+c|  &+WPoll #%-4d                                             &+c|\r\n",
		 poll.id);
	send_to_char(buf, ch);

	/* question */
	poll_send_wrapped(ch, poll.question.c_str(), 55, "&+c|  &+Y", "&+c|");

	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);

	/* metadata */
	snprintf(buf, MAX_STRING_LENGTH,
		 "&+c|  &nBy: &+W%-15s  &nExpires: &+W%-8s  &nVotes: &+W%5d   &+c|\r\n",
		 poll.created_by.c_str(), expire_str, poll.total_votes);
	send_to_char(buf, ch);

	if (poll.multi_select)
	{
		snprintf(
			buf, MAX_STRING_LENGTH,
			"&+c|  &nType: &+Wmultiple choice (max %d)                          &+c|\r\n",
			poll.max_choices);
		send_to_char(buf, ch);
	}

	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);

	/* max votes for bar scaling */
	int max_votes = 1;
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		if (poll.options[i].vote_count > max_votes)
		{
			max_votes = poll.options[i].vote_count;
		}
	}

	/* options */
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		char bar[12];
		poll_display_bar(bar, poll.options[i].vote_count, max_votes, 10);
		poll_send_option(ch, poll.options[i].option_num, poll.options[i].text.c_str(), bar,
				 poll.options[i].vote_count);
	}

	send_to_char("&+c'---------------------------------------------------------'&n\r\n", ch);

	/* already voted? */
	const char *acct = get_account_name_safe(ch);
	if (poll_has_voted(acct, poll_id))
	{
		send_to_char("&+GYou have already voted in this poll.&n\r\n", ch);
	}
	else if (poll.is_active)
	{
		if (poll.multi_select)
		{
			snprintf(buf, MAX_STRING_LENGTH,
				 "&+yVote: poll vote %d <opt1,opt2,...>&n\r\n", poll_id);
		}
		else
		{
			snprintf(buf, MAX_STRING_LENGTH, "&+yVote: poll vote %d <option>&n\r\n",
				 poll_id);
		}
		send_to_char(buf, ch);
	}
	else
	{
		send_to_char("&+rThis poll is closed.&n\r\n", ch);
	}
}

/* detailed results */
void poll_display_results(P_char ch, int poll_id)
{
	poll_data poll = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return;
	}

	/* mortals only see closed poll results */
	if (GET_LEVEL(ch) < MINLVLIMMORTAL && poll.is_active)
	{
		send_to_char("You can only view detailed results after a poll closes.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];

	send_to_char("\r\n", ch);
	send_to_char("&+c.---------------------------------------------------------.\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH,
		 "&+c|  &+WPoll #%-4d Results                                      &+c|\r\n",
		 poll.id);
	send_to_char(buf, ch);

	/* question */
	poll_send_wrapped(ch, poll.question.c_str(), 55, "&+c|  &+Y", "&+c|");

	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH,
		 "&+c|  &nStatus: &+W%-8s  &nTotal voters: &+W%-5d                  &+c|\r\n",
		 poll.is_active ? "open" : "closed", poll.total_votes);
	send_to_char(buf, ch);
	send_to_char("&+c|---------------------------------------------------------|\r\n", ch);

	int max_votes = 1;
	for (size_t i = 0; i < poll.options.size(); i++)
	{
		if (poll.options[i].vote_count > max_votes)
		{
			max_votes = poll.options[i].vote_count;
		}
	}

	for (size_t i = 0; i < poll.options.size(); i++)
	{
		float pct = 0.0;
		if (poll.total_votes > 0)
		{
			pct = (float)poll.options[i].vote_count / poll.total_votes * 100.0;
		}

		char bar[12];
		poll_display_bar(bar, poll.options[i].vote_count, max_votes, 10);
		poll_send_option_results(ch, poll.options[i].option_num,
					 poll.options[i].text.c_str(), bar,
					 poll.options[i].vote_count, pct);
	}

	send_to_char("&+c'---------------------------------------------------------'&n\r\n", ch);
}

/* display vote bar */
static void poll_display_bar(char *buf, int count, int max_count, int bar_width)
{
	int filled = 0;
	if (max_count > 0)
	{
		filled = (count * bar_width) / max_count;
	}

	for (int i = 0; i < bar_width; i++)
	{
		buf[i] = (i < filled) ? '#' : ' ';
	}
	buf[bar_width] = '\0';
}

/* voting handler */
static void poll_handle_vote(P_char ch, char *argument)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	argument = one_argument(argument, arg1);
	argument = one_argument(argument, arg2);

	if (!*arg1 || !is_number(arg1))
	{
		send_to_char("Usage: poll vote <poll_id> <option(s)>\r\n", ch);
		return;
	}

	int poll_id = atoi(arg1);
	poll_data poll = poll_get_by_id(poll_id);

	if (poll.id == 0)
	{
		send_to_char("That poll does not exist.\r\n", ch);
		return;
	}

	if (!poll.is_active)
	{
		send_to_char("That poll is closed.\r\n", ch);
		return;
	}

	const char *acct = get_account_name_safe(ch);
	if (!acct || !strcmp(acct, "Unknown"))
	{
		send_to_char("You must be logged in with an account to vote.\r\n", ch);
		return;
	}

	if (poll_has_voted(acct, poll_id))
	{
		send_to_char("Your account has already voted in this poll.\r\n", ch);
		return;
	}

	if (!*arg2)
	{
		send_to_char("You must specify which option(s) to vote for.\r\n", ch);
		return;
	}

	/* parse options */
	vector<int> choices;
	char *token = strtok(arg2, ",");
	while (token)
	{
		while (*token == ' ')
			token++;
		if (is_number(token))
		{
			choices.push_back(atoi(token));
		}
		token = strtok(NULL, ",");
	}

	if (choices.empty())
	{
		send_to_char("Invalid option format.\r\n", ch);
		return;
	}

	if (!poll.multi_select && choices.size() > 1)
	{
		send_to_char("This poll only allows one selection.\r\n", ch);
		return;
	}

	if ((int)choices.size() > poll.max_choices)
	{
		char buf[256];
		snprintf(buf, 256, "You can only select up to %d option(s).\r\n", poll.max_choices);
		send_to_char(buf, ch);
		return;
	}

	/* validate choices */
	for (size_t i = 0; i < choices.size(); i++)
	{
		bool found = false;
		for (size_t j = 0; j < poll.options.size(); j++)
		{
			if (poll.options[j].option_num == choices[i])
			{
				found = true;
				break;
			}
		}
		if (!found)
		{
			char buf[256];
			snprintf(buf, 256, "Option %d is not valid for this poll.\r\n", choices[i]);
			send_to_char(buf, ch);
			return;
		}
	}

	/* vote */
	int votes_cast = poll_cast_vote(ch, poll_id, choices);

	if (votes_cast > 0)
	{
		send_to_char("&+GYour vote has been recorded. Thank you for participating!&n\r\n",
			     ch);
	}
	else
	{
		send_to_char("Failed to record your vote.\r\n", ch);
	}
}

/* wizard active? */
bool poll_wizard_active(P_char ch)
{
	return poll_wizards.count(ch) > 0;
}

/* cancel wizard */
void poll_wizard_cancel(P_char ch)
{
	poll_wizards.erase(ch);
}

/* start wizard */
void poll_wizard_start(P_char ch)
{
	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		send_to_char("Only immortals can create polls.\r\n", ch);
		return;
	}

	poll_wizard_data wiz;
	wiz.state = POLL_WIZ_QUESTION;
	wiz.current_option = 1;
	wiz.poll.multi_select = false;
	wiz.poll.max_choices = 1;
	wiz.poll.created_by = GET_NAME(ch);
	wiz.poll.is_active = true;
	wiz.poll.total_votes = 0;

	poll_wizards[ch] = wiz;

	send_to_char("\r\n&+W=== Poll Creation Wizard ===&n\r\n", ch);
	send_to_char("Type 'cancel' at any time to abort.\r\n\r\n", ch);
	send_to_char("&+YEnter the poll question:&n\r\n", ch);
}

/* show summary */
static void poll_wizard_show_summary(P_char ch, poll_wizard_data *wiz)
{
	char buf[MAX_STRING_LENGTH];

	send_to_char("\r\n&+W=== Poll Summary ===&n\r\n", ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+YQuestion:&n %s\r\n", wiz->poll.question.c_str());
	send_to_char(buf, ch);
	snprintf(buf, MAX_STRING_LENGTH, "&+YType:&n %s",
		 wiz->poll.multi_select ? "multiple choice" : "single choice");
	send_to_char(buf, ch);
	if (wiz->poll.multi_select)
	{
		snprintf(buf, MAX_STRING_LENGTH, " (max %d selections)\r\n", wiz->poll.max_choices);
		send_to_char(buf, ch);
	}
	else
	{
		send_to_char("\r\n", ch);
	}

	time_t duration = wiz->poll.expires_at - time(NULL);
	snprintf(buf, MAX_STRING_LENGTH, "&+YDuration:&n %ld hours\r\n", (long)(duration / 3600));
	send_to_char(buf, ch);

	send_to_char("&+YOptions:&n\r\n", ch);
	for (size_t i = 0; i < wiz->poll.options.size(); i++)
	{
		snprintf(buf, MAX_STRING_LENGTH, "  %d) %s\r\n", wiz->poll.options[i].option_num,
			 wiz->poll.options[i].text.c_str());
		send_to_char(buf, ch);
	}
}

/* wizard input */
void poll_wizard_handle_input(P_char ch, char *input)
{
	if (!poll_wizards.count(ch))
		return;

	poll_wizard_data &wiz = poll_wizards[ch];
	char *arg = skip_spaces(input);

	if (!str_cmp(arg, "cancel"))
	{
		poll_wizards.erase(ch);
		send_to_char("Poll creation cancelled.\r\n", ch);
		return;
	}

	char buf[MAX_STRING_LENGTH];

	switch (wiz.state)
	{
	case POLL_WIZ_QUESTION:
		if (strlen(arg) < 10)
		{
			send_to_char("Question too short. Please enter a meaningful question:\r\n",
				     ch);
			return;
		}
		if (strlen(arg) > MAX_POLL_QUESTION - 1)
		{
			send_to_char("Question too long. Please keep it under 512 characters:\r\n",
				     ch);
			return;
		}
		wiz.poll.question = arg;
		wiz.state = POLL_WIZ_MULTI;
		send_to_char("\r\n&+YAllow multiple selections? (yes/no):&n\r\n", ch);
		break;

	case POLL_WIZ_MULTI:
		if (!str_cmp(arg, "yes") || !str_cmp(arg, "y"))
		{
			wiz.poll.multi_select = true;
			wiz.state = POLL_WIZ_MAX_CHOICE;
			send_to_char("\r\n&+YHow many choices can voters select? (2-10):&n\r\n",
				     ch);
		}
		else if (!str_cmp(arg, "no") || !str_cmp(arg, "n"))
		{
			wiz.poll.multi_select = false;
			wiz.poll.max_choices = 1;
			wiz.state = POLL_WIZ_DURATION;
			send_to_char("\r\n&+YPoll duration in hours (1-720):&n\r\n", ch);
		}
		else
		{
			send_to_char("Please answer yes or no:\r\n", ch);
		}
		break;

	case POLL_WIZ_MAX_CHOICE:
		if (!is_number(arg) || atoi(arg) < 2 || atoi(arg) > 10)
		{
			send_to_char("Please enter a number between 2 and 10:\r\n", ch);
			return;
		}
		wiz.poll.max_choices = atoi(arg);
		wiz.state = POLL_WIZ_DURATION;
		send_to_char("\r\n&+YPoll duration in hours (1-720):&n\r\n", ch);
		break;

	case POLL_WIZ_DURATION:
		if (!is_number(arg) || atoi(arg) < 1 || atoi(arg) > 720)
		{
			send_to_char("Please enter hours between 1 and 720 (30 days max):\r\n", ch);
			return;
		}
		wiz.poll.created_at = time(NULL);
		wiz.poll.expires_at = time(NULL) + (atoi(arg) * 3600);
		wiz.state = POLL_WIZ_OPTIONS;
		wiz.current_option = 1;
		send_to_char(
			"\r\n&+YEnter option 1 (or 'done' when finished adding options):&n\r\n",
			ch);
		break;

	case POLL_WIZ_OPTIONS:
		if (!str_cmp(arg, "done"))
		{
			if (wiz.poll.options.size() < 2)
			{
				send_to_char(
					"You need at least 2 options. Enter another option:\r\n",
					ch);
				return;
			}
			wiz.state = POLL_WIZ_CONFIRM;
			poll_wizard_show_summary(ch, &wiz);
			send_to_char("\r\n&+YCreate this poll? (yes/no):&n\r\n", ch);
		}
		else
		{
			if (strlen(arg) < 1)
			{
				send_to_char("Option cannot be empty. Enter option or 'done':\r\n",
					     ch);
				return;
			}
			if (strlen(arg) > MAX_OPTION_TEXT - 1)
			{
				send_to_char(
					"Option too long. Please keep it under 256 characters:\r\n",
					ch);
				return;
			}
			poll_option opt;
			opt.option_num = wiz.current_option;
			opt.text = arg;
			opt.vote_count = 0;
			opt.id = 0;
			wiz.poll.options.push_back(opt);
			wiz.current_option++;

			if (wiz.current_option > MAX_POLL_OPTIONS)
			{
				wiz.state = POLL_WIZ_CONFIRM;
				send_to_char("Maximum options reached.\r\n", ch);
				poll_wizard_show_summary(ch, &wiz);
				send_to_char("\r\n&+YCreate this poll? (yes/no):&n\r\n", ch);
			}
			else
			{
				snprintf(buf, MAX_STRING_LENGTH,
					 "\r\n&+YEnter option %d (or 'done' when finished):&n\r\n",
					 wiz.current_option);
				send_to_char(buf, ch);
			}
		}
		break;

	case POLL_WIZ_CONFIRM:
		if (!str_cmp(arg, "yes") || !str_cmp(arg, "y"))
		{
			if (poll_create(&wiz.poll))
			{
				send_to_char("&+GPoll created successfully!&n\r\n", ch);

				/* announce */
				snprintf(buf, MAX_STRING_LENGTH,
					 "&+W[POLL]&n %s has created a new poll: %s\r\n",
					 GET_NAME(ch), wiz.poll.question.c_str());
				send_to_all(buf);

				/* websocket */
				poll_broadcast_new(wiz.poll.id, wiz.poll.question.c_str(),
						   wiz.poll.created_by.c_str());
			}
			else
			{
				send_to_char("&+RError creating poll. Check logs.&n\r\n", ch);
			}
			poll_wizards.erase(ch);
		}
		else if (!str_cmp(arg, "no") || !str_cmp(arg, "n"))
		{
			poll_wizards.erase(ch);
			send_to_char("Poll creation cancelled.\r\n", ch);
		}
		else
		{
			send_to_char("Please answer yes or no:\r\n", ch);
		}
		break;
	}
}

/* broadcasts */
void poll_broadcast_new(int poll_id, const char *question, const char *creator)
{
	extern struct descriptor_data *descriptor_list;
	struct descriptor_data *d;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "poll_new");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", poll_id);
	cJSON_AddStringToObject(data, "question", question ? question : "");
	cJSON_AddStringToObject(data, "creator", creator ? creator : "");
	cJSON_AddItemToObject(root, "data", data);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->account)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
}

void poll_broadcast_vote(int poll_id, int total_votes)
{
	extern struct descriptor_data *descriptor_list;
	struct descriptor_data *d;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "poll_update");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", poll_id);
	cJSON_AddNumberToObject(data, "total_votes", total_votes);
	cJSON_AddItemToObject(root, "data", data);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->account)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
}

void poll_broadcast_close(int poll_id, const char *question)
{
	extern struct descriptor_data *descriptor_list;
	struct descriptor_data *d;

	cJSON *root = cJSON_CreateObject();
	if (!root)
		return;

	cJSON_AddStringToObject(root, "type", "poll_close");

	cJSON *data = cJSON_CreateObject();
	cJSON_AddNumberToObject(data, "id", poll_id);
	cJSON_AddStringToObject(data, "question", question ? question : "");
	cJSON_AddItemToObject(root, "data", data);

	char *json = cJSON_PrintUnformatted(root);
	cJSON_Delete(root);
	if (!json)
		return;

	for (d = descriptor_list; d; d = d->next)
	{
		if (d->websocket && d->account)
		{
			websocket_send_text(d, json);
		}
	}

	free(json);
}

/* main handler */
void do_poll(P_char ch, char *argument, int /*cmd*/)
{
	char arg1[MAX_INPUT_LENGTH];
	char arg2[MAX_INPUT_LENGTH];

	if (IS_NPC(ch))
	{
		send_to_char("Mobs can't participate in polls.\r\n", ch);
		return;
	}

	if (GET_LEVEL(ch) < MINIMUM_POLL_LEVEL)
	{
		send_to_char("You must be at least level 30 to participate in polls.\r\n", ch);
		return;
	}

	/* wizard mode? */
	if (poll_wizard_active(ch))
	{
		poll_wizard_handle_input(ch, argument);
		return;
	}

	argument = one_argument(argument, arg1);

	/* no arg or "list" */
	if (!*arg1 || !str_cmp(arg1, "list"))
	{
		poll_display_list(ch, false);
		return;
	}

	/* numeric = view */
	if (is_number(arg1))
	{
		poll_display_single(ch, atoi(arg1));
		return;
	}

	/* view */
	if (!str_cmp(arg1, "view"))
	{
		argument = one_argument(argument, arg2);
		if (!*arg2 || !is_number(arg2))
		{
			send_to_char("Usage: poll view <poll_id>\r\n", ch);
			return;
		}
		poll_display_single(ch, atoi(arg2));
		return;
	}

	/* vote */
	if (!str_cmp(arg1, "vote"))
	{
		poll_handle_vote(ch, argument);
		return;
	}

	/* results */
	if (!str_cmp(arg1, "results"))
	{
		argument = one_argument(argument, arg2);
		if (!*arg2 || !is_number(arg2))
		{
			send_to_char("Usage: poll results <poll_id>\r\n", ch);
			return;
		}
		poll_display_results(ch, atoi(arg2));
		return;
	}

	/* imm only below */
	if (GET_LEVEL(ch) < MINLVLIMMORTAL)
	{
		send_to_char("Unknown poll command. Try: list, view, vote, results\r\n", ch);
		return;
	}

	/* create */
	if (!str_cmp(arg1, "create"))
	{
		poll_wizard_start(ch);
		return;
	}

	/* close */
	if (!str_cmp(arg1, "close"))
	{
		argument = one_argument(argument, arg2);
		if (!*arg2 || !is_number(arg2))
		{
			send_to_char("Usage: poll close <poll_id>\r\n", ch);
			return;
		}
		poll_close(atoi(arg2), ch);
		return;
	}

	/* all (including closed) */
	if (!str_cmp(arg1, "all"))
	{
		poll_display_list(ch, true);
		return;
	}

	/* unknown */
	send_to_char("Poll commands: list, view, vote, results", ch);
	if (GET_LEVEL(ch) >= MINLVLIMMORTAL)
	{
		send_to_char(", create, close, all", ch);
	}
	send_to_char("\r\n", ch);
}
