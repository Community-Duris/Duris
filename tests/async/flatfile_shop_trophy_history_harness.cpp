#include "flatfile_shop_trophy_history.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

namespace fs = std::filesystem;

namespace
{
void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

void prepare_root(const fs::path &root)
{
	fs::create_directories(root / "metadata");
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(root / "metadata", fs::perms::owner_all, fs::perm_options::replace);
}

std::vector<char> read_bytes(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	return std::vector<char>(std::istreambuf_iterator<char>(input),
				 std::istreambuf_iterator<char>());
}
} // namespace

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	prepare_root(root);
	constexpr int64_t day = 60 * 60 * 24;
	constexpr int64_t now = 100 * day + 10;
	std::string error;
	int count = -1;
	require(flatfile_shop_trophy_count(root.string().c_str(), 1001, now, &count, &error) ==
				flatfile_shop_trophy_result::ok &&
			count == 0,
		"missing shop history was not empty");
	require(flatfile_shop_trophy_record(root.string().c_str(), 1001, 50, 7, 92 * day + 100,
					    &error) == flatfile_shop_trophy_result::ok &&
			flatfile_shop_trophy_record(root.string().c_str(), 1001, 60, 8,
						    93 * day + 100,
						    &error) == flatfile_shop_trophy_result::ok &&
			flatfile_shop_trophy_record(root.string().c_str(), 2002, 70, 9,
						    100 * day + 1,
						    &error) == flatfile_shop_trophy_result::ok &&
			flatfile_shop_trophy_record(root.string().c_str(), 1001, 80, 10,
						    100 * day + 2,
						    &error) == flatfile_shop_trophy_result::ok,
		"shop sale history could not be recorded");
	require(flatfile_shop_trophy_count(root.string().c_str(), 1001, now, &count, &error) ==
				flatfile_shop_trophy_result::ok &&
			count == 2,
		"seven-day UTC window or pruning differs from SQL");
	require(flatfile_shop_trophy_count(root.string().c_str(), 2002, now, &count, &error) ==
				flatfile_shop_trophy_result::ok &&
			count == 1,
		"shop history mixed item vnums");
	require(flatfile_shop_trophy_record(root.string().c_str(), 1001, 90, 11, 101 * day + 2,
					    &error) == flatfile_shop_trophy_result::ok &&
			flatfile_shop_trophy_count(root.string().c_str(), 1001, 101 * day + 10,
						   &count,
						   &error) == flatfile_shop_trophy_result::ok &&
			count == 2,
		"later sale did not advance the rolling seven-day history");

	const fs::path authority = root / "metadata" / "shop-trophy";
	require(fs::is_regular_file(authority), "shop history authority was not created");
	const auto permissions = fs::status(authority).permissions();
	require((permissions & fs::perms::group_all) == fs::perms::none &&
			(permissions & fs::perms::others_all) == fs::perms::none,
		"shop history authority is not private");
	std::fstream corrupt(authority, std::ios::in | std::ios::out | std::ios::binary);
	require(corrupt.good(), "could not open shop history for corruption test");
	corrupt.seekg(-1, std::ios::end);
	char byte = 0;
	corrupt.read(&byte, 1);
	byte ^= 0x5a;
	corrupt.seekp(-1, std::ios::end);
	corrupt.write(&byte, 1);
	corrupt.close();
	const std::vector<char> corrupted = read_bytes(authority);
	require(flatfile_shop_trophy_count(root.string().c_str(), 1001, now, &count, &error) ==
			flatfile_shop_trophy_result::corrupt,
		"corrupt shop history did not fail closed on read");
	require(flatfile_shop_trophy_record(root.string().c_str(), 1001, 100, 12, now, &error) ==
				flatfile_shop_trophy_result::corrupt &&
			read_bytes(authority) == corrupted,
		"a sale overwrote corrupt shop history");

	const fs::path symlink_root = root.parent_path() / "symlink-state";
	prepare_root(symlink_root);
	const fs::path target = root.parent_path() / "outside-shop-history";
	{
		std::ofstream output(target, std::ios::binary);
		output << "do not replace";
	}
	fs::create_symlink(target, symlink_root / "metadata" / "shop-trophy");
	const std::vector<char> target_before = read_bytes(target);
	require(flatfile_shop_trophy_record(symlink_root.string().c_str(), 1001, 100, 12, now,
					    &error) == flatfile_shop_trophy_result::corrupt &&
			read_bytes(target) == target_before,
		"shop history followed or replaced a symlink");

	std::cout << "flat-file shop-trophy history regression passed\n";
	return 0;
}
