#include "flatfile_authority_transaction.h"
#include "flatfile_store.h"

#include <array>
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <openssl/sha.h>
#include <string>
#include <type_traits>

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

template <typename T> static void number(std::vector<uint8_t> *output, T value)
{
	using U = std::make_unsigned_t<T>;
	U bits = static_cast<U>(value);
	for (size_t index = 0; index < sizeof(T); ++index)
	{
		output->push_back(static_cast<uint8_t>(bits & 0xff));
		bits >>= 8;
	}
}

static std::vector<uint8_t>
legacy_journal(const std::vector<flatfile_authority_after_image> &images)
{
	std::vector<uint8_t> payload;
	number<uint16_t>(&payload, images.size());
	for (const auto &image : images)
	{
		number<uint16_t>(&payload, image.filename.size());
		payload.insert(payload.end(), image.filename.begin(), image.filename.end());
		number<uint32_t>(&payload, image.bytes.size());
		payload.insert(payload.end(), image.bytes.begin(), image.bytes.end());
	}
	std::array<uint8_t, SHA256_DIGEST_LENGTH> digest = {};
	SHA256(payload.data(), payload.size(), digest.data());
	std::vector<uint8_t> journal = { 'D', 'U', 'R', 'A', 'U', 'T', 'H', 0 };
	number<uint32_t>(&journal, 1);
	number<uint32_t>(&journal, payload.size());
	journal.insert(journal.end(), digest.begin(), digest.end());
	journal.insert(journal.end(), payload.begin(), payload.end());
	return journal;
}

int main(int argc, char **argv)
{
	require(argc == 2, "state root argument required");
	const fs::path root = argv[1];
	const fs::path domains = root / "domains";
	const fs::path players = root / "players";
	fs::create_directories(domains);
	fs::create_directories(players);
	fs::permissions(root, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(domains, fs::perms::owner_all, fs::perm_options::replace);
	fs::permissions(players, fs::perms::owner_all, fs::perm_options::replace);
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
	require(flatfile_atomic_write(players.string(), "42", bytes("player-snapshot"), &error) &&
			flatfile_atomic_write(domains.string(), "player_42", bytes("player-domain"),
					      &error),
		"could not establish removal fixtures: " + error);
	const std::vector<flatfile_authority_operation> operations = {
		{ flatfile_authority_store::domains, flatfile_authority_operation_kind::write,
		  "authority-one", bytes("cross-store") },
		{ flatfile_authority_store::players,
		  flatfile_authority_operation_kind::remove,
		  "42",
		  {} },
		{ flatfile_authority_store::domains,
		  flatfile_authority_operation_kind::remove,
		  "player_42",
		  {} },
	};
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire removal lock");
		setenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE", "1", 1);
		require(flatfile_authority_transaction_commit_operations(root.string(), lock,
									 operations, &error) ==
				flatfile_authority_transaction_result::io_error,
			"fault injection did not interrupt cross-store removal");
		unsetenv("DURIS_FLATFILE_TEST_INTERRUPT_AFTER_AUTHORITY_IMAGE");
	}
	require(read(domains / "authority-one") == bytes("cross-store") &&
			fs::exists(players / "42") && fs::exists(domains / "player_42"),
		"interruption did not preserve the unapplied removals");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not reacquire removal lock");
		require(flatfile_authority_transaction_recover(root.string(), lock, &error) ==
				flatfile_authority_transaction_result::ok,
			"cross-store removal recovery failed: " + error);
	}
	require(!fs::exists(players / "42") && !fs::exists(domains / "player_42") &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"recovery did not complete and clear cross-store removals");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire idempotency lock");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock,
				{ { flatfile_authority_store::players,
				    flatfile_authority_operation_kind::remove,
				    "42",
				    {} } },
				&error) == flatfile_authority_transaction_result::ok,
			"missing cross-store removal was not idempotent: " + error);
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error), "could not acquire validation lock");
		require(flatfile_authority_transaction_commit(
				root.string(), lock, { { "../escape", bytes("bad") } }, &error) ==
				flatfile_authority_transaction_result::invalid,
			"unsafe authority target was accepted");
	}
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire operation validation lock");
		require(flatfile_authority_transaction_commit_operations(
				root.string(), lock,
				{ { flatfile_authority_store::players,
				    flatfile_authority_operation_kind::remove, "43",
				    bytes("bad") } },
				&error) == flatfile_authority_transaction_result::invalid,
			"removal operation with a payload was accepted");
	}
	require(flatfile_atomic_write(domains.string(), "legacy-one", bytes("old-legacy"),
				      &error) &&
			flatfile_atomic_write(
				domains.string(), ".critical-authority-transaction",
				legacy_journal({ { "legacy-one", bytes("new-legacy") } }), &error),
		"could not establish legacy journal fixture: " + error);
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire legacy recovery lock");
		require(flatfile_authority_transaction_recover(root.string(), lock, &error) ==
				flatfile_authority_transaction_result::ok,
			"version-1 authority journal recovery failed: " + error);
	}
	require(read(domains / "legacy-one") == bytes("new-legacy") &&
			!fs::exists(domains / ".critical-authority-transaction"),
		"version-1 authority journal was not replayed and cleared");
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
