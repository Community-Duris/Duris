#include "flatfile_offline_message_repository.h"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
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

static flatfile_offline_message_id message_id(uint8_t value)
{
	flatfile_offline_message_id id = {};
	id.front() = 0xa5;
	id.back() = value;
	return id;
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
	std::vector<flatfile_offline_message_record> messages;
	require(flatfile_offline_message_list(root.string(), 42, &messages, &error) ==
				flatfile_offline_message_result::ok &&
			messages.empty(),
		"missing offline catalog was not an empty mailbox");
	const auto first = message_id(1);
	const auto second = message_id(2);
	require(flatfile_offline_message_enqueue(root.string(), 42, second, "second\r\n", &error) ==
				flatfile_offline_message_result::ok &&
			flatfile_offline_message_enqueue(root.string(), 42, first, "first\r\n",
							 &error) ==
				flatfile_offline_message_result::ok,
		"could not enqueue offline messages: " + error);
	const fs::path catalog = domains / "offline_messages_42";
	const auto stable_size = fs::file_size(catalog);
	require(flatfile_offline_message_enqueue(root.string(), 42, first, "first\r\n", &error) ==
				flatfile_offline_message_result::ok &&
			fs::file_size(catalog) == stable_size,
		"exact offline enqueue replay was not idempotent");
	require(flatfile_offline_message_enqueue(root.string(), 42, first, "conflict\r\n",
						 &error) ==
			flatfile_offline_message_result::conflict,
		"conflicting offline message ID was accepted");
	require(flatfile_offline_message_list(root.string(), 42, &messages, &error) ==
				flatfile_offline_message_result::ok &&
			messages.size() == 2 && messages[0].id == first &&
			messages[0].text == "first\r\n" && messages[1].id == second,
		"offline mailbox did not decode in deterministic ID order");
	require(flatfile_offline_message_acknowledge(root.string(), 42, first, &error) ==
				flatfile_offline_message_result::ok &&
			flatfile_offline_message_acknowledge(root.string(), 42, first, &error) ==
				flatfile_offline_message_result::not_found &&
			flatfile_offline_message_list(root.string(), 42, &messages, &error) ==
				flatfile_offline_message_result::ok &&
			messages.size() == 1 && messages[0].id == second,
		"offline acknowledgement did not remove exactly one message");
	require(flatfile_offline_message_enqueue(root.string(), 0, second, "bad", &error) ==
				flatfile_offline_message_result::invalid &&
			flatfile_offline_message_enqueue(root.string(), 42, {}, "bad", &error) ==
				flatfile_offline_message_result::invalid,
		"invalid offline message identity was accepted");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not acquire authority lock for offline removal");
		flatfile_authority_operation operation;
		require(flatfile_offline_message_prepare_remove(root.string(), lock, 42, &operation,
								&error) ==
					flatfile_offline_message_result::ok &&
				operation.store == flatfile_authority_store::domains &&
				operation.kind == flatfile_authority_operation_kind::remove &&
				operation.filename == "offline_messages_42",
			"offline removal did not prepare the expected operation");
		require(flatfile_offline_message_prepare_remove(root.string(), lock, 99, &operation,
								&error) ==
				flatfile_offline_message_result::unchanged,
			"missing offline mailbox was not an idempotent removal");
	}
	{
		std::fstream file(catalog, std::ios::in | std::ios::out | std::ios::binary);
		require(file.good(), "could not open offline catalog for corruption");
		file.seekg(-1, std::ios::end);
		char value = 0;
		file.read(&value, 1);
		value ^= 0x39;
		file.seekp(-1, std::ios::end);
		file.write(&value, 1);
	}
	require(flatfile_offline_message_list(root.string(), 42, &messages, &error) ==
				flatfile_offline_message_result::invalid &&
			flatfile_offline_message_enqueue(root.string(), 42, message_id(3), "third",
							 &error) ==
				flatfile_offline_message_result::invalid,
		"corrupt offline catalog was exposed or overwritten");
	{
		flatfile_authority_lock lock;
		require(lock.acquire(root.string(), &error),
			"could not reacquire authority lock for corrupt offline removal");
		flatfile_authority_operation operation;
		require(flatfile_offline_message_prepare_remove(root.string(), lock, 42, &operation,
								&error) ==
				flatfile_offline_message_result::invalid,
			"corrupt offline mailbox was accepted for removal");
	}
	for (const auto &entry : fs::directory_iterator(domains))
		require(entry.path().filename().string().find(".tmp.") == std::string::npos,
			"temporary offline-message file was left behind");
	std::cout << "flat-file offline message repository passed\n";
	return 0;
}
