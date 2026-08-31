#include "persistence/persistence_mode.h"
#include "net/poll.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
std::string state_root;

void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

poll_data make_poll(const std::string &question, bool multi_select, time_t created_at,
		    time_t expires_at)
{
	poll_data poll = {};
	poll.question = question;
	poll.created_by = "Builder";
	poll.created_at = created_at;
	poll.expires_at = expires_at;
	poll.is_active = true;
	poll.multi_select = multi_select;
	poll.max_choices = multi_select ? 2 : 1;
	poll.options = { { 0, 1, "First option", 0 }, { 0, 2, "Second option", 0 } };
	return poll;
}

std::string read_file(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
}
} // namespace

const char *persistence_mode_flatfile_root()
{
	return state_root.c_str();
}

void logit(const char *, const char *, ...) {}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	state_root = argv[1];
	const fs::path metadata = fs::path(state_root) / "metadata";
	fs::create_directories(metadata);
	fs::permissions(state_root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(metadata, fs::perms::owner_all, fs::perm_options::replace);

	require(poll_get_all(false).empty(), "missing catalog was not an empty poll list");
	require(poll_get_by_id(1).id == 0, "missing poll lookup returned a poll");
	require(!poll_has_voted("Account", 1), "missing catalog reported a vote");

	const time_t now = time(nullptr);
	poll_data invalid = make_poll("Invalid poll", false, now, now + 3600);
	invalid.options.pop_back();
	require(!poll_create(&invalid), "poll with one option was accepted");
	require(!fs::exists(metadata / "polls"), "invalid poll created an authority file");

	poll_data first = make_poll("Choose both?", true, now, now + 3600);
	require(poll_create(&first), "first poll create failed");
	require(first.id == 1 && first.options[0].id == 1 && first.options[1].id == 2,
		"first poll identities were not allocated canonically");

	poll_data second = make_poll("Choose one?", false, now, now + 7200);
	require(poll_create(&second), "second poll create failed");
	require(second.id == 2 && second.options[0].id == 3 && second.options[1].id == 4,
		"second poll identities did not advance");

	std::vector<poll_data> polls = poll_get_all(true);
	require(polls.size() == 2 && polls[0].id == 2 && polls[1].id == 1,
		"active polls were not returned newest first");
	poll_data reloaded = poll_get_by_id(first.id);
	require(reloaded.question == first.question && reloaded.options.size() == 2,
		"created poll did not survive a reload");

	std::vector<int> both = { 1, 2 };
	require(poll_record_votes("Account", "Hero", first.id, first, both) == 2,
		"multi-select vote was not recorded");
	require(poll_has_voted("account", first.id),
		"account vote lookup was not case-insensitive");
	require(poll_record_votes("ACCOUNT", "Hero", first.id, first, both) == 0,
		"duplicate account-option votes were recorded");
	std::vector<int> second_only = { 2 };
	require(poll_record_votes("Other", "Alt", first.id, first, second_only) == 1,
		"second account vote was not recorded");
	reloaded = poll_get_by_id(first.id);
	require(reloaded.total_votes == 2 && reloaded.options[0].vote_count == 1 &&
			reloaded.options[1].vote_count == 2,
		"poll totals did not match stored distinct voters and choices");

	poll_data expired = make_poll("Already expired?", false, now - 7200, now - 3600);
	require(poll_create(&expired), "expired poll fixture create failed");
	require(poll_get_all(true).size() == 2, "expired poll appeared in the active list");
	require(poll_get_by_id(expired.id).is_active,
		"expiration fixture was closed before maintenance");
	poll_check_expirations();
	require(!poll_get_by_id(expired.id).is_active,
		"expiration maintenance did not durably close the poll");

	const fs::path record = metadata / "polls";
	require(fs::is_regular_file(record), "poll authority record was not created");
	const fs::perms permissions = fs::status(record).permissions();
	require((permissions & fs::perms::group_all) == fs::perms::none &&
			(permissions & fs::perms::others_all) == fs::perms::none,
		"poll authority record permissions were not private");

	{
		std::fstream file(record, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "poll record could not be opened for corruption test");
		file.seekg(-2, std::ios::end);
		char byte = 0;
		file.read(&byte, 1);
		byte ^= 0x01;
		file.seekp(-2, std::ios::end);
		file.write(&byte, 1);
	}
	const std::string corrupt_record = read_file(record);
	require(poll_get_all(false).empty(), "corrupt poll record was accepted");
	poll_data refused = make_poll("Must not overwrite", false, now, now + 3600);
	require(!poll_create(&refused), "poll create overwrote corrupt authority");
	require(read_file(record) == corrupt_record, "corrupt poll authority was modified");

	std::cout << "flat-file polls passed\n";
	return 0;
}
