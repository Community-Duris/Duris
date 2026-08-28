#include "flatfile_identity_repository.h"

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
	const fs::path identities = root / "identities";
	const fs::path names = identities / "names";
	fs::create_directories(names);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(identities, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(names, fs::perms::owner_all, fs::perm_options::replace);

	std::string error;
	int32_t highest = -1;
	require(flatfile_identity_current_highest_pid(root.string(), &highest, &error) ==
				flatfile_identity_result::ok &&
			highest == 0,
		"empty allocator did not start at zero: " + error);
	int32_t alpha_pid = 0, beta_pid = 0, reserved_pid = 0;
	require(flatfile_identity_allocate_pid(root.string(), &alpha_pid, &error) ==
				flatfile_identity_result::ok &&
			alpha_pid == 1,
		"first PID allocation failed: " + error);
	require(flatfile_identity_allocate_pid(root.string(), &beta_pid, &error) ==
				flatfile_identity_result::ok &&
			beta_pid == 2,
		"second PID allocation failed: " + error);
	require(flatfile_identity_claim(root.string(), alpha_pid, "Alpha", "Account-One", &error) ==
			flatfile_identity_result::ok,
		"alpha identity claim failed: " + error);
	require(flatfile_identity_claim(root.string(), beta_pid, "Beta", "Account-Two", &error) ==
			flatfile_identity_result::ok,
		"beta identity claim failed: " + error);

	flatfile_identity_record record;
	require(flatfile_identity_lookup_name(root.string(), "aLPHa", &record, &error) ==
				flatfile_identity_result::ok &&
			record.pid == alpha_pid && record.account == "Account-One" && record.active,
		"canonical name lookup failed: " + error);
	require(flatfile_identity_lookup_pid(root.string(), beta_pid, &record, &error) ==
				flatfile_identity_result::ok &&
			record.name == "Beta",
		"PID lookup failed: " + error);
	require(flatfile_identity_allocate_pid(root.string(), &reserved_pid, &error) ==
				flatfile_identity_result::ok &&
			reserved_pid == 3,
		"reserved PID allocation failed: " + error);
	require(flatfile_identity_claim(root.string(), reserved_pid, "ALPHA", "Account-Two",
					&error) == flatfile_identity_result::conflict,
		"duplicate canonical name was accepted");
	require(flatfile_identity_claim(root.string(), alpha_pid, "Other", "Account-One", &error) ==
			flatfile_identity_result::conflict,
		"duplicate PID was accepted");

	require(flatfile_identity_rename(root.string(), alpha_pid, "Alpha", "Gamma", &error) ==
			flatfile_identity_result::ok,
		"identity rename failed: " + error);
	require(flatfile_identity_lookup_name(root.string(), "Alpha", &record, &error) ==
			flatfile_identity_result::not_found,
		"old identity name remained active");
	require(flatfile_identity_rename(root.string(), alpha_pid, "Gamma", "Beta", &error) ==
			flatfile_identity_result::conflict,
		"rename collision was accepted");
	require(flatfile_identity_set_blocked(root.string(), alpha_pid, true, &error) ==
			flatfile_identity_result::ok,
		"identity block failed: " + error);
	require(flatfile_identity_lookup_pid(root.string(), alpha_pid, &record, &error) ==
				flatfile_identity_result::ok &&
			record.blocked,
		"blocked state did not round trip");
	require(flatfile_identity_remove(root.string(), beta_pid, "Beta", &error) ==
			flatfile_identity_result::ok,
		"identity removal failed: " + error);
	require(flatfile_identity_lookup_name(root.string(), "Beta", &record, &error) ==
			flatfile_identity_result::not_found,
		"removed name remained active");
	require(flatfile_identity_lookup_pid(root.string(), beta_pid, &record, &error) ==
				flatfile_identity_result::ok &&
			!record.active && record.blocked,
		"PID tombstone was not retained");

	constexpr int worker_count = 4;
	constexpr int allocations_per_worker = 10;
	for (int worker = 0; worker < worker_count; ++worker)
	{
		const pid_t child = fork();
		require(child >= 0, "fork failed");
		if (!child)
		{
			for (int allocation = 0; allocation < allocations_per_worker; ++allocation)
			{
				int32_t pid = 0;
				std::string child_error;
				if (flatfile_identity_allocate_pid(root.string(), &pid,
								   &child_error) !=
				    flatfile_identity_result::ok)
					_exit(2);
			}
			_exit(0);
		}
	}
	for (int worker = 0; worker < worker_count; ++worker)
	{
		int status = 0;
		require(wait(&status) > 0 && WIFEXITED(status) && WEXITSTATUS(status) == 0,
			"concurrent allocator worker failed");
	}
	require(flatfile_identity_current_highest_pid(root.string(), &highest, &error) ==
				flatfile_identity_result::ok &&
			highest == reserved_pid + worker_count * allocations_per_worker,
		"concurrent allocations lost or duplicated a publication");

	require(flatfile_identity_lookup_name(root.string(), "../escape", &record, &error) ==
			flatfile_identity_result::invalid,
		"unsafe identity name was accepted");
	const fs::path catalog = names / "catalog.identity";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open identity catalog for corruption test");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x5a;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_identity_lookup_pid(root.string(), alpha_pid, &record, &error) ==
			flatfile_identity_result::invalid,
		"corrupt identity checksum was accepted");

	for (const fs::directory_entry &entry : fs::directory_iterator(names))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary identity file was left behind");

	std::cout << "flat-file identity repository passed\n";
	return 0;
}
