#include "flatfile/flatfile_recipe_repository.h"

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
	std::vector<int32_t> recipes;
	require(flatfile_recipe_list(root.string(), 42, &recipes, &error) ==
			flatfile_recipe_result::not_found,
		"missing recipe authority did not fail closed");
	std::vector<flatfile_recipe_record> baseline = { { 43, { 900 } }, { 42, { 700, 500 } } };
	require(flatfile_recipe_establish(root.string(), baseline, &error) ==
				flatfile_recipe_result::ok &&
			flatfile_recipe_establish(root.string(), baseline, &error) ==
				flatfile_recipe_result::already_exists,
		"recipe authority was not established idempotently: " + error);
	require(flatfile_recipe_list(root.string(), 42, &recipes, &error) ==
				flatfile_recipe_result::ok &&
			recipes == std::vector<int32_t>({ 500, 700 }),
		"recipe baseline was not canonicalized");
	require(flatfile_recipe_list(root.string(), 99, &recipes, &error) ==
				flatfile_recipe_result::ok &&
			recipes.empty(),
		"absent player row was not an authoritative empty set");
	bool contains = false;
	require(flatfile_recipe_contains(root.string(), 42, 700, &contains, &error) ==
				flatfile_recipe_result::ok &&
			contains &&
			flatfile_recipe_contains(root.string(), 42, 701, &contains, &error) ==
				flatfile_recipe_result::ok &&
			!contains,
		"recipe membership projection was incorrect");
	require(flatfile_recipe_add(root.string(), 42, 600, &error) == flatfile_recipe_result::ok &&
			flatfile_recipe_add(root.string(), 42, 600, &error) ==
				flatfile_recipe_result::ok &&
			flatfile_recipe_list(root.string(), 42, &recipes, &error) ==
				flatfile_recipe_result::ok &&
			recipes == std::vector<int32_t>({ 500, 600, 700 }),
		"recipe add was not sorted and idempotent");

	std::vector<pid_t> children;
	for (int index = 0; index < 8; ++index)
	{
		const pid_t child = fork();
		require(child >= 0, "fork failed");
		if (!child)
		{
			std::string child_error;
			_exit(flatfile_recipe_add(root.string(), 42, 1000 + index, &child_error) ==
					      flatfile_recipe_result::ok ?
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
			"concurrent recipe writer failed");
	}
	require(flatfile_recipe_list(root.string(), 42, &recipes, &error) ==
				flatfile_recipe_result::ok &&
			recipes.size() == 11 && recipes.front() == 500 && recipes.back() == 1007,
		"concurrent recipe additions were lost");
	{
		flatfile_authority_lock lock;
		flatfile_authority_operation operation;
		require(lock.acquire(root.string(), &error) &&
				flatfile_recipe_prepare_clear(root.string(), lock, 42, &operation,
							      &error) ==
					flatfile_recipe_result::ok &&
				operation.filename == "recipe_catalog" && !operation.bytes.empty(),
			"recipe clear was not prepared: " + error);
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 { operation }, &error) ==
				flatfile_authority_transaction_result::ok,
			"prepared recipe clear did not commit: " + error);
	}
	require(flatfile_recipe_clear(root.string(), 42, &error) == flatfile_recipe_result::ok &&
			flatfile_recipe_clear(root.string(), 42, &error) ==
				flatfile_recipe_result::ok &&
			flatfile_recipe_list(root.string(), 42, &recipes, &error) ==
				flatfile_recipe_result::ok &&
			recipes.empty(),
		"prepared recipe clear was not durable and idempotent");
	std::vector<flatfile_recipe_record> conflict = { { 43, { 901 } } };
	require(flatfile_recipe_establish(root.string(), conflict, &error) ==
			flatfile_recipe_result::invalid,
		"conflicting recipe baseline was accepted");
	const fs::path catalog = domains / "recipe_catalog";
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open recipe catalog for corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x4d;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_recipe_list(root.string(), 43, &recipes, &error) ==
				flatfile_recipe_result::invalid &&
			flatfile_recipe_add(root.string(), 43, 902, &error) ==
				flatfile_recipe_result::invalid,
		"corrupt recipe authority was read or overwritten");
	for (const auto &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary recipe file was left behind");
	std::cout << "flat-file recipe repository passed\n";
	return 0;
}
