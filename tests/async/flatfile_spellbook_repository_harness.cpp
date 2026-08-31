#include "flatfile/flatfile_spellbook_repository.h"

#include <algorithm>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <sys/wait.h>
#include <unistd.h>
#include <vector>

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
	const fs::path domains = root / "domains";
	fs::create_directories(domains);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	std::string error;
	std::vector<int32_t> mobs;
	require(flatfile_spellbook_list(root.string(), 42, &mobs, &error) ==
			flatfile_spellbook_result::not_found,
		"missing spellbook authority did not fail closed");
	std::vector<flatfile_spellbook_record> baseline = { { 43, { 900 } }, { 42, { 700, 500 } } };
	require(flatfile_spellbook_establish(root.string(), baseline, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_establish(root.string(), baseline, &error) ==
				flatfile_spellbook_result::already_exists,
		"spellbook authority was not established idempotently: " + error);
	require(flatfile_spellbook_list(root.string(), 42, &mobs, &error) ==
				flatfile_spellbook_result::ok &&
			mobs == std::vector<int32_t>({ 500, 700 }),
		"spellbook baseline was not canonicalized");
	require(flatfile_spellbook_list(root.string(), 99, &mobs, &error) ==
				flatfile_spellbook_result::ok &&
			mobs.empty(),
		"absent player row was not an authoritative empty set");
	bool contains = false;
	require(flatfile_spellbook_contains(root.string(), 42, 700, &contains, &error) ==
				flatfile_spellbook_result::ok &&
			contains &&
			flatfile_spellbook_contains(root.string(), 42, 701, &contains, &error) ==
				flatfile_spellbook_result::ok &&
			!contains,
		"spellbook membership projection was incorrect");
	require(flatfile_spellbook_add(root.string(), 42, 600, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_add(root.string(), 42, 600, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_list(root.string(), 42, &mobs, &error) ==
				flatfile_spellbook_result::ok &&
			mobs == std::vector<int32_t>({ 500, 600, 700 }),
		"spellbook add was not sorted and idempotent");

	std::vector<pid_t> children;
	for (int index = 0; index < 8; ++index)
	{
		const pid_t child = fork();
		require(child >= 0, "fork failed");
		if (!child)
		{
			std::string child_error;
			_exit(flatfile_spellbook_add(root.string(), 42, 1000 + index,
						     &child_error) ==
					      flatfile_spellbook_result::ok ?
				      0 :
				      1);
		}
		children.push_back(child);
	}
	for (pid_t child : children)
	{
		int status = 0;
		require(waitpid(child, &status, 0) == child && WIFEXITED(status) &&
				WEXITSTATUS(status) == 0,
			"concurrent spellbook writer failed");
	}
	require(flatfile_spellbook_list(root.string(), 42, &mobs, &error) ==
				flatfile_spellbook_result::ok &&
			mobs.size() == 11 && mobs.front() == 500 && mobs.back() == 1007,
		"concurrent spellbook additions were lost");
	require(flatfile_spellbook_remove(root.string(), 42, 1003, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_remove(root.string(), 42, 1003, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_list(root.string(), 42, &mobs, &error) ==
				flatfile_spellbook_result::ok &&
			mobs.size() == 10 &&
			std::find(mobs.begin(), mobs.end(), 1003) == mobs.end(),
		"spellbook remove was not durable and idempotent");
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_spellbook_prepare_clear(root.string(), lock, 42,
								 &operation, &error) ==
					flatfile_spellbook_result::ok &&
				operation.filename == "spellbook_catalog" &&
				!operation.bytes.empty(),
			"spellbook clear was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"prepared spellbook clear did not commit: " + error);
	}
	require(flatfile_spellbook_clear(root.string(), 42, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_clear(root.string(), 42, &error) ==
				flatfile_spellbook_result::ok &&
			flatfile_spellbook_list(root.string(), 42, &mobs, &error) ==
				flatfile_spellbook_result::ok &&
			mobs.empty(),
		"prepared spellbook clear was not durable and idempotent");
	std::vector<flatfile_spellbook_record> conflict = { { 43, { 901 } } };
	require(flatfile_spellbook_establish(root.string(), conflict, &error) ==
			flatfile_spellbook_result::invalid,
		"conflicting spellbook baseline was accepted");
	const fs::path catalog = domains / "spellbook_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open spellbook catalog for corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x4d;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_spellbook_list(root.string(), 43, &mobs, &error) ==
				flatfile_spellbook_result::invalid &&
			flatfile_spellbook_add(root.string(), 43, 902, &error) ==
				flatfile_spellbook_result::invalid &&
			flatfile_spellbook_remove(root.string(), 43, 900, &error) ==
				flatfile_spellbook_result::invalid,
		"corrupt spellbook authority was read or overwritten");
	for (const auto &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary spellbook file was left behind");
	std::cout << "flat-file spellbook repository passed\n";
	return 0;
}
