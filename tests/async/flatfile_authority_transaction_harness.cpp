#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

namespace fs = std::filesystem;

static void require(bool condition, const std::string &message)
{
	if (!condition)
	{
		std::cerr << message << '\n';
		exit(1);
	}
}

static std::vector<uint8_t> bytes(const char *value)
{
	return { value, value + std::char_traits<char>::length(value) };
}

static std::vector<uint8_t> read(const fs::path &path)
{
	std::ifstream input(path, std::ios::binary);
	return { std::istreambuf_iterator<char>(input), std::istreambuf_iterator<char>() };
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
	require(flatfile_atomic_write(domains.string(), "authority-one", bytes("old-one"),
				      &error) &&
			flatfile_atomic_write(domains.string(), "authority-two", bytes("old-two"),
					      &error),
		"could not establish authority fixtures: " + error);
	const std::vector<flatfile_authority_after_image> images = {
		{ "authority-one", bytes("new-one") },
		{ "authority-two", bytes("new-two") },
	};
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire authority lock");
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
		require(flatfile_authority_transaction_commit(root.string(), lock, images,
							      &error) ==
				flatfile_authority_transaction_result::io_error,
			"fault injection did not interrupt authority transaction");
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	}
	require(read(domains / "authority-one") == bytes("new-one") &&
			read(domains / "authority-two") == bytes("old-two") &&
			fs::exists(domains / ".critical-authority-transaction"),
		"interruption did not leave the expected recoverable split state");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not reacquire authority lock");
		require(flatfile_authority_transaction_recover(root.string(), lock, &error) ==
				flatfile_authority_transaction_result::ok,
			"authority recovery failed: " + error);
	}
	require(read(domains / "authority-one") == bytes("new-one") &&
			read(domains / "authority-two") == bytes("new-two") &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"authority recovery did not publish and clear every after-image");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire validation lock");
		require(flatfile_authority_transaction_commit(
				root.string(), lock, { { "../escape", bytes("bad") } }, &error) ==
				flatfile_authority_transaction_result::invalid,
			"unsafe authority target was accepted");
	}
	require(flatfile_atomic_write(domains.string(), "authority-one", bytes("old-one"),
				      &error) &&
			flatfile_atomic_write(domains.string(), "authority-two", bytes("old-two"),
					      &error),
		"could not reset corruption fixtures");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire corruption lock");
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
		require(flatfile_authority_transaction_commit(root.string(), lock, images,
							      &error) ==
				flatfile_authority_transaction_result::io_error,
			"could not create journal for corruption test");
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	}
	const fs::path journal = domains / ".critical-authority-transaction";
	{
		std::fstream file(journal, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open authority journal for corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x40;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not reacquire corruption lock");
		require(flatfile_authority_transaction_recover(root.string(), lock, &error) ==
				flatfile_authority_transaction_result::invalid,
			"corrupt authority journal was accepted");
	}
	require(read(domains / "authority-one") == bytes("new-one") &&
			read(domains / "authority-two") == bytes("old-two") && fs::exists(journal),
		"corrupt journal recovery changed or discarded split authority state");

	std::cout << "flat-file cross-authority transaction passed\n";
	return 0;
}
