#include "flatfile_account_repository.h"
#include "flatfile_store.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/stat.h>
#include <sys/wait.h>
#include <unistd.h>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path directory = root / "identities" / "accounts";
	fs::create_directories(directory);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "identities", fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(directory, fs::perms::owner_all, fs::perm_options::replace);

	flatfile_account_record source;
	source.name = "Alpha";
	source.email = "alpha@example.test";
	source.password_hash = "$2b$test-hash";
	source.confirmation = "confirm-token";
	source.blocked = 1;
	source.confirmed = 1;
	source.confirmation_sent = 1;
	source.last_login = 100;
	source.last_good = 200;
	source.last_evil = 300;
	source.flags[0] = 11;
	source.flags[1] = 22;
	source.flags[2] = 33;
	source.flags[3] = 44;
	source.ips.push_back({ "host.example.test", "192.0.2.4", 9 });
	flatfile_account_character character;
	character.pid = 42;
	character.name = "Hero";
	character.login_count = 7;
	character.last_login = 1234;
	character.blocked = 1;
	character.racewar = 2;
	character.level = 50;
	character.race = 3;
	character.primary_class = 4;
	character.secondary_class = 5;
	character.last_room = 600;
	character.last_save = 700;
	source.characters.push_back(character);

	std::string error;
	uint64_t revision = 0;
	require(flatfile_account_save(root.string(), source, 0, &revision, &error) ==
			flatfile_account_result::ok,
		"initial account save failed: " + error);
	require(revision == 1, "initial revision was not 1");

	flatfile_account_record loaded;
	require(flatfile_account_load(root.string(), "aLpHa", &loaded, &error) ==
			flatfile_account_result::ok,
		"case-insensitive account load failed: " + error);
	require(loaded.revision == 1 && loaded.name == source.name &&
			loaded.email == source.email &&
			loaded.password_hash == source.password_hash && loaded.flags[3] == 44,
		"account scalar round trip failed");
	require(loaded.ips.size() == 1 && loaded.ips[0].address == "192.0.2.4" &&
			loaded.ips[0].count == 9,
		"account IP round trip failed");
	require(loaded.characters.size() == 1 && loaded.characters[0].pid == 42 &&
			loaded.characters[0].name == "Hero" &&
			loaded.characters[0].last_save == 700,
		"account character round trip failed");

	uint64_t ignored_revision = 0;
	require(flatfile_account_save(root.string(), source, 0, &ignored_revision, &error) ==
			flatfile_account_result::conflict,
		"stale account revision was accepted");
	source.email = "updated@example.test";
	require(flatfile_account_save(root.string(), source, 1, &revision, &error) ==
				flatfile_account_result::ok &&
			revision == 2,
		"account revision update failed: " + error);

	int ready_pipe[2], go_pipe[2];
	require(pipe(ready_pipe) == 0 && pipe(go_pipe) == 0, "could not create account race pipes");
	pid_t children[2];
	for (int index = 0; index < 2; ++index)
	{
		children[index] = fork();
		require(children[index] >= 0, "account race fork failed");
		if (!children[index])
		{
			close(ready_pipe[0]);
			close(go_pipe[1]);
			flatfile_account_record competing;
			std::string child_error;
			if (flatfile_account_load(root.string(), "Alpha", &competing,
						  &child_error) != flatfile_account_result::ok ||
			    competing.revision != 2)
				_exit(4);
			char signal = 'r';
			if (write(ready_pipe[1], &signal, 1) != 1 ||
			    read(go_pipe[0], &signal, 1) != 1)
				_exit(5);
			competing.email = index ? "racer-two@example.test" :
						  "racer-one@example.test";
			uint64_t competing_revision = 0;
			const flatfile_account_result competing_result = flatfile_account_save(
				root.string(), competing, 2, &competing_revision, &child_error);
			_exit(competing_result == flatfile_account_result::ok	    ? 0 :
			      competing_result == flatfile_account_result::conflict ? 3 :
										      6);
		}
	}
	close(ready_pipe[1]);
	close(go_pipe[0]);
	char signal = 0;
	require(read(ready_pipe[0], &signal, 1) == 1 && read(ready_pipe[0], &signal, 1) == 1,
		"account race workers did not become ready");
	require(write(go_pipe[1], "gg", 2) == 2, "account race workers could not be released");
	close(ready_pipe[0]);
	close(go_pipe[1]);
	int successes = 0, conflicts = 0;
	for (pid_t child : children)
	{
		int status = 0;
		require(waitpid(child, &status, 0) == child && WIFEXITED(status),
			"account race worker did not exit cleanly");
		successes += WEXITSTATUS(status) == 0;
		conflicts += WEXITSTATUS(status) == 3;
	}
	require(successes == 1 && conflicts == 1,
		"cross-process stale account write was not rejected");
	require(flatfile_account_load(root.string(), "Alpha", &loaded, &error) ==
				flatfile_account_result::ok &&
			loaded.revision == 3,
		"cross-process account winner did not publish revision 3");

	bool exists = true;
	require(flatfile_account_exists(root.string(), "missing", &exists, &error) ==
				flatfile_account_result::ok &&
			!exists,
		"missing account existence check failed");
	require(flatfile_account_load(root.string(), "../escape", &loaded, &error) ==
			flatfile_account_result::invalid,
		"unsafe account name was accepted");
	const std::vector<uint8_t> marker = { 1, 2, 3 };
	require(flatfile_atomic_write(directory.string(), "remove-test", marker, &error) &&
			flatfile_atomic_remove(directory.string(), "remove-test", false, &error) &&
			!fs::exists(directory / "remove-test") &&
			flatfile_atomic_remove(directory.string(), "remove-test", true, &error),
		"durable authority removal failed: " + error);
	fs::create_symlink("/etc/passwd", directory / "remove-symlink");
	require(!flatfile_atomic_remove(directory.string(), "remove-symlink", false, &error) &&
			fs::is_symlink(directory / "remove-symlink"),
		"authority removal followed a symlink");
	fs::remove(directory / "remove-symlink");

	const fs::path account_path = directory / "616c706861.acct";
	{
		std::fstream file(account_path, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open account for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_account_load(root.string(), "Alpha", &loaded, &error) ==
			flatfile_account_result::invalid,
		"corrupt checksum was accepted");

	fs::remove(account_path);
	fs::create_symlink("/etc/passwd", account_path);
	require(flatfile_account_load(root.string(), "Alpha", &loaded, &error) ==
			flatfile_account_result::invalid,
		"symlink account was accepted");
	fs::remove(account_path);
	require(flatfile_account_save(root.string(), source, 0, &revision, &error) ==
			flatfile_account_result::ok,
		"account restore after symlink test failed: " + error);
	chmod(account_path.c_str(), 0644);
	require(flatfile_account_load(root.string(), "Alpha", &loaded, &error) ==
			flatfile_account_result::invalid,
		"insecure account permissions were accepted");

	for (const fs::directory_entry &entry : fs::directory_iterator(directory))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary authority file was left behind");

	std::cout << "flat-file account repository passed\n";
	return 0;
}
